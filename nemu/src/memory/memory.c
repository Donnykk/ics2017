#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define PTE_ADDR(pte) ((uint32_t)(pte) & ~0xfff)

#define PDX(va) (((uint32_t)(va) >> 22) & 0x3ff)
#define PTX(va) (((uint32_t)(va) >> 12) & 0x3ff)
#define OFF(va) ((uint32_t)(va) & 0xfff)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int r = is_mmio(addr);  
  if(r == -1)
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  else 
    return mmio_read(addr, len, r);
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int r = is_mmio(addr);  
  if(r == -1)
    memcpy(guest_to_host(addr), &data, len);
  else  
    mmio_write(addr, len, data, r);
}

paddr_t page_translate(vaddr_t addr, bool iswrite) {
  CR0 cr0 = (CR0)cpu.CR0;
  if(cr0.paging && cr0.protect_enable) {
    CR3 crs = (CR3)cpu.CR3;

    PDE *pgdirs = (PDE*)PTE_ADDR(crs.val);
    PDE pde = (PDE)paddr_read((uint32_t)(pgdirs + PDX(addr)), 4);
    Assert(pde.present, "addr=0x%x", addr);

    PTE *ptable = (PTE*)PTE_ADDR(pde.val);
    PTE pte = (PTE)paddr_read((uint32_t)(ptable + PTX(addr)), 4);
    Assert(pte.present, "addr=0x%x", addr);

    pde.accessed=1;
    pte.accessed=1;
    if(iswrite)
      pte.dirty=1;

    paddr_t paddr = PTE_ADDR(pte.val) | OFF(addr);
    //printf("vaddr=0x%x, paddr=0x%x\n", addr, paddr);
    return paddr;
  }
	return addr;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if(PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)) {
    // printf("error: the data pass two pages:addr=0x%x, len=%d!\n", addr, len);
    // assert(0);
    int page1_num = 0x1000 - OFF(addr);
    int page2_num = len - page1_num;
    paddr_t paddr1 = page_translate(addr, false);
    paddr_t paddr2 = page_translate(addr + page1_num, false);

    uint32_t low = paddr_read(paddr1, page1_num);
    uint32_t high = paddr_read(paddr2, page2_num);

    uint32_t result = high << (page1_num * 8) | low;
    return result;
  } else {
    paddr_t paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if(PTE_ADDR(addr) != PTE_ADDR(addr+len-1)) {
    // printf("error: the data pass two pages:addr=0x%x, len=%d!\n", addr, len);
    // assert(0);
    if(PTE_ADDR(addr) != PTE_ADDR(addr + len -1)) {
      int page1_num = 0x1000-OFF(addr);
      int page2_num = len - page1_num;
      paddr_t paddr1 = page_translate(addr, true);
      paddr_t paddr2 = page_translate(addr + page1_num, true);

      uint32_t low = data & (~0u >> ((4 - page1_num) << 3));
      uint32_t high = data >> ((4 - page2_num) << 3);

      paddr_write(paddr1, page1_num, low);
      paddr_write(paddr2, page2_num, high);
      return;
    }
  }
  else {
    paddr_t paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
  }
}
