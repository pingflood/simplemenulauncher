#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "font.h"

extern SDL_Surface* backbuffer;

SDL_Surface* Load_Image(const int8_t* directory)
{
	SDL_Surface *tmp, *tmp2;
	tmp = IMG_Load(directory);
	
	//SDL_SetColorKey(tmp, (SDL_SRCCOLORKEY | SDL_RLEACCEL), SDL_MapRGB(tmp->format, 255, 0, 255));
	//tmp2 = SDL_DisplayFormat(tmp);
	//SDL_FreeSurface(tmp);

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
		Put_sprite( tmp, x + (size * i), y, size, size, text_ex[i]-33);
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