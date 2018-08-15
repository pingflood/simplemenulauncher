#ifndef _TEXT_H
#define _TEXT_H

#include <stdint.h>

extern SDL_Surface* Load_Image(const char* directory);
extern void Put_sprite(SDL_Surface* tmp, short x, short y, unsigned short w, unsigned short h, unsigned char f);
extern void Print_text(SDL_Surface *tmp, int x, int y, char *text_ex, unsigned char color, unsigned char size);
extern void Put_image(SDL_Surface* tmp, short x, short y);
extern void Draw_Rect(SDL_Surface* screen, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color);
extern void Print_smalltext(SDL_Surface *tmp, int32_t x, int32_t y, uint8_t *text_ex, uint16_t color, uint16_t size);
extern void Limit_FPS();

#endif
