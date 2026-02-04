#ifndef UKO_OS_KERNEL__RTL8139_H
#define UKO_OS_KERNEL__RTL8139_H 1

#include <types.h>

void rtl8139_init(u64 pci_device);
void rtl8139_test();
bool eth_send_packet(u8 *buffer, u32 len);

#endif
