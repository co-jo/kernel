#ifndef PANEL
#define PANEL

#define TR_CORNER
#define TL_CORNER
#define BR_CORNER
#define BL_CORNER
#define DLINE_H
#define DLINE_V

#define SLINE 196
#define SLINE_CONN_L 182
#define SLINE_CONN_R 199

#define DLINE 205
#define
#define DLINE_CONN_L

struct panel {
  unsigned short xcord;       /* Right now, just acts as read/write bounds */
  unsigned short ycord;       /* We draw borders in scrn.c - and must account for it */
  unsigned char cols;
  unsigned char rows;
  unsigned char csr_x;
  unsigned char csr_y;
  unsigned short *buffer;
  // Border Styles
  unsigned short *northb;
  unsigned short *southb;
  unsigned short *eastb;
  unsigned short *westb;
  // Header
  unsigned short *headerb;
  unsigned short *header_buff;
  // Footer / Input
  unsigned short *footerb;
  unsigned short *footer_buff;
} typedef panel_t;

panel_t *make();

#endif
