#include "system.h"
#include "scrn.h"
#define map(i, j) (i * 80 + j)

/* These define our textpointer, our background and foreground
 *  colors (attributes), and x and y cursor coordinates */
unsigned short *textmemptr;
/* Make it easy to map text to memory */
unsigned short (*textmap)[25];
int attrib = 0x0F;

int offset_x = 2, offset_y= 3;
int csr_x = 0, csr_y = 0;
int current_line = 3;

// Main window buffer
unsigned short *buffer[200][78];

unsigned short *input_buff;                 // PTR to actual place in mem
unsigned char input_chars[75] = { 0 };   // Buffer to retrieve lower 8 bits
//
// char buffer[1000][80] = { 0 };
/* Scrolls the screen */
void scroll(void)
{
  unsigned temp;

  /* A blank is defined as a space... we need to give it
   *  backcolor too */
  unsigned blank = 0x20 | (attrib << 8);

  /* Row 25 is the end, this means we need to scroll up */
  if(offset_y >= 22)
  {
    /* Move the current text chunk that makes up the screen
     *  back in the buffer by a line */
    temp = offset_y % 200 - 22 + 1;
    memcpy (textmemptr, textmemptr + temp * 80, (22 - temp) * 80 * 2);

    /* Finally, we set the chunk of memory that occupies
     *  theklast line of text to our 'blank' character */
    memsetw (textmemptr + (22 - temp) * 80, blank, 80);
    offset_y = 22 - 1;
  }
  if (offset_y < 0)
  {
    /* Move the current text chunk that makes up the screen
     *  back in the buffer by a line */
    temp = offset_y + 25 + 1;
    memcpy (textmemptr, textmemptr + temp * 80, (25 - temp) * 80 * 2);

    /* Finally, we set the chunk of memory that occupies
     *  the last line of text to our 'blank' character */
    offset_y = 0;
  }
}

/* Updates the hardware cursor: the little blinking line
 *  on the screen under the last character pressed! */
void move_csr(void)
{
  unsigned temp;

  /* The equation for finding the index in a linear
   *  chunk of memory can be represented by:
   *  Index = [(y * width) + x] */
  temp = csr_y * 80 + csr_x;

  /* This sends a command to indicies 14 and 15 in the
   *  CRT Control Register of the VGA controller. These
   *  are the high and low bytes of the index that show
   *  where the hardware cursor is to be 'blinking'. To
   *  learn more, you should look up some VGA specific
   *  programming documents. A great start to graphics:
   *  http://www.brackeen.com/home/vga */
  outportb(0x3D4, 14);
  outportb(0x3D5, temp >> 8);
  outportb(0x3D4, 15);
  outportb(0x3D5, temp);
}

/* Clears the screen */
void cls()
{
  int i;
  unsigned blank = 0x20 | (attrib << 8);
  /* Sets the entire screen to spaces in our current
   *  color */
  for(i = 0; i < 25; i++)
    memsetw (textmemptr + i * 80, blank, 80);

  /* Update out virtual cursor, and then move the
   *  hardware cursor */
  csr_x = 0;
  csr_y = 0;
  move_csr();
}

/* Puts a single character on the screen */
void putch(unsigned char c)
{
  unsigned short *where;
  unsigned att = attrib << 8;

  /* Handle a backspace, by moving the cursor back one space */
  if(c == 0x08) {
    if(offset_x != 0) offset_x--;
  }
  /* Handles a tab by incrementing the cursor's x, but only
   *  to a point that will make it divisible by 8 */
  else if(c == 0x09) {
    offset_x = (offset_x + 8) & ~(8 - 1);
  }
  /* Handles a 'Carriage Return', which simply brings the
   *  cursor back to the margin */
  else if(c == '\r') {
    offset_x = 2;
  }
  /* We handle our newlines the way DOS and the BIOS do: we
   *  treat it as if a 'CR' was also there, so we bring the
   *  cursor to the margin and we increment the 'y' value */
  else if(c == '\n') {
    offset_x = 2;
    offset_y++;
  }
  /* Any character greater than and including a space, is a
   *  printable character. The equation for finding the index
   *  in a linear chunk of memory can be represented by:
   *  Index = [(y * width) + x] */
  else if(c >= ' ') {
    where = textmemptr + ((offset_y % 200) * 80 + offset_x % 200);
    *where = c | att;	/* Character AND attributes: color */
    buffer[offset_y % 200][offset_x] = *where;
    offset_x++;
  }

  /* If the cursor has reached the edge of the screen's width, we
   *  insert a new line in there */
  if(offset_x >= 79) {
    offset_x = 2;
    offset_y++;
  }

  /* Scroll the screen if needed, and finally move the cursor */
  scroll();
  //move_csr();
}

/* Uses the above routine to output a string... */
void puts(char *text)
{
  int i;
  for (i = 0; i < strlen(text); i++) {
    putch(text[i]);
  }
}

/* Takes in some arg, and appends new line char */

void printf(char *string, int arg)
{
  int k = 0;
  int len = strlen(string);

  // Print up to '%'
  while (string[k] != '%' && k < len) {
    putch(string[k++]);
  }

  // Inject converted arg & print
  char type = string[++k];
  char input[33];

  if (type == 'x') {
    puts("0x");
    itoa(arg, 16, input);
  }
  else if (type == 'b') {
    itoa(arg, 2, input);
  }
  else {
    itoa(arg, 10, input);
  }
  /* Converted */
  puts(input);
  /* Print rest */
  k++;
  while (string[k] != '\0') {
    putch(string[k++]);
  }
}

/* Ignore Negative Numbers */
void itoa(unsigned int num, unsigned int base, char *string)
{
  if (num == 0) {
    string[0] = '0'; string[1] = '\0';
    return string;
  }
  int len = 0;
  int i = 0;
  while (num != 0) {
    int rem = num % base;
    // If Remaineder > 9, must be hex;
    string[len] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
    num /= base;
    len += 1;
  }
  // Swap and NULL
  for (; i<len/2; i++) {
    string[i] ^= string[len-i-1];
    string[len-i-1] ^= string[i];
    string[i] ^= string[len-i-1];
  }
  string[len] = '\0';
}


/* Sets the forecolor and backcolor that we will use */
void settextcolor(unsigned char forecolor, unsigned char backcolor)
{
  /* Top 4 bytes are the background, bottom 4 bytes
   *  are the foreground color */
  attrib = (backcolor << 4) | (forecolor & 0x0F);
}

void line()
{
  puts("---------------------------\n");
}

void shift_left()
{
  int i;
  unsigned int *where;
  for (i = 0; i < 78 - csr_x; i++) {
    where = textmemptr + map(csr_y, csr_x + i);
    *where = *(textmemptr + map(csr_y, csr_x + i + 1)) | (attrib << 8);
  }
}

// void shift_right()
// {
//   int i;
//   unsigned int *where;
//   for (i = 77; i > csr_x; i--) {
//     where = textmemptr + map(csr_y, i);
//     *where = *(textmemptr + map(csr_y, i - 1)) | (attrib << 8);
//     count++;
//   }
// }
//

static void load_buff()
{
  int i;
  for (i = 0; i < 75; i++)
    input_chars[i] = input_buff[i];
}

void key_handler(char scancode, char val)
{
  // Delete char - Then shift over
  if (scancode == 0xE) {
    if (csr_x > 4) {           /* Start of Input */
      // Delete character and shift
      csr_x--;
      shift_left();
    }
  }
  // Move Cursor Up
  else if (scancode == 0x48) {
    if (csr_y > 2) csr_y--;
  }
  // Move Cursor Left
  else if (scancode == 0x4B) {
    if (csr_x > 4) csr_x--;
  }
  // Move Cursor Right
  else if (scancode == 0x4D) {
    if (csr_x < 78) csr_x++;
  }
  // Move Cursor Down
  else if (scancode == 0x50) {
    if (csr_y < 23) csr_y++;
  }
  // Backspace
  else if (scancode == 0x1C) {
    load_buff();
    // Clear current contents
    memsetw(textmemptr + map(1, 1), 0, 78);
    set_window_title(input_chars);
    return;
  }
  // Spacebar
  else if (scancode == 0x39) {
    // Should shift right..
    unsigned short att = attrib << 8;
    memsetw(textmemptr + map(csr_y, csr_x), ' ' | att, 1);
    csr_x++;
  }
  else if (val > ' ' && csr_x < 78) {
    unsigned short att = attrib << 8;
    memsetw(textmemptr + map(csr_y, csr_x), val | att, 1);
    csr_x++;
  }
  scroll();
  move_csr();
}

void set_window_title(unsigned char *msg)
{
  int i;
  int offset = 1;
  unsigned att = attrib << 8;
  unsigned int *where;
  for (i = offset; i <= strlen(msg); i++) {
    where = textmemptr + map(1, i + 1);
    *where = msg[i - offset] | att;
  }
}

void header()
{
  unsigned short att = attrib << 8;
  // Row 0
  memsetw(textmemptr + map(0,0), 201 | att, 1);
  memsetw(textmemptr + map(0,1), 205 | att, 78);
  memsetw(textmemptr + map(0,79), 187 | att, 1);
  // Row 1
  memsetw(textmemptr + map(1,0), 186 | att, 1);
  memsetw(textmemptr + map(1,79), 186 | att, 1);
  // Row 2
  memsetw(textmemptr + map(2,0), 204 | att, 1);
  memsetw(textmemptr + map(2,1), 205 | att, 78);
  memsetw(textmemptr + map(2,79), 185 | att, 1);
}

unsigned short color (unsigned short word, unsigned short forecolor, unsigned short backcolor)
{
  backcolor <<= 12;
  forecolor <<= 8;
  return backcolor | forecolor | word;
}

void body()
{
  unsigned short att = attrib << 8;
  int i;
  // Right Most Column
  for (i = 3; i < 22; i++)
    memsetw(textmemptr + map(i, 0), 186 | att, 1);

  // Right Most Column
  for (i = 3; i < 22; i++)
    memsetw(textmemptr + map(i, 79), 186 | att, 1);
}

void footer()
{
  unsigned short att = attrib << 8;
  // Body - Footer Divier
  memsetw(textmemptr + map(22,0), 199 | att, 1);
  memsetw(textmemptr + map(22,1), 196 | att, 78);
  memsetw(textmemptr + map(22,79), 182 | att, 1);
  // Second Last
  memsetw(textmemptr + map(23,0), 186 | att, 1);
  memsetw(textmemptr + map(23,79), 186 | att, 1);
  // Input Chevron
  memsetw(textmemptr + map(23, 2), color(175, 11, 0), 1);
  // Last Row
  memsetw(textmemptr + map(24,0), 200 | att, 1);
  memsetw(textmemptr + map(24,1), 205 | att, 78);
  memsetw(textmemptr + map(24,79), 188 | att, 1);
}

void window_install()
{
  header();
  body();
  footer();
  // Text Area
  csr_x = 4;
  csr_y = 23;

  move_csr();
}


/* Sets our text-mode VGA pointer, then clears the screen for us */
void init_video(void)
{
  textmemptr = (unsigned short *)0xB8000;
  input_buff = textmemptr + map(23, 4);
  textmap = (void *)textmemptr;
  cls();
}

void print(const char *msg)
{
    puts(msg);
}

void print_dec(unsigned int value)
{
    printf("%d", value);
}

void print_hex(unsigned int value)
{
    printf("%x", value);
}
