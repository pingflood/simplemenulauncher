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

uint8_t button_time[20];
uint8_t button_state[20];
uint8_t* buf;

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

struct file_struct Fill_Element(struct file_struct example)
{
	uint32_t i, a;
	uint32_t lastpos = 0;
	uint8_t tmp[32];
	
	sprintf(example.name, "%s", loop(example.name, &lastpos));
	sprintf(example.description, "%s", loop(example.description, &lastpos));
	sprintf(example.executable_path, "%s", loop(example.executable_path, &lastpos));
	sprintf(example.yes_search, "%s", loop(example.yes_search, &lastpos));
	
	for(i=0;i<16;i++)
	{
		for(a=lastpos;a<lastpos+MAX_NAME_SIZE;a++)
		{
			snprintf(tmp, MAX_NAME_SIZE, "%s", example.ext[i]);
			snprintf(example.ext[i], MAX_NAME_SIZE, "%s%c", tmp, buf[a]);
			if (buf[a+1] == ',')
			{
				lastpos = a + 2;
				break;
			}
			else if (buf[a+1] == '\n')
			{
				lastpos = a + 2;
				example.howmuchext = i + 1;
				i = 16;
				break;
			}
		}
	}
	
	sprintf(example.commandline, "%s", loop(example.commandline, &lastpos));
	sprintf(example.icon_path, "%s", loop(example.icon_path, &lastpos));
	
	example.icon = Load_Image(example.icon_path);
	
	// Atoi alternative
	//example.real_clock_speed = atoi(loop(example.clock_speed, &lastpos));
	example.real_clock_speed = strtol (loop(example.clock_speed, &lastpos),NULL,10);
	
	return example;
}


int32_t main(int32_t argc, int8_t* argv[]) 
{
	FILE* fp;
	uint32_t sz;
	uint8_t tmp[32];
	uint32_t i, a;
	uint32_t lastpos = 0;
	
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
	
	SDL_WM_SetCaption(TITLE_WINDOW, NULL);
	
	fp = fopen("./emus", "r");
	fseek(fp, 0L, SEEK_END);
	sz = ftell(fp);
	buf = malloc(sz);
	fseek(fp, 0L, 0);
	fread(buf, sizeof(char), sz, fp);
	fclose(fp);
	
	emus[0] = Fill_Element(emus[0]);
	
	printf("Name : %s\n", emus[0].name);
	printf("Description : %s\n", emus[0].description);
	printf("Executable Path : %s\n", emus[0].executable_path);
	printf("Yes search : %s\n", emus[0].yes_search);
	
	for(i=0;i<emus[0].howmuchext;i++)
		printf("Extensions : %s\n", emus[0].ext[i]);
	printf("Commandline : %s\n", emus[0].commandline);
	printf("Icon path : %s\n", emus[0].icon_path);
	printf("Clock speed : %d\n", emus[0].real_clock_speed);
	
	while (button_state[6] == 0) 
	{
		controls();
		SDL_BlitSurface(img, NULL, backbuffer, &position);
		
		if (emus[0].icon) Put_image(emus[0].icon, 6, 4);
		
		Print_text(font_bmp, 48,0, emus[0].name, COLOR_INACTIVE_ITEM);
		Print_text(font_bmp, 48,24, emus[0].description, COLOR_INACTIVE_ITEM);
		
		SDL_SoftStretch(backbuffer, NULL, screen, NULL);
		SDL_Flip(screen);
	}


	if (backbuffer != NULL) SDL_FreeSurface(backbuffer);
	if (screen != NULL) SDL_FreeSurface(screen);
	if (img != NULL) SDL_FreeSurface(img);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();

	/*if (err == 3 || err == 4)
	{
		if (err == 4)
		{
			snprintf(temp_string, MAX_LENGH, "%s %s > ./log.txt", file_to_start, search_file);
			chdir(directory_start);
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		}
		else
		{
			snprintf(temp_string, MAX_LENGH, "%s > ./log.txt", file_to_start);
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		}
	}
	else if (err == 10 || err == 20)
	{
		if (err == 10)
			snprintf(temp_string, MAX_LENGH, "exec /sbin/reboot");
		else
			snprintf(temp_string, MAX_LENGH, "exec /sbin/poweroff");
		execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
	}
	
	*/
	
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
