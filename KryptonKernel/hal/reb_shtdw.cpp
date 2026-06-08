// written by saphhic.

#include "reb_shtdw.h"

#if defined(LAWAI_TARGET_UEFI)

#include "../../uefi/runtime.hpp"

namespace {
[[noreturn]] void stop_forever() {
    for (;;) {
    }
}
}  // namespace

void shutdown() {
    EFI_RUNTIME_SERVICES* runtime = lawai_runtime_services();
    if (runtime != nullptr && runtime->ResetSystem != nullptr) {
        runtime->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, nullptr);
    }

    stop_forever();
}

void reboot() {
    EFI_RUNTIME_SERVICES* runtime = lawai_runtime_services();
    if (runtime != nullptr && runtime->ResetSystem != nullptr) {
        runtime->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, nullptr);
    }

    stop_forever();
}

#else

#include "io.h"

#include <stddef.h>
#include <stdint.h>

namespace {
constexpr uint16_t kAcpiSciEnable = 1u << 0;
constexpr uint16_t kAcpiSleepTypeMask = 7u << 10;
constexpr uint16_t kAcpiSleepEnable = 1u << 13;
constexpr uint8_t kAddressSpaceSystemMemory = 0;
constexpr uint8_t kAddressSpaceSystemIo = 1;

#pragma pack(push, 1)
struct AcpiRsdpDescriptor {
    char Signature[8];
    uint8_t Checksum;
    char OemId[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
};

struct AcpiRsdpDescriptor20 {
    AcpiRsdpDescriptor FirstPart;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t Reserved[3];
};

struct AcpiSdtHeader {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OemId[6];
    char OemTableId[8];
    uint32_t OemRevision;
    uint32_t CreatorId;
    uint32_t CreatorRevision;
};

struct AcpiGenericAddress {
    uint8_t AddressSpaceId;
    uint8_t RegisterBitWidth;
    uint8_t RegisterBitOffset;
    uint8_t AccessSize;
    uint64_t Address;
};

struct AcpiFadt {
    AcpiSdtHeader Header;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;
    uint8_t Reserved0;
    uint8_t PreferredPmProfile;
    uint16_t SciInterrupt;
    uint32_t SmiCommandPort;
    uint8_t AcpiEnableValue;
    uint8_t AcpiDisableValue;
    uint8_t S4BiosRequest;
    uint8_t PstateControl;
    uint32_t Pm1aEventBlock;
    uint32_t Pm1bEventBlock;
    uint32_t Pm1aControlBlock;
    uint32_t Pm1bControlBlock;
    uint32_t Pm2ControlBlock;
    uint32_t PmTimerBlock;
    uint32_t Gpe0Block;
    uint32_t Gpe1Block;
    uint8_t Pm1EventLength;
    uint8_t Pm1ControlLength;
    uint8_t Pm2ControlLength;
    uint8_t PmTimerLength;
    uint8_t Gpe0Length;
    uint8_t Gpe1Length;
    uint8_t Gpe1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;
    uint16_t BootArchitectureFlags;
    uint8_t Reserved1;
    uint32_t Flags;
    AcpiGenericAddress ResetRegister;
    uint8_t ResetValue;
    uint8_t Reserved2[3];
    uint64_t XFirmwareCtrl;
    uint64_t XDsdt;
    AcpiGenericAddress XPm1aEventBlock;
    AcpiGenericAddress XPm1bEventBlock;
    AcpiGenericAddress XPm1aControlBlock;
    AcpiGenericAddress XPm1bControlBlock;
};

struct CpuDescriptorTablePointer {
    uint16_t Limit;
    uint32_t Base;
};
#pragma pack(pop)

struct AcpiControlRegister {
    uint8_t AddressSpaceId;
    uint64_t Address;
};

struct AcpiPowerContext {
    bool CanShutdown;
    bool CanReset;
    uint16_t SleepTypeA;
    uint16_t SleepTypeB;
    AcpiControlRegister Pm1aControl;
    AcpiControlRegister Pm1bControl;
    uint32_t SmiCommandPort;
    uint8_t AcpiEnableValue;
    AcpiGenericAddress ResetRegister;
    uint8_t ResetValue;
};

bool signature_equals(const char* left, const char* right, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (left[i] != right[i]) {
            return false;
        }
    }

    return true;
}

bool address_fits_u32(uint64_t address) {
    return address != 0 && address <= 0xFFFFFFFFull;
}

uint8_t checksum_bytes(const void* base, size_t length) {
    const uint8_t* bytes = static_cast<const uint8_t*>(base);
    uint8_t sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum = static_cast<uint8_t>(sum + bytes[i]);
    }

    return sum;
}

bool checksum_ok(const void* base, size_t length) {
    return checksum_bytes(base, length) == 0;
}

const AcpiRsdpDescriptor* find_rsdp_range(uintptr_t start, uintptr_t end) {
    for (uintptr_t address = start; address + sizeof(AcpiRsdpDescriptor) <= end; address += 16) {
        const auto* rsdp = reinterpret_cast<const AcpiRsdpDescriptor*>(address);
        if (!signature_equals(rsdp->Signature, "RSD PTR ", 8)) {
            continue;
        }

        if (!checksum_ok(rsdp, sizeof(AcpiRsdpDescriptor))) {
            continue;
        }

        if (rsdp->Revision >= 2) {
            const auto* extended = reinterpret_cast<const AcpiRsdpDescriptor20*>(rsdp);
            if (extended->Length < sizeof(AcpiRsdpDescriptor20)) {
                continue;
            }
            if (!checksum_ok(extended, extended->Length)) {
                continue;
            }
        }

        return rsdp;
    }

    return nullptr;
}

const AcpiRsdpDescriptor* find_rsdp() {
    const uintptr_t ebda_segment_address = 0x40E;
    const uint16_t ebda_segment = *reinterpret_cast<volatile const uint16_t*>(ebda_segment_address);
    const uintptr_t ebda_base = static_cast<uintptr_t>(ebda_segment) << 4;
    if (ebda_base >= 0x80000 && ebda_base < 0xA0000) {
        const AcpiRsdpDescriptor* rsdp = find_rsdp_range(ebda_base, ebda_base + 1024);
        if (rsdp != nullptr) {
            return rsdp;
        }
    }

    return find_rsdp_range(0xE0000, 0x100000);
}

bool table_checksum_ok(const AcpiSdtHeader* table) {
    return table != nullptr &&
           table->Length >= sizeof(AcpiSdtHeader) &&
           checksum_ok(table, table->Length);
}

const AcpiSdtHeader* resolve_table_32(uint32_t address) {
    if (address == 0) {
        return nullptr;
    }

    const auto* table = reinterpret_cast<const AcpiSdtHeader*>(static_cast<uintptr_t>(address));
    return table_checksum_ok(table) ? table : nullptr;
}

const AcpiSdtHeader* resolve_table_64(uint64_t address) {
    if (!address_fits_u32(address)) {
        return nullptr;
    }

    return resolve_table_32(static_cast<uint32_t>(address));
}

const AcpiSdtHeader* find_table_in_sdt(const AcpiSdtHeader* root, const char* signature, size_t entry_size) {
    if (!table_checksum_ok(root) || root->Length < sizeof(AcpiSdtHeader)) {
        return nullptr;
    }

    const uintptr_t entries_base = reinterpret_cast<uintptr_t>(root) + sizeof(AcpiSdtHeader);
    const size_t entry_count = (root->Length - sizeof(AcpiSdtHeader)) / entry_size;
    for (size_t i = 0; i < entry_count; ++i) {
        const AcpiSdtHeader* candidate = nullptr;
        if (entry_size == sizeof(uint32_t)) {
            const uint32_t address = *reinterpret_cast<const uint32_t*>(entries_base + i * entry_size);
            candidate = resolve_table_32(address);
        } else {
            const uint64_t address = *reinterpret_cast<const uint64_t*>(entries_base + i * entry_size);
            candidate = resolve_table_64(address);
        }

        if (candidate != nullptr && signature_equals(candidate->Signature, signature, 4)) {
            return candidate;
        }
    }

    return nullptr;
}

const AcpiSdtHeader* find_acpi_table(const char* signature) {
    const AcpiRsdpDescriptor* rsdp = find_rsdp();
    if (rsdp == nullptr) {
        return nullptr;
    }

    if (rsdp->Revision >= 2) {
        const auto* extended = reinterpret_cast<const AcpiRsdpDescriptor20*>(rsdp);
        const AcpiSdtHeader* xsdt = resolve_table_64(extended->XsdtAddress);
        if (xsdt != nullptr) {
            const AcpiSdtHeader* result = find_table_in_sdt(xsdt, signature, sizeof(uint64_t));
            if (result != nullptr) {
                return result;
            }
        }
    }

    const AcpiSdtHeader* rsdt = resolve_table_32(rsdp->RsdtAddress);
    return find_table_in_sdt(rsdt, signature, sizeof(uint32_t));
}

const AcpiSdtHeader* find_dsdt(const AcpiFadt* fadt) {
    const size_t xdsdt_offset = offsetof(AcpiFadt, XDsdt);
    if (fadt->Header.Length >= xdsdt_offset + sizeof(fadt->XDsdt) && address_fits_u32(fadt->XDsdt)) {
        const AcpiSdtHeader* table = resolve_table_64(fadt->XDsdt);
        if (table != nullptr) {
            return table;
        }
    }

    return resolve_table_32(fadt->Dsdt);
}

AcpiControlRegister select_control_register(uint32_t legacy_address,
                                            const AcpiGenericAddress* extended_register,
                                            bool extended_available) {
    if (legacy_address != 0) {
        return {kAddressSpaceSystemIo, legacy_address};
    }

    if (extended_available &&
        (extended_register->AddressSpaceId == kAddressSpaceSystemIo ||
         extended_register->AddressSpaceId == kAddressSpaceSystemMemory) &&
        address_fits_u32(extended_register->Address)) {
        return {extended_register->AddressSpaceId, extended_register->Address};
    }

    return {0, 0};
}

bool read_register16(const AcpiControlRegister& reg, uint16_t* value) {
    if (value == nullptr || !address_fits_u32(reg.Address)) {
        return false;
    }

    const uintptr_t address = static_cast<uintptr_t>(reg.Address);
    if (reg.AddressSpaceId == kAddressSpaceSystemIo) {
        if (address > 0xFFFFu) {
            return false;
        }
        *value = inw(static_cast<uint16_t>(address));
        return true;
    }

    if (reg.AddressSpaceId == kAddressSpaceSystemMemory) {
        *value = *reinterpret_cast<volatile const uint16_t*>(address);
        return true;
    }

    return false;
}

bool write_register16(const AcpiControlRegister& reg, uint16_t value) {
    if (!address_fits_u32(reg.Address)) {
        return false;
    }

    const uintptr_t address = static_cast<uintptr_t>(reg.Address);
    if (reg.AddressSpaceId == kAddressSpaceSystemIo) {
        if (address > 0xFFFFu) {
            return false;
        }
        outw(static_cast<uint16_t>(address), value);
        return true;
    }

    if (reg.AddressSpaceId == kAddressSpaceSystemMemory) {
        *reinterpret_cast<volatile uint16_t*>(address) = value;
        return true;
    }

    return false;
}

bool write_register8(const AcpiGenericAddress& reg, uint8_t value) {
    if (!address_fits_u32(reg.Address) ||
        (reg.AddressSpaceId != kAddressSpaceSystemIo && reg.AddressSpaceId != kAddressSpaceSystemMemory)) {
        return false;
    }

    const uintptr_t address = static_cast<uintptr_t>(reg.Address);
    if (reg.AddressSpaceId == kAddressSpaceSystemIo) {
        if (address > 0xFFFFu) {
            return false;
        }
        outb(static_cast<uint16_t>(address), value);
        return true;
    }

    *reinterpret_cast<volatile uint8_t*>(address) = value;
    return true;
}

uint8_t parse_aml_byte_value(const uint8_t*& ptr, const uint8_t* end) {
    if (ptr >= end) {
        return 0;
    }

    if (*ptr == 0x00) {
        ++ptr;
        return 0;
    }

    if (*ptr == 0x01) {
        ++ptr;
        return 1;
    }

    if (*ptr == 0x0A) {
        ++ptr;
        if (ptr >= end) {
            return 0;
        }
        const uint8_t value = *ptr;
        ++ptr;
        return value;
    }

    const uint8_t value = *ptr;
    ++ptr;
    return value;
}

bool extract_sleep_types(const AcpiSdtHeader* dsdt, uint16_t* sleep_type_a, uint16_t* sleep_type_b) {
    if (!table_checksum_ok(dsdt) || sleep_type_a == nullptr || sleep_type_b == nullptr) {
        return false;
    }

    const uint8_t* start = reinterpret_cast<const uint8_t*>(dsdt) + sizeof(AcpiSdtHeader);
    const uint8_t* end = reinterpret_cast<const uint8_t*>(dsdt) + dsdt->Length;
    for (const uint8_t* ptr = start; ptr + 4 < end; ++ptr) {
        if (!signature_equals(reinterpret_cast<const char*>(ptr), "_S5_", 4)) {
            continue;
        }

        const bool has_nameop_prefix =
            (ptr > start && ptr[-1] == 0x08) ||
            (ptr > start + 1 && ptr[-2] == 0x08 && ptr[-1] == '\\');
        if (!has_nameop_prefix) {
            continue;
        }

        ptr += 4;
        if (ptr >= end || *ptr != 0x12) {
            continue;
        }

        ++ptr;
        if (ptr >= end) {
            continue;
        }

        const size_t package_length_bytes = static_cast<size_t>(((ptr[0] & 0xC0) >> 6) + 1);
        ptr += package_length_bytes;
        if (ptr >= end) {
            continue;
        }

        ++ptr;
        if (ptr >= end) {
            continue;
        }

        const uint8_t value_a = parse_aml_byte_value(ptr, end);
        const uint8_t value_b = parse_aml_byte_value(ptr, end);
        *sleep_type_a = static_cast<uint16_t>(value_a) << 10;
        *sleep_type_b = static_cast<uint16_t>(value_b) << 10;
        return true;
    }

    return false;
}

bool build_acpi_context(AcpiPowerContext* context) {
    if (context == nullptr) {
        return false;
    }

    const auto* fadt_header = find_acpi_table("FACP");
    if (fadt_header == nullptr || fadt_header->Length < offsetof(AcpiFadt, XPm1bControlBlock)) {
        return false;
    }

    const auto* fadt = reinterpret_cast<const AcpiFadt*>(fadt_header);
    const bool has_extended_control_blocks =
        fadt->Header.Length >= offsetof(AcpiFadt, XPm1bControlBlock) + sizeof(fadt->XPm1bControlBlock);
    const AcpiControlRegister pm1a = select_control_register(
        fadt->Pm1aControlBlock,
        &fadt->XPm1aControlBlock,
        has_extended_control_blocks);
    const AcpiControlRegister pm1b = select_control_register(
        fadt->Pm1bControlBlock,
        &fadt->XPm1bControlBlock,
        has_extended_control_blocks);

    const AcpiSdtHeader* dsdt = find_dsdt(fadt);
    uint16_t sleep_type_a = 0;
    uint16_t sleep_type_b = 0;
    const bool can_shutdown =
        pm1a.Address != 0 && dsdt != nullptr && extract_sleep_types(dsdt, &sleep_type_a, &sleep_type_b);

    context->CanShutdown = can_shutdown;
    context->CanReset = false;
    context->SleepTypeA = sleep_type_a;
    context->SleepTypeB = sleep_type_b;
    context->Pm1aControl = pm1a;
    context->Pm1bControl = pm1b;
    context->SmiCommandPort = fadt->SmiCommandPort;
    context->AcpiEnableValue = fadt->AcpiEnableValue;
    context->ResetRegister = {};
    context->ResetValue = 0;

    const size_t reset_register_offset = offsetof(AcpiFadt, ResetRegister);
    if (fadt->Header.Length >= reset_register_offset + sizeof(fadt->ResetRegister) + sizeof(fadt->ResetValue) &&
        (fadt->ResetRegister.AddressSpaceId == kAddressSpaceSystemIo ||
         fadt->ResetRegister.AddressSpaceId == kAddressSpaceSystemMemory) &&
        address_fits_u32(fadt->ResetRegister.Address)) {
        context->CanReset = true;
        context->ResetRegister = fadt->ResetRegister;
        context->ResetValue = fadt->ResetValue;
    }

    return context->CanShutdown || context->CanReset;
}

AcpiPowerContext* get_acpi_context() {
    static bool initialized = false;
    static AcpiPowerContext context {};

    if (!initialized) {
        build_acpi_context(&context);
        initialized = true;
    }

    return &context;
}

bool ensure_acpi_mode(const AcpiPowerContext& context) {
    if (context.Pm1aControl.Address == 0) {
        return false;
    }

    uint16_t control_value = 0;
    if (!read_register16(context.Pm1aControl, &control_value)) {
        return false;
    }

    if ((control_value & kAcpiSciEnable) != 0) {
        return true;
    }

    if (context.SmiCommandPort == 0 || context.AcpiEnableValue == 0 || context.SmiCommandPort > 0xFFFFu) {
        return false;
    }

    outb(static_cast<uint16_t>(context.SmiCommandPort), context.AcpiEnableValue);

    for (int i = 0; i < 300000; ++i) {
        if (read_register16(context.Pm1aControl, &control_value) &&
            (control_value & kAcpiSciEnable) != 0) {
            return true;
        }
    }

    return false;
}

bool try_acpi_shutdown() {
    AcpiPowerContext* context = get_acpi_context();
    if (context == nullptr || !context->CanShutdown) {
        return false;
    }

    if (!ensure_acpi_mode(*context)) {
        return false;
    }

    uint16_t control_value = 0;
    if (!read_register16(context->Pm1aControl, &control_value)) {
        return false;
    }

    const uint16_t shutdown_value_a =
        static_cast<uint16_t>((control_value & ~kAcpiSleepTypeMask) | context->SleepTypeA | kAcpiSleepEnable);
    if (!write_register16(context->Pm1aControl, shutdown_value_a)) {
        return false;
    }

    if (context->Pm1bControl.Address != 0) {
        uint16_t control_value_b = 0;
        if (read_register16(context->Pm1bControl, &control_value_b)) {
            const uint16_t shutdown_value_b =
                static_cast<uint16_t>((control_value_b & ~kAcpiSleepTypeMask) | context->SleepTypeB | kAcpiSleepEnable);
            write_register16(context->Pm1bControl, shutdown_value_b);
        }
    }

    for (int i = 0; i < 1000000; ++i) {
        asm volatile ("pause");
    }

    return false;
}

bool try_acpi_reset() {
    AcpiPowerContext* context = get_acpi_context();
    if (context == nullptr || !context->CanReset) {
        return false;
    }

    return write_register8(context->ResetRegister, context->ResetValue);
}

[[noreturn]] void halt_forever() {
    asm volatile ("cli");
    for (;;) {
        asm volatile ("hlt");
    }
}

[[noreturn]] void triple_fault_reboot() {
    const CpuDescriptorTablePointer empty_idt = {0, 0};
    asm volatile ("cli");
    asm volatile ("lidt %0" : : "m"(empty_idt));
    asm volatile ("int3");
    halt_forever();
}
}  // namespace

void shutdown() {
    if (try_acpi_shutdown()) {
        return;
    }

    outw(0x0604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
    halt_forever();
}

void reboot() {
    if (try_acpi_reset()) {
        for (int i = 0; i < 1000000; ++i) {
            asm volatile ("pause");
        }
    }

    asm volatile ("cli");
    for (int i = 0; i < 0x10000; ++i) {
        if ((inb(0x64) & 0x02) == 0) {
            outb(0x64, 0xFE);
        }
    }

    triple_fault_reboot();
}

#endif
