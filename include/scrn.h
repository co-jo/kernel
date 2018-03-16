#ifndef SCREEN
#define SCREEN

void puts(char *text);
void putch(unsigned char c);
void cls();
void itoa(unsigned int num, unsigned  int base, char *buffer);
void scroll(void);
void move_csr(void);

void printf(char* string, int arg);
void line();

#endif
