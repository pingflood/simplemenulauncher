/*  
  COPYRIGHT (C) 2018, Gameblabla
  
        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
                    Version 2, December 2004 

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net> 

 Everyone is permitted to copy and distribute verbatim or modified 
 copies of this license document, and changing it is allowed as long 
 as the name is changed. 

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 

  0. You just DO WHAT THE FUCK YOU WANT TO.
*/

/*
 * INCOMING : HUGE MESS ! 
 * 
*/

/*
 * nFile Browser 
 * Simple file browser mainly targetting the TI nspire using SDL/n2DLib.
 * It can also be used on other platforms as a simple file browser.
 * On TI Nspire, it can launch Ndless applications. (only using the SDL port though)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <SDL/SDL.h>
#include "main.h"
#include "graphics.h"

#define OUR_PATH_MAX 512
#define MAX_NAME_SIZE 64
#define MAX_ELEMENTS 128

SDL_Surface *screen;
SDL_Surface *backbuffer;
SDL_Surface* img;
SDL_Surface* font_bmp;
SDL_Surface* font_bmp_small;

uint8_t button_time[20];
uint8_t button_state[20];
uint8_t* buf;
uint8_t err;

#define COLOR_BG           	SDL_MapRGB(backbuffer->format,5,3,2)
#define COLOR_INACTIVE_ITEM SDL_MapRGB(backbuffer->format,255,255,255)

struct file_struct
{
	uint8_t name[MAX_NAME_SIZE];
	uint8_t description[MAX_NAME_SIZE];
	uint8_t executable_path[OUR_PATH_MAX];
	uint8_t yes_search[2];
	uint8_t ext[16][MAX_NAME_SIZE];
	uint8_t howmuchext;
	uint8_t commandline[MAX_NAME_SIZE];
	uint8_t icon_path[OUR_PATH_MAX];
	uint8_t clock_speed[4];
	uint16_t real_clock_speed;
	SDL_Surface* icon;
} ;

struct file_struct apps[MAX_ELEMENTS];
struct file_struct games[MAX_ELEMENTS];
struct file_struct emus[MAX_ELEMENTS];
uint16_t emus_totalsize, games_totalsize, apps_totalsize;

uint8_t* loop(uint8_t* name, uint32_t* lastpos)
{
	uint32_t i, a;
	uint8_t tmp[32];
	for(a=*lastpos;a<*lastpos+MAX_NAME_SIZE;a++)
	{
		snprintf(tmp, MAX_NAME_SIZE, "%s", name);
		snprintf(name, MAX_NAME_SIZE, "%s%c", tmp, buf[a]);
		if (buf[a+1] == '\n')
		{
			*lastpos = a + 2;
			break;
		}
	}
	
	// 226 is € and it basically means "ignore" in our case
	if (name[0] == 'K') name[0] = '\0';

	return name;
}

uint8_t* extension(uint8_t* name, uint32_t* lastpos)
{
	uint32_t i, a;
	uint8_t tmp[32];
	for(a=*lastpos;a<*lastpos+MAX_NAME_SIZE;a++)
	{
		snprintf(tmp, MAX_NAME_SIZE, "%s", name);
		snprintf(name, MAX_NAME_SIZE, "%s%c", tmp, buf[a]);
		if (buf[a+1] == '\n')
		{
			*lastpos = a + 2;
			break;
		}
	}
	return name;
}

void Fill_Element(int sz, struct file_struct* tocopy, uint16_t* totalsize)
{
	uint32_t i, a, e;
	uint32_t lastpos = 0;
	uint8_t tmp[32];
	uint32_t totalsz;
	
	//struct file_struct* example = (struct file_struct*) emus;
	struct file_struct example[MAX_ELEMENTS];
	//struct file_struct* tocopy = (struct file_struct*) emus;
	
	for(e=0;e<MAX_ELEMENTS;e++)
	{
		if (lastpos >= sz) 
		{
			*totalsize = totalsz = e;
			printf("END OF FILE\n");
			break;
		}
		
		sprintf(example[e].name, "%s", loop(example[e].name, &lastpos));
		
		printf("Name %s\n", example[e].name);
		
		sprintf(example[e].description, "%s", loop(example[e].description, &lastpos));
		sprintf(example[e].executable_path, "%s", loop(example[e].executable_path, &lastpos));
		sprintf(example[e].yes_search, "%s", loop(example[e].yes_search, &lastpos));
		
		for(i=0;i<16;i++)
		{
			for(a=lastpos;a<lastpos+MAX_NAME_SIZE;a++)
			{
				snprintf(tmp, MAX_NAME_SIZE, "%s", example[e].ext[i]);
				snprintf(example[e].ext[i], MAX_NAME_SIZE, "%s%c", tmp, buf[a]);
				if (buf[a+1] == ',')
				{
					lastpos = a + 2;
					break;
				}
				else if (buf[a+1] == '\n')
				{
					lastpos = a + 2;
					example[e].howmuchext = i + 1;
					i = 17;
					break;
				}
			}
		}

		sprintf(example[e].commandline, "%s", loop(example[e].commandline, &lastpos));
		printf("example[e].commandline %d\n", example[e].commandline[0]);
		
		sprintf(example[e].icon_path, "%s", loop(example[e].icon_path, &lastpos));
		
		example[e].icon = Load_Image(example[e].icon_path);
		
		// Atoi alternative
		//example[e].real_clock_speed = atoi(loop(example[e].clock_speed, &lastpos));
		example[e].real_clock_speed = strtol (loop(example[e].clock_speed, &lastpos),NULL,10);
		lastpos += 4;
	}
	
	for(i=0;i<totalsz;i++)
	{
		sprintf(tocopy[i].name, "%s", example[i].name);
		sprintf(tocopy[i].description, "%s", example[i].description);
		sprintf(tocopy[i].executable_path, "%s", example[i].executable_path);
		sprintf(tocopy[i].yes_search, "%s", example[i].yes_search);
		
		for(e=0;e<16;e++)
		{
			snprintf(tocopy[i].ext[e], MAX_NAME_SIZE, "%s", example[i].ext[e]);
		}
		
		tocopy[i].howmuchext = example[i].howmuchext;
		
		sprintf(tocopy[i].commandline, "%s", example[i].commandline);
		sprintf(tocopy[i].icon_path, "%s", example[i].icon_path);
		
		tocopy[i].icon = example[i].icon;
		tocopy[i].howmuchext = example[i].howmuchext;
		
		// Atoi alternative
		//example[i].real_clock_speed = atoi(loop(example[i].clock_speed, &lastpos));
		tocopy[i].real_clock_speed  = example[i].real_clock_speed;
	}
}


int32_t main(int32_t argc, int8_t* argv[]) 
{
	FILE* fp;
	uint32_t sz;
	uint8_t tmp[32];
	uint8_t temp_string[512];
	uint8_t additional_file[512];
	uint32_t i, a, e;
	uint32_t lastpos = 0;
	int32_t select = 0;
	int32_t list = 0;
	
	SDL_Rect position;
	position.x = 0;
	position.y = 0;

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		SDL_Init(SDL_INIT_VIDEO);
	}
	screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE ); 
	backbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, 320, 240, 16, 0, 0, 0, 0);
	SDL_ShowCursor(SDL_DISABLE);
	
	img = Load_Image("background.bmp");
	font_bmp = Load_Image("font.png");
	font_bmp_small = Load_Image("font_small.png");
	
	SDL_WM_SetCaption(TITLE_WINDOW, NULL);
	
	fp = fopen("./emus", "r");
	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	buf = malloc(sz);
	fseek(fp, 0L, 0);
	fread(buf, sizeof(char), sz, fp);
	fclose(fp);
	
	Fill_Element(sz, emus, &emus_totalsize);
	
	for(i=0;i<emus_totalsize;i++)
	{
		printf("Name : %s\n", emus[i].name);
		printf("Description : %s\n", emus[i].description);
		printf("Executable Path : %s\n", emus[i].executable_path);
		printf("Yes search : %s\n", emus[i].yes_search);
		
		for(e=0;e<emus[i].howmuchext;e++)
		{
			printf("Extensions : %s\n", emus[i].ext[e]);
		}
			
		printf("Commandline : %s\n", emus[i].commandline);
		printf("Icon path : %s\n", emus[i].icon_path);
		printf("Clock speed : %d\n", emus[i].real_clock_speed);
	}
	
	while (button_state[6] == 0) 
	{
		controls();
		
		SDL_BlitSurface(img, NULL, backbuffer, &position);
		Draw_Rect(backbuffer, 0, select*38, 320, 39, 500);
			
		for(i=0;i<6;i++)
		{
			if (emus[list+i].icon) Put_image(emus[list+i].icon, 8, 4+(39*i));
			Print_text(font_bmp, 50,1+(39*i), emus[list+i].name, COLOR_INACTIVE_ITEM, 16);
			Print_text(font_bmp_small, 50,25+(39*i), emus[list+i].description, COLOR_INACTIVE_ITEM, 8);
		}
		
		if (button_state[0] == 1) 
		{
			if (select == 0 && list > 0)
			{
				list = list - 5;
				select = 5;
			}
			else if (select > 0) 
			{
				select--;
			}
		}
		else if (button_state[1]  == 1)
		{
			if (select > 4)
			{
				list = list + 5;
				select = 0;
			}
			else if (select < 5)
			{
				if (!((select+list)+2 > emus_totalsize))
				select++;
			}
		}
		
		if (button_state[4] == 1) 
		{
			button_state[6] = 1;
			err = 1;
		}
		
		SDL_SoftStretch(backbuffer, NULL, screen, NULL);
		SDL_Flip(screen);
		
		SDL_Delay(50);
		
	}


	if (backbuffer != NULL) SDL_FreeSurface(backbuffer);
	if (screen != NULL) SDL_FreeSurface(screen);
	if (img != NULL) SDL_FreeSurface(img);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();

	switch(err)
	{
		/* Shutdowns or reboot */
		case 2:
		case 3:
			if (err == 2)
			snprintf(temp_string, MAX_LENGH, "exec /sbin/reboot");
			else
			snprintf(temp_string, MAX_LENGH, "exec /sbin/poweroff");
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		break;
		/* Executes the executable */
		case 1:
			snprintf(temp_string, MAX_LENGH, "%s %s", emus[select+list].executable_path, emus[select+list].commandline);
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		break;
		/* Asks for file to execute */
		case 4:
			snprintf(temp_string, MAX_LENGH, "%s %s %s", emus[select+list].executable_path, emus[select+list].commandline , additional_file);
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		break;
	}

	return 0;
}

void controls()
{
    Uint8 *keystate = SDL_GetKeyState(NULL);
	int32_t pad = 0;
	uint8_t i;

	/*	Pressure buttons
	 *  0 means Inactive
	 *  1 means that the button was just pressed
	 *  2 means that the button is being held
	 *  3 means RELEASE THE BOGUS
	*/
	
	/*
	 * 0: UP
	 * 1: Down
	 * 2: Left
	 * 3: Right
	 * 4: Confirm
	 * 5: Confirm2
	 * 6: Quit
	*/

	for(i=0;i<sizeof(button_state);i++)
	{
		switch (i)
		{
			case 0:
			pad = PAD_UP;
			break;
			case 1:
			pad = PAD_DOWN;
			break;
			case 2:
			pad = PAD_LEFT;
			break;
			case 3:
			pad = PAD_RIGHT;
			break;
			case 4:
			pad = PAD_CONFIRM;
			break;
			case 5:
			pad = PAD_CONFIRM2;
			break;
			case 6:
			pad = PAD_QUIT;
			break;
			case 7:
			pad = PAD_DELETE;
			break;
			case 8:
			pad = PAD_CANCEL;
			break;
		}
		
		switch (button_state[i])
		{
			case 0:
				if (pad)
				{
					button_state[i] = 1;
					button_time[i] = 0;
				}
			break;
			
			case 1:
				button_time[i] = button_time[i] + 1;
				
				if (button_time[i] > 0)
				{
					button_state[i] = 2;
					button_time[i] = 0;
				}
			break;
			
			case 2:
				if (!(pad))
				{
					button_state[i] = 3;
					button_time[i] = 0;
				}
			break;
			
			case 3:
				button_time[i] = button_time[i] + 1;
				
				if (button_time[i] > 1)
				{
					button_state[i] = 0;
					button_time[i] = 0;
				}
			break;
		}
		
	}

    SDL_Event event;
    SDL_PollEvent(&event);
}