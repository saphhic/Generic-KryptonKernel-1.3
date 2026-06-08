#ifndef PCI_H
#define PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar[6];
    uint16_t subsystem_vendor;
    uint16_t subsystem_id;
    uint32_t expansion_rom;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
}  pci_device_t;

void pci_init();
void pci_scan();
void pci_list_devices();

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

const char* pci_class_name(uint8_t class_code);
const char* pci_vendor_name(uint16_t vendor_id);

#endif // PCI_H
