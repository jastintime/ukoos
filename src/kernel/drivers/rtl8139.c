#include <arch/riscv64/insns.h>

#include <drivers/rtl8139.h>

#include <mm/paging.h>
#include <physical.h>
#include <print.h>

#define READ_U8(addr, off) physical_read_u8(paddr_of_bits(addr + off))
#define WRITE_U8(addr, off, value) physical_write_u8(paddr_of_bits(addr + off), value)

#define READ_U16(addr, off) physical_read_u16le(paddr_of_bits(addr + off))
#define WRITE_U16(addr, off, value) physical_write_u16le(paddr_of_bits(addr + off), value)

#define READ_U32(addr, off) physical_read_u32le(paddr_of_bits(addr + off))
#define WRITE_U32(addr, off, value) physical_write_u32le(paddr_of_bits(addr + off), value)

#define READ_REG(reg) READ_U8(g_regs_addr, reg)
#define WRITE_REG(reg, value) WRITE_U8(g_regs_addr, reg, value)

#define READ_REG_32(reg) READ_U32(g_regs_addr, reg)
#define WRITE_REG_32(reg, value) WRITE_U32(g_regs_addr, reg, value)

#define REG_TSADn(N) (0x20 + N * 4)
#define REG_TSDn(N) (0x10 + N * 4)

enum RTL8139_REGS {
  REG_COM = 0x37,
  REG_TCR = 0x40,
  REG_CONFIG1 = 0x52
};

enum RTL8139_CONST {
  TSD_TOK = (1 << 15),
  TSD_OWN = (1 << 13),

  COM_RST = (1 << 4),
  COM_RE = (1 << 3),
  COM_TE = (1 << 2)
};


#define REG_IDR0 0x0
#define REG_IDR4 0x4


#define TX_BUFF_SIZE 0x1700

u8 rtl8139_tbuff[4][TX_BUFF_SIZE] __attribute__((aligned(256)));

u8 mac_addr[6];

u64 g_regs_addr;
u64 g_cur_buf = 0;


bool rtl8139_send_packet(u8 *buffer, u32 len) {
  u32 phys_buffer = (u32) walkaddr((uaddr) (&rtl8139_tbuff[0]));
  u64 i;

  for (i=0; i<len; ++i) {
    rtl8139_tbuff[0][i] = buffer[i];
  }

  if ((READ_REG_32(REG_TSDn(g_cur_buf)) & TSD_OWN) == 0) {
    return false;
  }

  sfence_vma();
  WRITE_REG_32(REG_TSADn(g_cur_buf), phys_buffer);
  WRITE_REG_32(REG_TSDn(g_cur_buf), len);

  ++g_cur_buf;
  return true;
}

bool eth_send_packet(u8 *buffer, u32 len) {
  u8 buf[0x100];
  u64 i;

  for (i=0; i<6; ++i) {
    buf[i] = 0xff;
    buf[i + 6] = mac_addr[i];
  }
  buf[12] = 0x08;
  buf[13] = 0x00;

  for (i=0; i<len; ++i) {
    buf[i + 14] = buffer[i];
  }
  for (i=len; i<64; ++i) {
    buf[i + 14] = 0;
  }
  if (len < 64) len = 64;

  return rtl8139_send_packet(buf, len + 13);
}


void read_mac(u8 * mac) {
  u8 i;

  for (i=0; i<6; ++i)
    mac[i] = READ_REG(i);
}


void rtl8139_test() {
  eth_send_packet((u8*) "yellow submarine", 16);

  assert(READ_REG_32(REG_TSADn(0)) != 0);
  assert((READ_REG_32(REG_TSDn(0)) & TSD_TOK) != 0);
}


void rtl8139_init(u64 pci_device) {
  print("initializing rtl8139");

  u32 reg_addr = 0x40000000;
  g_regs_addr = reg_addr;

  /* TODO: Maybe move PCI init to pci.c? */

  /* Setup BAR for MMIO - we just pick a nice number for now */
  WRITE_U32(pci_device,0x14,reg_addr);

  /* Enable memory mapped regs */
  u8 cmd = READ_U8(pci_device, 0x4);
  WRITE_U8(pci_device,0x4,cmd | 0x6);

  WRITE_REG(REG_CONFIG1, 0);

  WRITE_REG(REG_COM, COM_RST);
  while (READ_REG(REG_COM) & COM_RST) ;
  WRITE_REG(REG_COM, COM_RE | COM_TE);

  read_mac(mac_addr);
  rtl8139_test();
}
