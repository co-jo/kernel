#include "system.h"
#include "scrn.h"

#define MAP(i, j) (i * 80 + j)
#define scrn_line(y) (y % 19)

#define DOWN 1
#define UP -1
#define TOP_BOUND 3
#define BOT_BOUND 22
#define LEFT_BOUND 2
#define RIGHT_BOUND 78
#define NUM_LINES 19
#define INPUT_START 4

#define BUFF_SIZE 1000
/* These define our textpointer, our background and foreground
 *  colors (attributes), and x and y cursor coordinates */
unsigned short *textmemptr;
unsigned short *inputmemptr;
/* Make it easy to map text to memory */
unsigned short (*textmap)[25];
int attrib = 0x0F;

// Where should we print next character (In relation to DMA)
int offset_x = LEFT_BOUND;
int offset_y = 0;
// The current position of our cursor
int csr_x = 0, csr_y = 0;
// Where should the top and bottom be if we are scrolling
int top = 0, bottom = 0;

unsigned int shifted = 0;
unsigned int user_scrolling = 0;

// Main window buffer
unsigned short *buffer[BUFF_SIZE][80] = { 0 };
unsigned char input_chars[75] = { 0 };   // Buffer to retrieve lower 8 bits

void paint(int start, int end)
{
  int i, j;
  for (j = TOP_BOUND; j < BOT_BOUND; j++) {
    for (i = LEFT_BOUND; i < RIGHT_BOUND; i++) {
      unsigned int *where = textmemptr + MAP(j, i);
      *where = buffer[start][i];
    }
    start++;
  }
}

// User scroll handler
void scroll(int direction)
{
  if (top + direction >= 0 && bottom + direction < offset_y) {
    top+=direction;
    bottom+=direction;
    paint(top, bottom);
  }
}

// Frame scroll - Kernel induced scrolling
void fscroll(void)
{
  // Scroll Down - When text is overflowing view
  if(offset_y >= NUM_LINES && csr_y > TOP_BOUND) {
    int start = (offset_y - NUM_LINES);
    paint(start, offset_y);
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

  if(c == 0x08) {
    if(offset_x != 0) offset_x--;
  }
  /* Handles a tab by incrementing the cursor's x, but only
   *  to a point that will make it divisible by 8 */
  else if(c == 0x09) {
    offset_x = (offset_x + 8) & ~(8 - 1);
  }
  else if(c == '\r') {
    offset_x = LEFT_BOUND;
  }
  else if(c == '\n') {
    offset_x = LEFT_BOUND;
    offset_y++;
  }
  else if(c >= ' ' || c == 22 || c == 28) {
    unsigned short line = scrn_line(offset_y);
    // Remove brackets = Bug (Adds to itself after returning)
    where = textmemptr + MAP((line + TOP_BOUND), offset_x);
    *where = c | att;	/* Character AND attributes: color */
    buffer[offset_y % BUFF_SIZE][offset_x] = *where;
    offset_x++;
  }

  if(offset_x >= RIGHT_BOUND) {
    offset_x = LEFT_BOUND;
    offset_y++;
  }

  fscroll();
  move_csr();
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
  for (i = 0; i < RIGHT_BOUND - csr_x; i++) {
    where = textmemptr + MAP(csr_y, csr_x + i);
    *where = *(textmemptr + MAP(csr_y, csr_x + i + 1)) | (attrib << 8);
  }
}

static void load_buff()
{
  int i;
  for (i = 0; i < 75; i++) {
    input_chars[i] = inputmemptr[i];
  }
  input_chars[csr_x - INPUT_START] = '\0';
}

void key_handler(unsigned short scancode, char val)
{
  // Delete char - Then shift over
  if (scancode == 0xE) {
    if (csr_x > INPUT_START) {           /* Start of Input */
      // Delete character and shift
      csr_x--;
      shift_left();
    }
  }
  // Move Cursor Up
  else if (scancode == 0x48) {
    if (csr_y > TOP_BOUND) {
      csr_y--;
    }
    else {
      // Start Scrolling
      if (!user_scrolling) {
        top = offset_y - NUM_LINES;
        bottom = offset_y - 1;
        user_scrolling = 1;
      }
      scroll(UP);
    }
  }
  // Move Cursor Left
  else if (scancode == 0x4B) {
    if (csr_x > INPUT_START) csr_x--;
  }
  // Move Cursor Right
  else if (scancode == 0x4D) {
    if (csr_x < RIGHT_BOUND) csr_x++;
  }
  // Move Cursor Down
  else if (scancode == 0x50) {
    if (csr_y <= BOT_BOUND) {
      csr_y++;
    }
    else {
      // Reached EOB
      if (bottom == offset_y || !user_scrolling) {
        user_scrolling = 0;
        return;
      }
      scroll(DOWN);
    }
  }
  // Enter Key
  else if (scancode == 0x1C) {
    unsigned blank = 0x20 | (attrib << 8);
    // Load characters typed into input_chars
    load_buff();
    // Clear current contents of header
    memsetw(textmemptr + MAP(1, 1), blank, 78);
    set_window_title(input_chars);

    exec_cmd(input_chars);
    // Clear contents of input buffer
    memsetw(inputmemptr, blank, 75);
    csr_x = INPUT_START;
  }
  // Spacebar
  else if (scancode == 0x39) {
    unsigned short att = attrib << 8;
    memsetw(textmemptr + MAP(csr_y, csr_x), ' ' | att, 1);
    csr_x++;
  }
  // Shit Modifier - Released
  else if (scancode == 0xAA) {
    shifted = 0;
    return;
  }
  // Shift Modifier - Pressed
  else if (scancode == 0x2A) {
    shifted = 1;
    return;
  }
  else if (csr_y == BOT_BOUND + 1 && csr_x < RIGHT_BOUND) {
    unsigned short att = attrib << 8;
    if (shifted) {
      val -= 32;
    }
    memsetw(textmemptr + MAP(csr_y, csr_x), val | att, 1);
    csr_x++;
  }

  move_csr();
}

void set_window_title(unsigned char *msg)
{
  int i;
  int offset = 1;
  unsigned blank = 0x20 | (attrib << 8);
  unsigned red = (12) << 8;
  unsigned int *where;
  // Clear
  memsetw(textmemptr + MAP(1,1), blank, 78);
  // Write
  for (i = offset; i <= strlen(msg); i++) {
    where = textmemptr + MAP(1, i + 1);
    *where = msg[i - offset] | red;
  }
}

void header()
{
  unsigned short att = attrib << 8;
  // Row 0
  memsetw(textmemptr + MAP(0,0), 201 | att, 1);
  memsetw(textmemptr + MAP(0,1), 205 | att, 78);
  memsetw(textmemptr + MAP(0,79), 187 | att, 1);
  // Row 1
  memsetw(textmemptr + MAP(1,0), 186 | att, 1);
  memsetw(textmemptr + MAP(1,79), 186 | att, 1);
  // Row 2
  memsetw(textmemptr + MAP(2,0), 204 | att, 1);
  memsetw(textmemptr + MAP(2,1), 205 | att, 78);
  memsetw(textmemptr + MAP(2,79), 185 | att, 1);
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
    memsetw(textmemptr + MAP(i, 0), 186 | att, 1);

  // Right Most Column
  for (i = 3; i < 22; i++)
    memsetw(textmemptr + MAP(i, 79), 186 | att, 1);
}

void footer()
{
  unsigned short att = attrib << 8;
  // Body - Footer Divier
  memsetw(textmemptr + MAP(22,0), 199 | att, 1);
  memsetw(textmemptr + MAP(22,1), 196 | att, 78);
  memsetw(textmemptr + MAP(22,79), 182 | att, 1);
  // Second Last
  memsetw(textmemptr + MAP(23,0), 186 | att, 1);
  memsetw(textmemptr + MAP(23,79), 186 | att, 1);
  // Input Chevron
  memsetw(textmemptr + MAP(23, 2), color(175, 11, 0), 1);
  // Last Row
  memsetw(textmemptr + MAP(24,0), 200 | att, 1);
  memsetw(textmemptr + MAP(24,1), 205 | att, 78);
  memsetw(textmemptr + MAP(24,79), 188 | att, 1);
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
  inputmemptr = textmemptr + MAP(23, 4);
  textmap = (void *)textmemptr;
  cls();
}
