#include "system.h"
/* KEDUS means US Keyboard Layout. This is a scancode table
 *  used to layout a standard US keyboard. I have left some
 *  comments in to give you an idea of what key is what, even
 *  though I set it's array index to 0. You can change that to
 *  whatever you want using a macro, if you wish! */
unsigned char kbdus[128] =
{
  0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',     // 11
  '-', '=','\b',                        	                // Backspace
  '\t',		                                  	        // Tab
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
  '\n',	                                                        // Enter key
  0,		                                        	// 29 - Control
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     	// 39
  '\'', '`',   0,		                                // Left shift
  '\\', 'z', 'x', 'c', 'v', 'b', 'n',		          	// 49
  'm', ',', '.', '/',   0,				        // Right shift
  '*',
  0,	                                                        // Alt
  ' ',	                                                        // Space bar
  0,	                                                        // Caps lock
  0,	                                                        // 59 - F1 key ... >
  0,   0,   0,   0,   0,   0,   0,   0,
  0,                                                    	// < ... F10
  0,	                                                        // 69 - Num lock
  0,	                                                        // Scroll Lock
  0,	                                                        // Home key
  0,	                                                        // Up Arrow
  0,	                                                        // Page Up
  '-',
  0,	                                                        // Left Arrow
  0,
  0,	                                                        // Right Arrow
  '+',
  0,	                                                        // 79 - End key
  0,	                                                        // Down Arrow
  0,	                                                        // Page Down
  0,	                                                        // Insert Key
  0,	                                                        // Delete Key
  0, 0,   0,
  0,	                                                        // F11 Key
  0,	                                                        // F12 Key
  0,	                                                        // All other keys are undefined
};

static void handle(char scancode)
{
  if (scancode == 0x48)
    return;
  if (scancode == 0x4B)
    return;
  if (scancode == 0x4D)
    return;
  if (scancode == 0x50)
    return;
  if (scancode == 0xE)
    return;
}

unsigned char special_keys[32] = {
  0x48, // Up Arrow
  0x4B, // Left Arrow
  0x4D, // Right Arrow
  0x50, // Down Arrow
  0xE,  // Backspace
  0x1C  // Newline
};

static int special_key(char scancode)
{
  int i;
  for (i = 0; i < 32; i++)
    if (special_keys[i] == scancode)
      return 1;

  return 0;
}
/* Handles the keyboard interrupt */
void keyboard_handler(struct regs *r)
{
  unsigned char scancode;

  /* Read from the keyboard's data buffer */
  scancode = inportb(0x60);

  /* If the top bit of the byte we read from the keyboard is
   *  set, that means that a key has just been released */
  if (scancode & 0x80)
  {
    /* You can use this one to see if the user released the
     *  shift, alt, or control keys... */
  }
  else
  {
    /* Here, a key was just pressed. Please note that if you
     *  hold a key down, you will get repeated key press
     *  interrupts. */

//    printf("Scancode: [%x]\n", scancode);
      key_handler(scancode, kbdus[scancode]);
  }
}

void keyboard_install()
{
  irq_install_handler(1, keyboard_handler);
}
