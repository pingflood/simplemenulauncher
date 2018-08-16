#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "font.h"

extern SDL_Surface *backbuffer, *screen, *img;
static uint32_t start = 0;

SDL_Surface* Load_Image(const int8_t* directory)
{
	SDL_Surface *tmp;
	tmp = IMG_Load(directory);
	return tmp;
}

void Put_image(SDL_Surface* tmp, int16_t x, int16_t y)
{
	SDL_Rect position;
	position.x = x;
	position.y = y;

	SDL_BlitSurface(tmp, NULL, backbuffer, &position);
}

void Put_sprite(SDL_Surface* tmp, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t f)
{
	SDL_Rect position;
	position.x = x;
	position.y = y;

	SDL_Rect frame;
	frame.x = f*w;
	frame.y = 0;
	frame.w = w;
	frame.h = h;

	SDL_BlitSurface(tmp, &frame, backbuffer, &position);
}

void Print_text(SDL_Surface *tmp, int32_t x, int32_t y, uint8_t *text_ex, uint16_t color, uint16_t size)
{
	int32_t i = 0;
	for (i=0;text_ex[i]!='\0';i++)
	{
		Put_sprite( tmp, x + ((size-5) * i), y, size, size, text_ex[i]-33);
	}
}

void Print_smalltext(SDL_Surface *tmp, int32_t x, int32_t y, uint8_t *text_ex, uint16_t color, uint16_t size)
{
	int32_t i = 0;
	for (i=0;text_ex[i]!='\0';i++)
	{
		Put_sprite( tmp, x + ((size-2) * i), y, size, size, text_ex[i]-33);
	}
}

void Draw_Rect(SDL_Surface* screen, int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color)
{
	SDL_LockSurface(screen);
	
	int32_t color_draw;
	SDL_Rect scr_draw;
	scr_draw.x = x;
	scr_draw.y = y;
	scr_draw.w = width;
	scr_draw.h = height;
	
	color_draw = color;
	
	SDL_FillRect(screen, &scr_draw, color_draw);
	
	SDL_UnlockSurface(screen);
}

void ScaleUp()
{
	#ifdef RS97
	/* We'll use memmove rather than SoftStretch for speed reasons */
	uint32_t *s = (uint32_t*) backbuffer->pixels;
	uint32_t *d = (uint32_t*) screen->pixels;
	for(uint8_t y = 0; y < 240; y++, s += 160, d += 320) 
		memmove(d, s, 1280);
	#else
	SDL_SoftStretch(backbuffer, NULL, screen, NULL);
	#endif
	SDL_Flip(screen);
	if((1000.0f/60.0f) > SDL_GetTicks()-start) SDL_Delay((1000.0f/60.0f)-(SDL_GetTicks()-start));	
	start = SDL_GetTicks();
}

void Display_Background()
{
	if (img) SDL_BlitSurface(img, NULL, backbuffer, NULL);
	else Draw_Rect(backbuffer, 0, 0, 320, 240, 0);
}
