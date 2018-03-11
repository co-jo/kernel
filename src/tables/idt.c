#include "system.h"
#include "idt.h"

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

void idt_install()
{
  idt_ptr.limit = sizeof(idt_entry_t) * 256 -1;
  idt_ptr.base  = (unsigned int)&idt_entries;
  memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

  idt_flush((unsigned int)&idt_ptr);
}

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags)
{
  idt_entries[num].base_lo = base & 0xFFFF;
  idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

  idt_entries[num].sel     = sel;
  idt_entries[num].always0 = 0;
  idt_entries[num].flags   = flags  | 0x60;
}
