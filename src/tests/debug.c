#include "isr.h"
#include "system.h"
#include "kheap.h"
#include "paging.h"

void debug_regs(regs_t *regs)
{
  line();
  printf("DS[%x] : ", regs->ds);
  printf("ERR: [%d]\n", regs->int_no);
  printf("[SS]:[ESP] - [%x]", regs->ss);
  printf("[%x]\n", regs->esp);
  printf("[CS]:[EIP] - [%x]", regs->cs);
  printf("[%x]\n", regs->eip);
  line();
}

void dump_regs(regs_t *regs)
{
  line();
 // printf("GS[%x]\n", regs->gs);
 // printf("FS[%x]\n", regs->fs);
 // printf("ES[%x]\n", regs->es);
 // printf("DS[%x]\n", regs->ds);
  printf("EDI[%x]\n", regs->edi);
  printf("ESI[%x]\n", regs->esi);
  printf("EBP[%x]\n", regs->ebp);
  printf("ESP[%x]\n", regs->esp);
  printf("EBX[%x]\n", regs->ebx);
  printf("EDX[%x]\n", regs->edx);
  printf("ECX[%x]\n", regs->ecx);
  printf("EAX[%x]\n", regs->eax);
  printf("INT[%x]\n", regs->int_no);
  printf("ERR[%b]\n", regs->err_code);
  
  unsigned int* ptr = (unsigned int *)regs->eip;
  printf("EIP[%x]\n", *ptr);
  printf("CS[%x]\n", regs->cs);
  printf("EFLAGS[%b]\n", regs->eflags);
  printf("USERESP[%x]\n", regs->useresp);
  printf("SS[%x]\n", regs->ss);
  printf("LAST FRAME: [%x]\n", first_frame() - 1);
  line();
}

void print_stack(int num, unsigned int esp)
{
  int i;
  int *ptr;
  for (i = esp; i >  esp - 4*num; i-=4) {
    ptr = (int *)i;
    printf("STACK[%x] - ", i);
    printf("[%x]\n", *ptr);
  }
}

void print_user_mode_flag()
{
  int user_flag = 0;
  asm volatile("mov %%cr0, %0": "=r"(user_flag));
  printf("CR0 [%b]\n", user_flag);
  printf("USER MODE FLAG [%x]\n", user_flag&0x01);
}

void *gdb(void *ptr)
{
  return ++ptr;
}
