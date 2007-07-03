
#ifndef _RENDER_H_
#define _RENDER_H_

#ifdef PSP
#include "video.h"
#define MAKE_PIXEL(r,g,b)   RGB(r,g,b)
#else
/* Pack RGB data into a 16-bit RGB 5:6:5 format */
#define MAKE_PIXEL(r,g,b)   (((r << 8) & 0xF800) | ((g << 3) & 0x07E0) | ((b >> 3) & 0x001F))
#endif

/* Used for blanking a line in whole or in part */
#define BACKDROP_COLOR      (0x10 | (vdp.reg[7] & 0x0F))

/* Global data */
extern uint8 bg_name_dirty[0x200];
extern uint16 bg_name_list[0x200];
extern uint16 bg_list_index;
extern uint8 bg_pattern_cache[0x20000];

/* Function prototypes */
void render_init(void);
void render_shutdown(void);
void render_reset(void);

void render_line(int line);
void update_bg_pattern_cache(void);
void palette_sync(int index);
void remap_8_to_16(int line);

void render_bg_sms(int line);
void render_obj_sms(int line);

void render_bg_m0(int line);
void render_bg_m1(int line);
void render_bg_m2(int line);
void render_bg_m3(int line);
void render_bg_m4(int line);
void render_bg_m5(int line);
void render_bg_m6(int line);
void render_bg_m7(int line);

void render_obj_m0(int line);
void render_obj_m1(int line);
void render_obj_m2(int line);
void render_obj_m3(int line);

void set_tms_palette(void);

#endif /* _RENDER_H_ */
