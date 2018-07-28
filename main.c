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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "main.h"
#include "graphics.h"
#include "rs97.h"
#include "browser.h"
#include "dirname.h"

SDL_Surface *screen, *backbuffer;
SDL_Surface *img, *font_bmp, *font_bmp_small;
TTF_Font *gFont;

uint8_t button_time[20], button_state[20];

/* Global buffer used to store temporarely the text files, 
* i would like to avoid dealing with pointers as much as possible...
*/
uint8_t *buf, *cwdbuf;
/* Used to determine whetever we should shutdown, execute app etc..*/
uint8_t err;

/* used to store our selection in the menu's browser */
int32_t select_menu = 0, list_menu = 0;
uint8_t additional_file[MAX_NAME_SIZE];

int8_t* currentdir;
struct file_struct apps[MAX_ELEMENTS], games[MAX_ELEMENTS], emus[MAX_ELEMENTS];
/* They are not inside the structure to avoid duplication and memory waste */
uint16_t emus_totalsize, games_totalsize, apps_totalsize;


uint8_t* loop(uint8_t* name, uint32_t* lastpos)
{
	uint32_t i, a;
	uint8_t tmp[OUR_PATH_MAX];
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
	
	// Looks for big character 'K', if found then it means that there's nothing to copy
	if (name[0] == 'K') name[0] = '\0';

	return name;
}

/* Using a local struct was prone to bugs for some reasons (?) so we'll pass everything through a pointer. */
void Fill_Element(int sz, struct file_struct* example, uint16_t* totalsize)
{
	uint32_t i, a, e;
	/* This is used to walk though the file, one entry at a time */
	uint32_t lastpos = 0;
	uint8_t tmp[OUR_PATH_MAX];
	uint32_t totalsz;
	
	for(e=0;e<MAX_ELEMENTS;e++)
	{
		if (lastpos >= sz) 
		{
			*totalsize = totalsz = e;
			break;
		}
		
		sprintf(example[e].name, "%s", loop(example[e].name, &lastpos));
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
		sprintf(example[e].icon_path, "%s", loop(example[e].icon_path, &lastpos));
		
		example[e].icon = Load_Image(example[e].icon_path);
		
		// Atoi alternative
		//example[e].real_clock_speed = atoi(loop(example[e].clock_speed, &lastpos));
		example[e].real_clock_speed = strtol (loop(example[e].clock_speed, &lastpos),NULL,10);
		lastpos += 4;
	}
}

/* We probably need a better way than a hardcoded switch for only 3 categories 
 * TODO : Add failsafe in case it does not sucessfully load the file. (either because the file is invalid, write-only or non-existant)
 * In that case, we would need to load hardcoded ones (in a header file, const arrays of course) generated with each release.
 * At least, i won't get complains about "Y LAUNCHER BROKE AFTER APPLYING SERVICE PACK???" because that crap known as the RS-97
 * corrupted the file "en route". It should still allowing them to add external apps. (even though they are not recommended)
 * */
uint16_t Load_Files(uint8_t caca)
{
	FILE* fp;
	uint32_t sz = 0;
	uint16_t totalsize = 0;
	
	if (buf)
	{
		free(buf);
		buf = NULL;
	}
	
	switch(caca)
	{
		case 0:
			fp = fopen("./apps", "r");
			if (fp)
			{
				fseek(fp, 0L, SEEK_END);
				sz = ftell(fp);
				buf = malloc(sz);
				fseek(fp, 0L, 0);
				fread(buf, sizeof(uint8_t), sz, fp);
				fclose(fp);
				Fill_Element(sz, apps, &totalsize);
			}
		break;
		case 1:
			fp = fopen("./emus", "r");
			if (fp)
			{
				fseek(fp, 0L, SEEK_END);
				sz = ftell(fp);
				buf = malloc(sz);
				fseek(fp, 0L, 0);
				fread(buf, sizeof(uint8_t), sz, fp);
				fclose(fp);
				Fill_Element(sz, emus, &totalsize);
			}
		break;
		case 2:
			fp = fopen("./games", "r");
			if (fp)
			{
				fseek(fp, 0L, SEEK_END);
				sz = ftell(fp);
				buf = malloc(sz);
				fseek(fp, 0L, 0);
				fread(buf, sizeof(uint8_t), sz, fp);
				fclose(fp);
				Fill_Element(sz, games, &totalsize);
			}
		break;
	}

	return totalsize;
}

void Display_Files(int32_t* listi, struct file_struct* structure_file)
{
	uint8_t i;
	/* Our launcher is going to be list-based for now and we can only display 6 entries
	 * so let's not overdraw */
	for(i=0;i<6;i++)
	{
		if (structure_file[*listi+i].icon) Put_image(structure_file[*listi+i].icon, 8, 4+(38*i));
		Print_text(font_bmp, 50,1+(39*i), structure_file[*listi+i].name, COLOR_INACTIVE_ITEM, 16);
		Print_text(font_bmp_small, 50,25+(39*i), structure_file[*listi+i].description, COLOR_INACTIVE_ITEM, 8);
	}
}

uint8_t prompt(uint8_t* text, uint8_t* yes_text, uint8_t* no_text)
{
	uint8_t done;
	done = 0;
	/* Reset input values to 2 (aka HELD, just in case) */
	button_state[4] = 2;
	button_state[8] = 2;
	while (done == 0) 
	{
		controls();
		SDL_BlitSurface(img, NULL, backbuffer, NULL);
		Draw_Rect(backbuffer, 56, 24, 224, 96, COLOR_SELECT);
		Print_text(font_bmp, 72,32, text, COLOR_INACTIVE_ITEM, 16);
		Print_text(font_bmp, 64,64, yes_text, COLOR_INACTIVE_ITEM, 16);
		Print_text(font_bmp, 64,96, no_text, COLOR_INACTIVE_ITEM, 16);
		if (button_state[4] == 1) done = 1;
		else if (button_state[8] == 1) done = 2;
		SDL_SoftStretch(backbuffer, NULL, screen, NULL);
		SDL_Flip(screen);
	}
	button_state[4] = 2;
	button_state[8] = 2;
	return done;
}

void USB_Mount_Loop()
{
	uint8_t done = 1;
	while (done == 1) 
	{
		controls();
		SDL_BlitSurface(img, NULL, backbuffer, NULL);
		Draw_Rect(backbuffer, 56, 24, 224, 96, COLOR_SELECT);
		Print_text(font_bmp, 80,64, "USB MOUNTED", COLOR_INACTIVE_ITEM, 16);
		if (button_state[5] == 1 || button_state[6] == 1 || button_state[8] == 1  || getUDCStatus() != UDC_CONNECT) done = 0;
		SDL_SoftStretch(backbuffer, NULL, screen, NULL);
		SDL_Flip(screen);
	}
}

struct file_struct* SetMenu(uint8_t cc, uint16_t* tot)
{
	switch(cc)
	{
		case 0:
		default:
			*tot = apps_totalsize;
			return apps;
		break;
		case 1:
			*tot = emus_totalsize;
			return emus;
		break;
		case 2:
			*tot = games_totalsize;
			return games;
		break;
	}
}

void Backlight_control()
{
	if (button_state[11] == 1)
		Increase_Backlight();
}

void MenuBrowser()
{
	static uint8_t state_b[2];
	static uint8_t time_b[2];
	uint8_t temp_string[512];
	char yourpath[128];
	static uint8_t select_cat = 0;
	static uint16_t struct_totalsize = 0;
	uint8_t i;
	uint8_t done = 0;
	
	struct file_struct* structure_file;
	structure_file = SetMenu(0, &struct_totalsize);
	
	list_all_files(currentdir,structure_file);
	
	while (done == 0) 
	{
		while (done == 0) 
		{
			controls();
					
			TV_Out();
			SD_Mount();
					
			/* Hopefully this won't make it fly...*/
			for(i=0;i<2;i++)
			{
				if (button_state[i] == 2)
				{
					time_b[i]++;
					if (time_b[i] > 3)
						state_b[i] = 1;
					if (time_b[i] > 10)
					{
						state_b[i] = 0;
						time_b[i] = 0;
					}
				}
				else
				{
					state_b[i] = 0;
					time_b[i] = 0;
				}
			}
					
			SDL_BlitSurface(img, NULL, backbuffer, NULL);
			Draw_Rect(backbuffer, 0, select_menu*38, 320, 39, 500);
			Display_Files(&list_menu, structure_file);
			
			if (button_state[5] == 1)
				USB_Mount();
			
			/* L shoulder */
			if (button_state[9] == 1)
			{
				if (select_cat > 0) select_cat--;
				list_menu = 0;
				select_menu = 0;
				structure_file = SetMenu(select_cat, &struct_totalsize);
			}
			/* R shoulder */
			else if (button_state[10] == 1)
			{
				if (select_cat < 2) select_cat++;
				list_menu = 0;
				select_menu = 0;
				structure_file = SetMenu(select_cat, &struct_totalsize);
			}
					
			if (button_state[0] == 1 || state_b[0] == 1) 
			{
				if (select_menu == 0 && list_menu > 0)
				{
					list_menu = list_menu - 6;
					select_menu = 5;
				}
				else if (select_menu > 0) 
				{
					select_menu--;
				}
			}
			else if (button_state[1] == 1 || state_b[1] == 1) 
			{
				if (select_menu > 4)
				{
					list_menu = list_menu + 6;
					select_menu = 0;
				}
				else if (select_menu < 5)
				{
					if (!((select_menu+list_menu)+2 > struct_totalsize))
					select_menu++;
				}
			}
			
			if (button_state[2] == 1) 
			{
				if (list_menu > 0)
				{
					list_menu = list_menu - 6;
					select_menu = 0;
				}
			}
			else if (button_state[3] == 1) 
			{
				if (!((list_menu)+6 > struct_totalsize))
				{
					list_menu = list_menu + 6;
					select_menu = 0;
				}
			}
			
			Backlight_control();
					
			if (button_state[4] == 1) 
			{
				done = 1;
				err = 1;
			}
					
			/* Shutdown */
			if (button_state[12] == 1) 
			{
				err = Shutdown();
				if (err == 2 || err == 3) done = 1;
			}
					
			SDL_SoftStretch(backbuffer, NULL, screen, NULL);
			SDL_Flip(screen);
			Limit_FPS();
		}
		
		if (emus[select_menu+list_menu].yes_search[0] == 'y' && (err == 1 || err == 4))
		{
			/* Refresh again */
			list_all_files(currentdir,structure_file);
			err = File_Browser_file(structure_file);
		}
			
		if (err == 0) done = 0;
	}

	if (backbuffer != NULL) SDL_FreeSurface(backbuffer);
	if (screen != NULL) SDL_FreeSurface(screen);
	if (img != NULL) SDL_FreeSurface(img);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();

	if (err == 1 || err == 4)
		chdir(DirName(structure_file[select_menu+list_menu].executable_path));

	switch(err)
	{
		/* Shutdowns or reboot */
		case 2:
		case 3:
			/* Sync before rebooting or shuting down.
			 * For some reasons, it will sometimes refuse to shutdown or reboot...
			 * Since we are forcing the shutdown, we need to make sure to unmount all mount points
			 * and sync 3 times at least for safety.
			 * */
			Unmount_all();
			if (err == 2)
			snprintf(temp_string, sizeof(temp_string), "exec /sbin/reboot -f");
			else
			snprintf(temp_string, sizeof(temp_string), "exec /sbin/poweroff -f");
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		break;
		/* Launch the executable 
		 * It's important that we also need to parse the command line switch.
		 * Sure, we could have used an sh file for that but they like being corrupted too...
		 * They should be avoided whetever possible. This one is most suited for games.
		 * */
		case 1:
			//snprintf(yourpath, sizeof(yourpath), "%s", DirName(structure_file[select_menu+list_menu].executable_path));
			SetCPU(structure_file[select_menu+list_menu].real_clock_speed);
			snprintf(temp_string, sizeof(temp_string), "%s %s", structure_file[select_menu+list_menu].executable_path, structure_file[select_menu+list_menu].commandline);
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		break;
		/* Same, except that we also need to parse the additional file.
		 * Emulators are the most likely to make use of this, followed by game engines and apps like ffplay.
		 * */
		case 4:
			SetCPU(structure_file[select_menu+list_menu].real_clock_speed);
			snprintf(temp_string, sizeof(temp_string), "%s %s \"%s\"", structure_file[select_menu+list_menu].executable_path, structure_file[select_menu+list_menu].commandline, additional_file);
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		break;
	}
	
}


int32_t main(int32_t argc, int8_t* argv[]) 
{
	uint8_t tmp[32];
	uint32_t i, a, e;
	uint32_t lastpos = 0;
	
	/* Safety check for the sh script autoexec.sh.
	 * SimpleMenuLauncher should be started without any arguments.
	 * */
	if (argc == 2)
	{
		printf("SAFE!\n");
		return 0;
	}

	/* Now, these environment variables are supposed to be set through sh.
	 * But what happens if those lines get removed or are incorrectly modified ? Well you're screwed, simple.
	 * Because without those, most apps won't work, such as FBA GCW0 for example.
	 * There are 7 billions of humans on earth, one of them is going to screw the file and ask why it broke.
	 * */
	setenv("SDL_NOMOUSE", "1", 1);
	setenv("HOME", "/mnt/int_sd", 1);

	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		SDL_Init(SDL_INIT_VIDEO);
	}
	
	/* The RS-97 has a crappy screen of 320x480, but with an aspect ratio of 4:3.
	 * Thus, we need to render to a buffer and scale it before renderin to the screen.
	 * */
	screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE ); 
	backbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, 320, 240, 16, 0, 0, 0, 0);
	SDL_ShowCursor(SDL_DISABLE);
	
	TTF_Init();
	gFont = TTF_OpenFont("font.ttf", 12 );
	TTF_SetFontStyle(gFont, TTF_STYLE_NORMAL);
	
	img = Load_Image("background.bmp");
	font_bmp = Load_Image("font.png");
	font_bmp_small = Load_Image("font_small.png");
	
	HW_Init();
	Increase_Backlight();
	
	apps_totalsize = Load_Files(0);
	emus_totalsize = Load_Files(1);
	games_totalsize = Load_Files(2);
	
	USB_Mount();
	
	currentdir = getcwd(cwdbuf, 512);
	
	MenuBrowser();


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
			case 9:
			pad = PAD_LEFT_SHOULDER;
			break;
			case 10:
			pad = PAD_RIGHT_SHOULDER;
			break;
			case 11:
			pad = PAD_BRIGHTNESS;
			break;
			case 12:
			pad = PAD_HOME;
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
