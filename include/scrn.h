#ifndef SCREEN
#define SCREEN

void puts(char *text);
void putch(unsigned char c);
void cls();
void itoa(unsigned int num, unsigned  int base, char *buffer);
void scroll(int direction);
void move_csr(void);

void printf(char* string, int arg);
void line();
void key_handler(unsigned short scancode, char c);

/* Construct window */
void header();
void window_install();

void print(const char *);
void print_hex(unsigned int);
void print_dec(unsigned int);

#endif
