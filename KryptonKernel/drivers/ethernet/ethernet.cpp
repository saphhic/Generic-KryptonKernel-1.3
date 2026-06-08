//written by saphhic.

#include "ethernet.h"
#include "debug.h"
#include "pci.h"
#include "r8169.h"
#include "rtl8139.h"
#include "../vga/printf.h"

extern "C" void* memcpy(void* destination, const void* source, unsigned long long size);
extern "C" void* memset(void* destination, int value, unsigned long long size);

namespace {

static bool g_ethernet_probed = false;
static uint8_t g_active_driver = ETHERNET_DRIVER_NONE;

static uint16_t host_to_network16(uint16_t value) {
    return static_cast<uint16_t>((value >> 8) | (value << 8));
}

static const char* ethernet_driver_name(uint8_t driver) {
    if (driver == ETHERNET_DRIVER_R8169) {
        return "r8169";
    }

    if (driver == ETHERNET_DRIVER_RTL8139) {
        return "RTL8139";
    }

    return "none";
}

static void print_pci_location(const eth_pci_device& device) {
    eth_dbg_print_hex8(device.bus);
    putchar(':');
    eth_dbg_print_hex8(device.slot);
    putchar('.');
    eth_dbg_print_hex8(device.function);
}

static void print_unsupported_network_controller() {
    eth_pci_device device = {};
    if (!eth_pci_find_class(0x02, 0x00, &device)) {
        printf("[ethernet] no network controller detected.\n");
        return;
    }

    printf("[ethernet] no supported driver initialized; first ethernet controller vendor=0x");
    eth_dbg_print_hex16(device.vendor_id);
    printf(" device=0x");
    eth_dbg_print_hex16(device.device_id);
    printf(".\n");
}

static void print_r8169_ready() {
    r8169_info info = {};
    r8169_get_info(&info);

    printf("[ethernet] r8169 ready on PCI ");
    print_pci_location(info.pci_device);
    if (info.using_mmio) {
        printf(" mmio=0x");
        eth_dbg_print_hex32(info.mmio_base);
    } else {
        printf(" io=0x");
        eth_dbg_print_hex16(info.io_base);
    }
    printf(" mac=");
    eth_dbg_print_mac(info.mac);
    printf(" link=");
    printf(info.link_up ? "up" : "down");
    printf(".\n");
}

static void print_rtl8139_ready() {
    rtl8139_info info = {};
    rtl8139_get_info(&info);

    printf("[ethernet] RTL8139 ready on PCI ");
    print_pci_location(info.pci_device);
    printf(" io=0x");
    eth_dbg_print_hex16(info.io_base);
    printf(" mac=");
    eth_dbg_print_mac(info.mac);
    printf(" link=");
    printf(info.link_up ? "up" : "down");
    printf(".\n");
}

static bool active_driver_is_ready() {
    if (g_active_driver == ETHERNET_DRIVER_R8169) {
        return r8169_is_ready();
    }

    if (g_active_driver == ETHERNET_DRIVER_RTL8139) {
        return rtl8139_is_ready();
    }

    return false;
}

static void active_driver_poll() {
    if (g_active_driver == ETHERNET_DRIVER_R8169) {
        r8169_poll();
    } else if (g_active_driver == ETHERNET_DRIVER_RTL8139) {
        rtl8139_poll();
    }
}

static bool active_driver_send_raw(const uint8_t* frame, uint16_t length) {
    if (g_active_driver == ETHERNET_DRIVER_R8169) {
        return r8169_send_raw(frame, length);
    }

    if (g_active_driver == ETHERNET_DRIVER_RTL8139) {
        return rtl8139_send_raw(frame, length);
    }

    return false;
}

static bool decode_raw_frame(ethernet_packet* out_packet, const uint8_t* bytes, uint16_t length) {
    if (out_packet == nullptr || bytes == nullptr || length < 14) {
        return false;
    }

    const uint16_t payload_length = static_cast<uint16_t>(length - 14);
    if (payload_length > ETHERNET_MAX_PAYLOAD_SIZE) {
        return false;
    }

    memcpy(out_packet->destination, &bytes[0], ETHERNET_ADDRESS_LENGTH);
    memcpy(out_packet->source, &bytes[6], ETHERNET_ADDRESS_LENGTH);
    out_packet->ether_type = static_cast<uint16_t>(
        (static_cast<uint16_t>(bytes[12]) << 8) |
        static_cast<uint16_t>(bytes[13])
    );
    out_packet->payload_length = payload_length;
    if (payload_length > 0) {
        memcpy(out_packet->payload, &bytes[14], payload_length);
    }

    return true;
}

static bool active_driver_receive_raw(ethernet_packet* out_packet) {
    if (g_active_driver == ETHERNET_DRIVER_R8169) {
        r8169_raw_frame raw_frame = {};
        if (!r8169_receive_raw(&raw_frame)) {
            return false;
        }

        return decode_raw_frame(out_packet, raw_frame.bytes, raw_frame.length);
    }

    if (g_active_driver == ETHERNET_DRIVER_RTL8139) {
        rtl8139_raw_frame raw_frame = {};
        if (!rtl8139_receive_raw(&raw_frame)) {
            return false;
        }

        return decode_raw_frame(out_packet, raw_frame.bytes, raw_frame.length);
    }

    return false;
}

} // namespace

void ethernet_init() {
    if (g_ethernet_probed) {
        return;
    }

    g_ethernet_probed = true;
    printf("[ethernet] probing PCI NICs...\n");

    if (r8169_init()) {
        g_active_driver = ETHERNET_DRIVER_R8169;
        print_r8169_ready();
        return;
    }

    if (rtl8139_init()) {
        g_active_driver = ETHERNET_DRIVER_RTL8139;
        print_rtl8139_ready();
        return;
    }

    print_unsupported_network_controller();
}

bool ethernet_is_ready() {
    return active_driver_is_ready();
}

void ethernet_poll() {
    active_driver_poll();
}

bool ethernet_send(const uint8_t destination[ETHERNET_ADDRESS_LENGTH], uint16_t ether_type, const void* payload, uint16_t payload_length) {
    if (!active_driver_is_ready() || destination == nullptr) {
        return false;
    }

    if (payload_length > ETHERNET_MAX_PAYLOAD_SIZE) {
        return false;
    }

    ethernet_info info = {};
    ethernet_get_info(&info);
    if (!info.ready) {
        return false;
    }

    uint8_t frame[14 + ETHERNET_MAX_PAYLOAD_SIZE];
    memset(frame, 0, sizeof(frame));

    memcpy(&frame[0], destination, ETHERNET_ADDRESS_LENGTH);
    memcpy(&frame[6], info.mac, ETHERNET_ADDRESS_LENGTH);

    const uint16_t network_ether_type = host_to_network16(ether_type);
    memcpy(&frame[12], &network_ether_type, sizeof(network_ether_type));

    if (payload_length > 0 && payload != nullptr) {
        memcpy(&frame[14], payload, payload_length);
    }

    return active_driver_send_raw(frame, static_cast<uint16_t>(14 + payload_length));
}

bool ethernet_receive(ethernet_packet* out_packet) {
    if (out_packet == nullptr || !active_driver_is_ready()) {
        return false;
    }

    active_driver_poll();
    return active_driver_receive_raw(out_packet);
}

void ethernet_get_info(ethernet_info* out_info) {
    if (out_info == nullptr) {
        return;
    }

    memset(out_info, 0, sizeof(ethernet_info));
    out_info->driver = g_active_driver;

    if (g_active_driver == ETHERNET_DRIVER_R8169) {
        r8169_info info = {};
        r8169_get_info(&info);

        out_info->ready = info.initialized;
        out_info->link_up = info.link_up;
        out_info->using_mmio = info.using_mmio;
        out_info->pci_vendor_id = info.pci_device.vendor_id;
        out_info->pci_device_id = info.pci_device.device_id;
        out_info->io_base = info.io_base;
        out_info->mmio_base = info.mmio_base;
        out_info->tx_packets = info.stats.tx_packets;
        out_info->rx_packets = info.stats.rx_packets;
        out_info->tx_errors = info.stats.tx_errors;
        out_info->rx_errors = info.stats.rx_errors;
        out_info->tx_dropped = info.stats.tx_dropped;
        out_info->rx_dropped = info.stats.rx_dropped;
        out_info->resets = info.stats.resets;
        out_info->queued_frames = info.queued_frames;
        memcpy(out_info->mac, info.mac, ETHERNET_ADDRESS_LENGTH);
        return;
    }

    if (g_active_driver == ETHERNET_DRIVER_RTL8139) {
        rtl8139_info info = {};
        rtl8139_get_info(&info);

        out_info->ready = info.initialized;
        out_info->link_up = info.link_up;
        out_info->using_mmio = false;
        out_info->pci_vendor_id = info.pci_device.vendor_id;
        out_info->pci_device_id = info.pci_device.device_id;
        out_info->io_base = info.io_base;
        out_info->tx_packets = info.stats.tx_packets;
        out_info->rx_packets = info.stats.rx_packets;
        out_info->tx_errors = info.stats.tx_errors;
        out_info->rx_errors = info.stats.rx_errors;
        out_info->tx_dropped = info.stats.tx_dropped;
        out_info->rx_dropped = info.stats.rx_dropped;
        out_info->resets = info.stats.resets;
        out_info->queued_frames = info.queued_frames;
        memcpy(out_info->mac, info.mac, ETHERNET_ADDRESS_LENGTH);
    }
}

void ethernet_debug_print_status() {
    ethernet_info info = {};
    ethernet_get_info(&info);

    if (!info.ready) {
        printf("[ethernet] driver not ready.\n");
        return;
    }

    printf("[ethernet] driver=");
    printf(ethernet_driver_name(info.driver));
    putchar(' ');
    if (info.using_mmio) {
        printf("mmio=0x");
        eth_dbg_print_hex32(info.mmio_base);
    } else {
        printf("io=0x");
        eth_dbg_print_hex16(info.io_base);
    }
    printf(" mac=");
    eth_dbg_print_mac(info.mac);
    printf(" link=");
    printf(info.link_up ? "up" : "down");
    printf(" tx=");
    eth_dbg_print_dec(info.tx_packets);
    printf(" rx=");
    eth_dbg_print_dec(info.rx_packets);
    printf(" queued=");
    eth_dbg_print_dec(info.queued_frames);
    printf(" txerr=");
    eth_dbg_print_dec(info.tx_errors);
    printf(" rxerr=");
    eth_dbg_print_dec(info.rx_errors);
    printf(" drops=");
    eth_dbg_print_dec(info.tx_dropped + info.rx_dropped);
    printf(" resets=");
    eth_dbg_print_dec(info.resets);
    printf(".\n");
}

void ethernet_debug_drain_frames() {
    if (!active_driver_is_ready()) {
        printf("[ethernet] driver not ready.\n");
        return;
    }

    ethernet_poll();

    uint32_t drained = 0;
    ethernet_packet packet = {};
    while (ethernet_receive(&packet) && drained < 16) {
        printf("[ethernet] rx type=0x");
        eth_dbg_print_hex16(packet.ether_type);
        printf(" len=");
        eth_dbg_print_dec(packet.payload_length);
        printf(" dst=");
        eth_dbg_print_mac(packet.destination);
        printf(" src=");
        eth_dbg_print_mac(packet.source);
        printf(".\n");
        ++drained;
    }

    if (drained == 0) {
        printf("[ethernet] no queued frames.\n");
    }
}
