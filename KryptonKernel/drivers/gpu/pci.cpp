#include "pci.h"
#include "../vga/printf.h"
#include "../../hal/io.h"

void pci_init() {
    printf("initializing PCI bus...\n");
    pci_scan();
}

uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint16_t)(pci_read_dword(bus, slot, func, offset) >> ((offset & 2) * 8));
}

const char* pci_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        default:   return "Unknown Class";
    }
}

const char* pci_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086: return "Intel Corporation";
        case 0x10DE: return "NVIDIA Corporation";
        case 0x1002: return "Advanced Micro Devices(AMD), Inc.";
        case 0x1234: return "QEMU Virtual Device";
        case 0x1B21: return "XFX Pine Group Inc.";
        case 0x1B39: return "ASUS Computer Inc.";
        case 0x8087: return "Realtek Semiconductor Corp.";
        case 0x1414: return "TSMC";
        case 0x1022: return "Toshiba";
        case 0x1A0D: return "Asrock";
        case 0x1B36: return "Gigabyte";
        case 0x1B3E: return "MSI";
        case 0x1B3F: return "ASRock";
        default:     return "Unknown Vendor";
    }
}

void pci_scan() {
    printf("scanning PCI devices...\n");

    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {

                uint16_t vendor = pci_read_word(bus, slot, func, 0);
                if (vendor == 0xFFFF) continue;

                uint16_t device = pci_read_word(bus, slot, func, 2);
                uint8_t class_code = (pci_read_word(bus, slot, func, 0x0A) >> 8) & 0xFF;

                printf("[");
                printf("%02x", bus);
                printf(":");
                printf("%02x", slot);
                printf(".");
                printf("%x", func);
                printf("] Vendor: 0x");
                printf("%04x", vendor);
                printf(" (");
                printf(pci_vendor_name(vendor));
                printf(") | Device: 0x");
                printf("%04x", device);
                printf(" | Class: 0x");
                printf("%02x", class_code);
                printf(" (");
                printf(pci_class_name(class_code));
                printf(")\n");

                printf("[%02X:%02X.%d] Vendor: %s (0x%04X), Device: 0x%04X, Class: %s (0x%02X)\n",
                    bus, slot, func, pci_vendor_name(vendor), vendor, device, pci_class_name(class_code), class_code);
                if (vendor == 0x1002 && class_code == 0x03) {
                    printf("  Found AMD GPU at [%02X:%02X.%d]\n", bus, slot, func);
                }
            }
        }
    }
}