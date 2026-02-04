#include <pci.h>
#include <print.h>
#include <mm/paging.h>
#include <physical.h>

#include <drivers/rtl8139.h>

const u64 regs = 0x123L;

const u64 pci_config = 0x30000000L;
const u32 rtl8139_vender = 0x123;

paddr mad(u64 addr) {
  paddr *x;
  x = (paddr*) (&addr);
  return *x;
}


void pci_enumerate() {

  for (u64 dev=0; dev<32; ++dev) {
    u64 bus = 0;
    u64 func = 0;
    u64 offset = 0;
    offset = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    u64 device = (pci_config + offset);

    u16 vid = physical_read_u16le(mad(device));
    u16 did = physical_read_u16le(mad(device+2));

    if (vid == 0x10ec && did == 0x8139){ //0xedf11efd) {
      rtl8139_init(device); //device_pci_cfg);
    } else if (did != 0xffff && vid != 0xffff && vid) {
      print("unknown pci device: {u16:x} vender: {u16:x}", did, vid);
    } 
  }
}
