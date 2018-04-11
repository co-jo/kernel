#include "system.h"
#include "scrn.h"

void *memcpy(void *dest, const void *src, size_t count)
{
    const char *sp = (const char *)src;
    char *dp = (char *)dest;
    for(; count != 0; count--) *dp++ = *sp++;
    return dest;
}

void *memset(void *dest, char val, size_t count)
{
    char *temp = (char *)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

unsigned short *memsetw(unsigned short *dest, unsigned short val, size_t count)
{
    unsigned short *temp = (unsigned short *)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

size_t strlen(const char *str)
{
    size_t retval;
    for(retval = 0; *str != '\0'; str++) retval++;
    return retval;
}

unsigned char inportb (unsigned short _port)
{
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outportb (unsigned short _port, unsigned char _data)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

int strcmp(const char *str1, const char *str2)
{
    int i;
    for(i = strlen(str1); i >= 0; i--) {
        if (str1[i] != str2[i])
            return 0;
    }
    return 1;
}

void halt()
{
  asm volatile("hlt");
}

void cli(char *message)
{
  asm volatile("cli");
  puts(message);
  putch('\n');
  for (;;);
}

void shutdown(char *message)
{
  set_window_title(message);
  while(1);
}
