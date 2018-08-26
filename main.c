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
SDL_Surface *img, *font_bmp, *font_bmp_small, *menu_icons, *power_bmp, *usb_bmp[2], *selector_bmp, *battery_icon;
SDL_Surface *help_gfx, *chip_bmp, *bar_bmp;
TTF_Font *gFont;

uint8_t button_time[15], button_state[15];

/* Global buffer used to store temporarely the text files, 
* i would like to avoid dealing with pointers as much as possible...
*/
uint8_t *buf, *cwdbuf;
/* Used to determine whetever we should shutdown, execute app etc..*/
uint8_t err;

/* used to store our selection in the menu's browser. It's set to 2 and -2 for the scrolling */
int32_t select_menu = 2, list_menu = -2;
uint8_t additional_file[MAX_NAME_SIZE];

int8_t* currentdir;
struct file_struct apps[MAX_ELEMENTS], games[MAX_ELEMENTS], emus[MAX_ELEMENTS], fav[MAX_ELEMENTS], walla[1];
/* They are not inside the structure to avoid duplication and memory waste */
uint16_t emus_totalsize, games_totalsize, apps_totalsize, fav_totalsize;

/* Select category, global because we need it for saves */
static uint8_t select_cat = 0;

struct settings mysettings;
const uint8_t default_wallpaperpath[] = "gfx/background.bmp";
uint8_t executable_directory[OUR_PATH_MAX];

uint32_t loop(uint8_t* name, uint32_t position_file_byte)
{
	uint32_t i, a;
	uint8_t tmp[OUR_PATH_MAX];
	
	for(a=position_file_byte;a<position_file_byte+MAX_NAME_SIZE;a++)
	{
		snprintf(tmp, MAX_NAME_SIZE, "%s", name);
		snprintf(name, MAX_NAME_SIZE, "%s%c", tmp, buf[a]);
		if (buf[a+1] == '\n')
		{
			position_file_byte = a + 2;
			break;
		}
	}

	return position_file_byte;
}

/* Using a local struct was prone to bugs for some reasons (?) so we'll pass everything through a pointer. */
void Fill_Element(int sz, struct file_struct* example, uint16_t* totalsize)
{
	uint32_t i, a, e;
	/* This is used to walk though the file, one entry at a time */
	uint32_t lastpos = 0;
	uint8_t tmp[OUR_PATH_MAX];
	uint32_t totalsz;
	
	/* We scroll through the text file and fill the array with names, description and all...
	 * Loop function stops reading when it meets the endline '\n'.
	 * The current position is parsed to lastpos, which is a pointer
	 * */
	for(e=0;e<MAX_ELEMENTS;e++)
	{
		if (lastpos >= sz) 
		{
			*totalsize = totalsz = e;
			break;
		}
		
		lastpos = loop(example[e].name, lastpos);
		lastpos = loop(example[e].description, lastpos);
		lastpos = loop(example[e].executable_path, lastpos);
		lastpos = loop(example[e].yes_search, lastpos);
		
		for(i=0;i<64;i++)
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
					i = 65;
					break;
				}
			}
		}

		lastpos = loop(example[e].mnt_path, lastpos);
		lastpos = loop(example[e].commandline, lastpos);
		lastpos = loop(example[e].icon_path, lastpos);
		
		example[e].icon = Load_Image(example[e].icon_path);
		
		lastpos = loop(example[e].clock_speed, lastpos);
		example[e].real_clock_speed = strtol (example[e].clock_speed, NULL, 10);
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
			fp = fopen("./apps.txt", "r");
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
			fp = fopen("./emus.txt", "r");
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
			fp = fopen("./games.txt", "r");
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
	
	/* If the file could not be loaded then just report that there are no apps */
	if (!fp)
	{

		totalsize = 0;
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
		Print_smalltext(font_bmp_small, 50,25+(39*i), structure_file[*listi+i].description, COLOR_INACTIVE_ITEM, 8);
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
		Display_Background();
		Draw_Rect(backbuffer, 56, 24, 224, 96, COLOR_SELECT);
		Print_text(font_bmp, 72,32, text, COLOR_INACTIVE_ITEM, 16);
		Print_text(font_bmp, 64,64, yes_text, COLOR_INACTIVE_ITEM, 16);
		Print_text(font_bmp, 64,96, no_text, COLOR_INACTIVE_ITEM, 16);
		if (button_state[4] == 1) done = 1;
		else if (button_state[8] == 1) done = 2;
		ScaleUp();
	}
	button_state[4] = 2;
	button_state[8] = 2;
	return done;
}

uint8_t prompt_img(SDL_Surface* img_todraw)
{
	uint8_t done;
	done = 0;
	/* Reset input values to 2 (aka HELD, just in case) */
	button_state[4] = 2;
	button_state[8] = 2;
	while (done == 0) 
	{
		controls();
		Display_Background();
		SDL_BlitSurface(img_todraw, NULL, backbuffer, NULL);
		/* If A button is pressed */
		if (button_state[4] == 1) done = 3;
		/* If B button is pressed */
		else if (button_state[8] == 1) done = 1;
		/* If X button is pressed */
		else if (button_state[13] == 1) done = 2;
		/* If Y button is pressed */
		else if (button_state[14] == 1) done = 5;
		ScaleUp();
	}
	
	button_state[4] = 2;
	button_state[8] = 2;
	return done;
}

void USB_Mount_Loop()
{
	uint8_t done = 1;
	/* Show USB graphics if loaded */
	if (usb_bmp[0] && usb_bmp[1])
	{
		while (done == 1) 
		{
			controls();
			Display_Background();
			SDL_BlitSurface(usb_bmp[1], NULL, backbuffer, NULL);
			if (button_state[5] == 1 || button_state[6] == 1 || button_state[8] == 1  || getUDCStatus() != UDC_CONNECT) done = 0;
			ScaleUp();
		}
	}
	/* Fall back to text if not */
	else
	{
		while (done == 1) 
		{
			controls();
			Display_Background();
			Draw_Rect(backbuffer, 56, 24, 224, 96, COLOR_SELECT);
			Print_text(font_bmp, 80,64, "USB MOUNTED", COLOR_INACTIVE_ITEM, 16);
			if (button_state[5] == 1 || button_state[6] == 1 || button_state[8] == 1  || getUDCStatus() != UDC_CONNECT) done = 0;
			ScaleUp();
		}
	}
}

/* This writes back the settings and changes done in memory to the file again.
 * Used for changing overclock settings. It should still be safer than GMenuNext/GMenu2x in any cases.
 * */
void Write_Settings(uint8_t category)
{
	uint16_t totalsize_ = 0;
	uint16_t e, i;
	struct file_struct* example;
	FILE* fp;
	
	HACK_CHDIR_MNT
	
	switch(category)
	{
		case 0:
			totalsize_ = apps_totalsize;
			example = apps;
			fp = fopen("apps.txt", "wb");
		break;
		case 1:
			totalsize_ = emus_totalsize;
			example = emus;
			fp = fopen("emus.txt", "wb");
		break;
		case 2:
			totalsize_ = games_totalsize;
			example = games;
			fp = fopen("games.txt", "wb");
		break;
	}
	
	for(e=0;e<totalsize_;e++)
	{
		fprintf(fp, "%s\n", example[e].name);
		fprintf(fp, "%s\n", example[e].description);
		fprintf(fp, "%s\n", example[e].executable_path);
		fprintf(fp, "%s\n", example[e].yes_search);
		for(i=0;i<example[e].howmuchext;i++)
		{
			fprintf(fp, "%s", example[e].ext[i]);
			if (i+1 < example[e].howmuchext) fprintf(fp, ",");
		}
		fprintf(fp, "\n%s\n", example[e].mnt_path);
		fprintf(fp, "\n%s\n", example[e].commandline);
		fprintf(fp, "%s\n", example[e].icon_path);
		fprintf(fp, "%d\n", example[e].real_clock_speed);
		fprintf(fp, "===\n");
	}
	fclose(fp);
}

void Write_AppSettings()
{
	FILE* fp;
	HACK_CHDIR_MNT
	fp = fopen("wallpaper.txt", "w");
	if (fp)
	{
		fprintf(fp, "%s", mysettings.wallpaper_path);
		fclose(fp);
	}
}

void Read_AppSettings()
{
	FILE* fp;
	SDL_Surface* tmp;
	HACK_CHDIR_MNT
	fp = fopen("wallpaper.txt", "r");

	if (fp)
	{
		/* FIXME : Surely there's a better way ? */
		fscanf (fp, "%s", mysettings.wallpaper_path);
		fclose(fp);
		
		tmp = Load_Image(mysettings.wallpaper_path);
		if (!tmp)
			snprintf(mysettings.wallpaper_path, sizeof(mysettings.wallpaper_path), "%s", default_wallpaperpath);
		else
			SDL_FreeSurface(tmp);
	}
	else
	{
		snprintf(mysettings.wallpaper_path, sizeof(mysettings.wallpaper_path), "%s", default_wallpaperpath);
	}
	Write_AppSettings();
}


void AppSettings_screen(uint8_t category, uint16_t list_numb)
{
	uint8_t done = 1;
	int8_t tmp_buf[10];
	uint16_t old_overclocking;
	struct file_struct* structure_file;
	
	switch(category)
	{
		case 0:
		structure_file = apps;
		break;
		case 1:
		structure_file = emus;
		break;
		case 2:
		structure_file = games;
		break;
	}
	
	old_overclocking = structure_file[list_numb].real_clock_speed;
	snprintf(tmp_buf, sizeof(tmp_buf), "%d Mhz", structure_file[list_numb].real_clock_speed);
	
	while (done == 1) 
	{
		controls();
		Display_Background();
		Put_image(structure_file[list_numb].icon, 8, 8);
		Print_text(font_bmp, 48, 12, structure_file[list_numb].name, COLOR_INACTIVE_ITEM, 16);
		
		Draw_Rect(backbuffer, 0, 48, 320, 39, 500);
		
		Put_image(chip_bmp, 8, 50);
		Print_text(font_bmp, 48, 56, "Clock Speed: ", COLOR_INACTIVE_ITEM, 16);
		Print_text(font_bmp, 208, 56, tmp_buf, COLOR_INACTIVE_ITEM, 16);

		if (button_state[3] == 1)
		{
			if (structure_file[list_numb].real_clock_speed < 642)
			{
				structure_file[list_numb].real_clock_speed += 6;
			}
			snprintf(tmp_buf, sizeof(tmp_buf), "%d Mhz", structure_file[list_numb].real_clock_speed);
		}
		
		if (button_state[2] == 1)
		{
			if (structure_file[list_numb].real_clock_speed > 528)
			{
				structure_file[list_numb].real_clock_speed -= 6;
			}
			snprintf(tmp_buf, sizeof(tmp_buf), "%d Mhz", structure_file[list_numb].real_clock_speed);
		}
		
		if (button_state[8] == 1) done = 0;
		ScaleUp();
	}
	
	/* Only write back overclocking settings if they changed */
	if (old_overclocking != structure_file[list_numb].real_clock_speed)
	{
		Write_Settings(category);
	}
}


void Help_Screen()
{
	uint8_t done = 1;
	/* Show USB graphics if loaded */
	while (done == 1) 
	{
		controls();
		Display_Background();
		SDL_BlitSurface(help_gfx, NULL, backbuffer, NULL);
		if (button_state[4] == 1 || button_state[8] == 1) done = 0;
		ScaleUp();
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

int32_t GlobalSettings_screen()
{
	int32_t err = 0;
	uint8_t done = 1;

	while (done == 1) 
	{
		controls();
		Display_Background();
		
		Put_image(bar_bmp, 0, 240-20);
			
		if (selector_bmp) Put_image(selector_bmp, 96 + (3 * 32), 199);
		else Draw_Rect(backbuffer, 96 + (3 * 32), 208, 32, 32, 1024);
			
		Put_image(menu_icons, 96, 208);
		
		Print_text(font_bmp, 48, 12, "Settings", COLOR_INACTIVE_ITEM, 16);
		
		Draw_Rect(backbuffer, 0, 48, 320, 39, 500);
		
		Put_image(chip_bmp, 8, 50);
		Print_text(font_bmp, 48, 56, "Wallpaper", COLOR_INACTIVE_ITEM, 16);
		//Print_smalltext(font_bmp_small, 160, 56, "/mnt/stuff", COLOR_INACTIVE_ITEM, 8);
		Print_smalltext(font_bmp_small, 48, 76, mysettings.wallpaper_path, COLOR_INACTIVE_ITEM, 8);
		
		Battery_Status();
		
		if (button_state[12] == 1) 
		{
			err = Shutdown();
			if (err == 2 || err == 3) done = 0;
		}
		
		if (button_state[4] == 1)
		{
			#ifdef RS97
			snprintf(currentdir, 512, "/mnt/");
			chdir(currentdir);
			#endif
			/* Refresh again */
			list_menu = 0;
			select_menu = 0;
			list_all_files(currentdir,walla);
			int32_t err = File_Browser_file(walla);	
			snprintf(mysettings.wallpaper_path, sizeof(mysettings.wallpaper_path), "%s", additional_file);
			Write_AppSettings();
			
			if (img) SDL_FreeSurface(img);
			img = Load_Image(mysettings.wallpaper_path);
		}
			
		if (button_state[5] == 1)
			USB_Mount();
				
		if (button_state[13] == 1)
			Help_Screen();
		
		if (button_state[9] == 1)
		{
			select_cat = 2;
			list_menu = -2;
			select_menu = 2;
			done = 0;	
		}
		else if (button_state[10] == 1) 
		{
			select_cat = 0;
			list_menu = -2;
			select_menu = 2;
			done = 0;	
		}

		ScaleUp();
	}
	
	return err;
}

/* 
 * This function makes sure to save where the user was left.
 * If it can't load said file then it will simply use the default. (O for all 3 variables)
*/

void Progress_RW(uint8_t mode)
{
	FILE* fp;
	HACK_CHDIR_MNT
	switch(mode)
	{
		/* Read progress from file */
		case 0:
			fp = fopen("./sv.sav", "rb");
			if (fp)
			{
				fread(&select_menu, sizeof(char), sizeof(select_menu), fp);
				fread(&list_menu, sizeof(char), sizeof(list_menu), fp);
				fread(&select_cat, sizeof(char), sizeof(select_cat), fp);
				fclose(fp);
			}
		break;
		/* Write progress to file */
		case 1:
			fp = fopen("./sv.sav", "wb");
			if (fp)
			{
				fwrite(&select_menu, sizeof(char), sizeof(select_menu), fp);
				fwrite(&list_menu, sizeof(char), sizeof(list_menu), fp);
				fwrite(&select_cat, sizeof(char), sizeof(select_cat), fp);
				fclose(fp);
			}
		break;
	}
	
}

void MenuBrowser()
{
	static uint8_t state_b[2];
	static uint8_t time_b[2];
	uint8_t temp_string[512];
	char yourpath[128];
	static uint16_t struct_totalsize = 0;
	uint8_t i;
	uint8_t done = 0;
	
	struct file_struct* structure_file;
	structure_file = SetMenu(select_cat, &struct_totalsize);
	
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
				}
				else
				{
					state_b[i] = 0;
					time_b[i] = 0;
				}
			}
					
			Display_Background();
			Draw_Rect(backbuffer, 0, select_menu*38, 320, 39, 500);
			Display_Files(&list_menu, structure_file);
			
			Put_image(bar_bmp, 0, 240-20);
			
			if (selector_bmp) Put_image(selector_bmp, 96 + (select_cat * 32), 199);
			else Draw_Rect(backbuffer, 96 + (select_cat * 32), 208, 32, 32, 1024);
			
			Put_image(menu_icons, 96, 208);
			
			Battery_Status();
			
			if (button_state[5] == 1)
				USB_Mount();
				
			if (button_state[13] == 1)
				Help_Screen();
				
			if (button_state[14] == 1)
				AppSettings_screen(select_cat, list_menu+select_menu);
			
			/* L shoulder */
			if (button_state[9] == 1)
			{
				if (select_cat > 0)
				{
					select_cat--;
					list_menu = -2;
					select_menu = 2;
					structure_file = SetMenu(select_cat, &struct_totalsize);
				}
				else if (select_cat == 0)
				{
					err = GlobalSettings_screen();
					structure_file = SetMenu(select_cat, &struct_totalsize);
					if (err == 2 || err == 3) done = 1;
				}
			}
			/* R shoulder */
			else if (button_state[10] == 1)
			{
				if (select_cat < 2)
				{
					select_cat++;
					list_menu = -2;
					select_menu = 2;
					structure_file = SetMenu(select_cat, &struct_totalsize);
				}
				else if (select_cat == 2)
				{
					err = GlobalSettings_screen();
					structure_file = SetMenu(select_cat, &struct_totalsize);
					if (err == 2 || err == 3) done = 1;
				}
			}
			

			if (button_state[0] == 1 || state_b[0] == 1) 
			{
				if ((list_menu)+(select_menu) > 0)
				{
					list_menu--;
					select_menu = 2;
				}
			}
			else if (button_state[1] == 1 || state_b[1] == 1) 
			{
				/* We also need to make sure to check if the next list also has games left before allowing to scroll */
				if ((list_menu+select_menu+1) < struct_totalsize)
				{
					list_menu++;
					select_menu = 2;
				}
			}
			
			if (button_state[2] == 1) 
			{
				if ((list_menu)+(select_menu)-4 > 0)
				{
					list_menu = list_menu - 4;
					select_menu = 2;
				}
				else
				{
					select_menu = 2;
					list_menu = -2;
				}
			}
			else if (button_state[3] == 1) 
			{
				if ((list_menu+select_menu+4) < struct_totalsize)
				{
					list_menu = list_menu + 4;
					select_menu = 2;
				}
				else
				{
					select_menu = 2;
					list_menu = struct_totalsize - 3;
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
					
			ScaleUp();
		}
		
		if (structure_file[select_menu+list_menu].yes_search[0] == 'y' && (err == 1 || err == 4))
		{
			/* User default user mount to /mnt */
			#ifdef RS97
			snprintf(currentdir, OUR_PATH_MAX, "/mnt/");
			chdir(currentdir);
			#endif
			/* Refresh again */
			list_all_files(currentdir,structure_file);
			err = File_Browser_file(structure_file);
		}
			
		if (err == 0) done = 0;
	}

	if (backbuffer != NULL) SDL_FreeSurface(backbuffer);
	if (screen != NULL) SDL_FreeSurface(screen);
	if (img != NULL) SDL_FreeSurface(img);
	if (usb_bmp[0] != NULL) SDL_FreeSurface(usb_bmp[0]);
	if (usb_bmp[1] != NULL) SDL_FreeSurface(usb_bmp[1]);
	if (font_bmp != NULL) SDL_FreeSurface(font_bmp);
	if (font_bmp_small != NULL) SDL_FreeSurface(font_bmp_small);
	if (power_bmp != NULL) SDL_FreeSurface(power_bmp);
	if (menu_icons != NULL) SDL_FreeSurface(menu_icons);
	if (selector_bmp != NULL) SDL_FreeSurface(selector_bmp);
	if (battery_icon != NULL) SDL_FreeSurface(battery_icon);
	if (help_gfx != NULL) SDL_FreeSurface(help_gfx);
	if (chip_bmp != NULL) SDL_FreeSurface(chip_bmp);
	if (bar_bmp != NULL) SDL_FreeSurface(bar_bmp);
	
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();

	HACK_CHDIR_MNT
	Progress_RW(1);
	
	HW_Deinit();

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
			SetCPU(structure_file[select_menu+list_menu].real_clock_speed);
			if (structure_file[select_menu+list_menu].commandline[0] == '#')
				snprintf(temp_string, sizeof(temp_string), "\"%s\" \"%s\"", structure_file[select_menu+list_menu].executable_path, structure_file[select_menu+list_menu].commandline);
			else
				snprintf(temp_string, sizeof(temp_string), "\"%s\"", structure_file[select_menu+list_menu].executable_path);
			execlp("/bin/sh", "/bin/sh", "-c", temp_string, NULL);
		break;
		/* Same, except that we also need to parse the additional file.
		 * Emulators are the most likely to make use of this, followed by game engines and apps like ffplay.
		 * */
		case 4:
			SetCPU(structure_file[select_menu+list_menu].real_clock_speed);
			if (structure_file[select_menu+list_menu].commandline[0] == '#')
				snprintf(temp_string, sizeof(temp_string), "\"%s\" \"%s\"", structure_file[select_menu+list_menu].executable_path, additional_file);
			else
				snprintf(temp_string, sizeof(temp_string), "\"%s\" \"%s\" \"%s\"", structure_file[select_menu+list_menu].executable_path, structure_file[select_menu+list_menu].commandline, additional_file);
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
	
	/* Parse our executable's current directory to our buffer. We can't use argv[0] for that due to relative paths */
	getcwd(executable_directory, sizeof(executable_directory));

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
	
	/* Set default extensions for Wallpaper */
	walla[0].howmuchext = 8;
	snprintf(walla[0].ext[0], MAX_NAME_SIZE, ".png");
	snprintf(walla[0].ext[1], MAX_NAME_SIZE, ".bmp");
	snprintf(walla[0].ext[2], MAX_NAME_SIZE, ".jpeg");
	snprintf(walla[0].ext[3], MAX_NAME_SIZE, ".jpg");
	snprintf(walla[0].ext[4], MAX_NAME_SIZE, ".PNG");
	snprintf(walla[0].ext[5], MAX_NAME_SIZE, ".BMP");
	snprintf(walla[0].ext[6], MAX_NAME_SIZE, ".JPEG");
	snprintf(walla[0].ext[7], MAX_NAME_SIZE, ".JPG");
	
	/* The RS-97 has a crappy screen of 320x480, but with an aspect ratio of 4:3.
	 * Thus, we need to render to a buffer and scale it before renderin to the screen.
	 * */
	#ifdef RS97
	screen = SDL_SetVideoMode(320, 480, 16, SDL_HWSURFACE); 
	#else
	screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE); 
	#endif
	backbuffer = SDL_CreateRGBSurface(SDL_HWSURFACE, 320, 240, 16, 0, 0, 0, 0);
	SDL_ShowCursor(SDL_DISABLE);
	
	Read_AppSettings();
	
	TTF_Init();
	gFont = TTF_OpenFont("gfx/font.ttf", 12 );
	TTF_SetFontStyle(gFont, TTF_STYLE_NORMAL);
	
	img = Load_Image(mysettings.wallpaper_path);
	menu_icons = Load_Image("gfx/menu_icons.png");
	font_bmp = Load_Image("gfx/font.png");
	font_bmp_small = Load_Image("gfx/font_small.png");
	power_bmp = Load_Image("gfx/power.png");
	
	usb_bmp[0] = Load_Image("gfx/usb_mount.png");
	usb_bmp[1] = Load_Image("gfx/usb_mount2.png");
	selector_bmp = Load_Image("gfx/selector.png");
	battery_icon = Load_Image("gfx/battery.png");
	
	help_gfx = Load_Image("gfx/help.png");
	chip_bmp = Load_Image("gfx/chip.png");
	bar_bmp = Load_Image("gfx/grey_bar.png");
	
	HW_Init();
	Increase_Backlight();
	
	apps_totalsize = Load_Files(0);
	emus_totalsize = Load_Files(1);
	games_totalsize = Load_Files(2);
	
	USB_Mount();
	
	currentdir = getcwd(cwdbuf, OUR_PATH_MAX);
	
	Progress_RW(0);

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
			case 13:
			pad = PAD_X;
			break;
			case 14:
			pad = PAD_Y;
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
				button_state[i] = 2;
				button_time[i] = 0;
			break;
			
			case 2:
				if (!(pad))
				{
					button_state[i] = 0;
					button_time[i] = 0;
				}
			break;
		}
		
	}

	/* HACCCCKKKK */
    SDL_Event event;
	while( SDL_PollEvent( &event ) )
	{
		switch( event.type )
		{
			case SDL_KEYDOWN:
			switch( event.key.keysym.sym )
			{
				case SDLK_UP:
					if (button_state[0] == 0) button_state[0] = 1;
				break;
				case SDLK_DOWN:
					if (button_state[0] == 1) button_state[1] = 1;
				break;
			}
			break;
		}
	}
}
