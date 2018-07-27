/*
 * 
 * 
 * 
 * 
 * 
 *  File browser part 
 * 
 * 
 * 
 * 
 * 
 * 
 * */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "main.h"
#include "graphics.h"

extern SDL_Surface *screen, *backbuffer;
extern TTF_Font *gFont;
static SDL_Event gui_event;

extern struct file_struct apps[MAX_ELEMENTS], games[MAX_ELEMENTS], emus[MAX_ELEMENTS];
extern uint16_t emus_totalsize, games_totalsize, apps_totalsize;

extern uint8_t button_state[16], button_time[16];
extern int8_t* currentdir;
extern uint8_t *buf, *cwdbuf;

extern int32_t select_menu, list_menu;
extern uint8_t additional_file[MAX_NAME_SIZE];

/* 4096 just in case they use big rom sets... We may never know !*/
static int8_t file_name[4096][512];
static int16_t file_type[OUR_PATH_MAX];
static uint16_t fileid_selected = 0;
static uint16_t choice = 0;
static uint16_t scroll_choice = 0;
static uint8_t filemode_search = 0;
static int16_t numb_files = 0;

static void Draw_TTF_Text(int8_t* text, int16_t x, int16_t y)
{
	SDL_Surface* tmp;
	SDL_Rect position;
	SDL_Color fg;
	
	int8_t buffer_tmp[128];
	
	position.x = (int16_t)x;
	position.y = (int16_t)y;
	fg.r = 255;
	fg.g = 255;
	fg.b = 255;
	
	snprintf(buffer_tmp, sizeof(buffer_tmp), "%s", text);
	
	tmp = TTF_RenderUTF8_Solid(gFont, buffer_tmp, fg);
	SDL_BlitSurface(tmp, NULL, backbuffer, &position);
	if (tmp) SDL_FreeSurface(tmp);
}


void list_all_files(int8_t* directory, struct file_struct* example)
{
	DIR *dir;
	struct dirent *ent;
	uint8_t isit_a_directory;
	uint32_t i, e;
	int16_t pc;
	uint8_t present[2];
	int8_t* pch[3];
	
	/* Reset all the stored files to zero */
	for(i=0;i<4096;i++)
	{
		strncpy(file_name[i], "", 512);
	}
	
	i = 0;
	pc = -1;
	numb_files = 0;
	
	if ((dir = opendir (directory)) != NULL) 
	{
		while ( (ent = readdir (dir)) != NULL ) 
		{
			
			/* Add the .. string and then after, reject them all */
			
			if (i == 0)
			{
				strncpy(file_name[i], "..", 512);
				file_type[i] = TUR_C;
				i++;
				numb_files++;
			}
			
			pch[0] = strstr (ent->d_name, FORMAT_FILE);
			/* Reject these two signs and the executable itself */
			pch[1] = strstr (ent->d_name,"..");
			pch[2] = strstr (ent->d_name,EXECUTABLE_NAME);
			pc = strncmp (ent->d_name, ".", 2);
			
			/* Check if file in question is a folder */
			isit_a_directory = is_folder(ent->d_name);	
			present[0] = (pch[1] == NULL && pch[2] == NULL && pc != 0 ) ? 1 : 0;
			
			if (present[0] == 1)
			{
				present[1] = 0;
				/* Let's look for the extension in our file */
				for(e=0;e<example[list_menu+select_menu].howmuchext;e++)
				{
					if (strstr (ent->d_name, example[list_menu+select_menu].ext[e]) != NULL) 
					{
						present[1] = 1;
						break;
					}
				}
				
				if (isit_a_directory == 1 || present[1] == 1)
				{
						/* Copy string cotent from ent->d_name to file_name[i] */
						strncpy(file_name[i], ent->d_name, 512);
						
						if (present[1] == 1)
						{
							file_type[i] = BLUE_C;
						}
						else if (isit_a_directory == 1)
						{
							file_type[i] = F_C;
						}
						else
						{
							file_type[i] = WHITE_C;
						}

						i++;
						numb_files++;
				}
			}
			
		}
		closedir (dir);
	} 
	
	numb_files = numb_files - 1;
}


void clear_entirescreen()
{
	Draw_Rect(backbuffer, 0, 0, 320, 240, 0);
}

void update_entirescreen()
{
	SDL_SoftStretch(backbuffer, NULL, screen, NULL);
	SDL_Flip(screen);
}

void set_fileid()
{
	/* fileid_selected is the id of the file selected (first dimension of the file_name array) */
	fileid_selected = choice + (scroll_choice*12);
}

void goto_folder(struct file_struct* example)
{
	int8_t buf[MAX_LENGH];
	int8_t currentdir_temp[MAX_LENGH];
	snprintf(currentdir_temp, MAX_LENGH, "%s", currentdir);
	
	/* Refresh current directory, just to make sure */
	currentdir = getcwd(cwdbuf, MAX_LENGH);
	snprintf(currentdir, MAX_LENGH, "%s/%s", currentdir_temp, file_name[fileid_selected]);

	/* Set it to the current directory. */
	chdir(currentdir);
	/* Get current directory. */
	currentdir = getcwd(cwdbuf, MAX_LENGH);

	list_all_files(currentdir,example);
	refresh_cursor(2);
}

void refresh_cursor(uint8_t filemode)
{	
	/*
	 * 0: 
	 *  
	 * 3 : Refresh file list
	*/
	if (filemode == 2)
	{
		choice = 0;
		scroll_choice = 0;
		set_fileid();
		clear_entirescreen();
	}
	else if (filemode == 3 || filemode == 4)
	{
		clear_entirescreen();
	}
	
	/* Before drawing selection rectangle, let's clear the mess it left */
	if (choice > 0) Draw_Rect(backbuffer, 0, 40+(16*choice)-16, 320, 14, 0);
	if (choice < 11) Draw_Rect(backbuffer, 0, 40+(16*choice)+16, 320, 14, 0);
	
	/* Draw blue rectangle for selection*/
	Draw_Rect(backbuffer, 0, 40+(16*choice), 320, 14, 500);
	
	/* Draw all the files */
	draw_files_list();
	
	/* getcwd seems very unstable... Only draw it once */
	if (filemode > 0 && filemode < 4) 
	{
		Draw_TTF_Text(currentdir, 8, 16);
		switch(filemode_search)
		{
			case 0:
				Draw_TTF_Text(DEFAULT_TEXT, 96, 6);
				Draw_TTF_Text(INST_TEXT, 60, 28);
			break;
			case 1:
				Draw_TTF_Text(DEFAULT_TEXT_SEARCH, 96, 6);
				Draw_TTF_Text(INST_TEXT_SEARCH, 60, 28);
			break;
		}
	}
	update_entirescreen();
}


/* Is the path a folder ? */
uint8_t is_folder(int8_t* str1)
{
	struct stat st;
	uint8_t temp;
	
	temp = 0;
	
	if ( stat( str1, &st ) == 0 && S_ISDIR( st.st_mode ) ) 
	{
		temp = 1;
	}
	 
	return temp;
}


/*
	Draw the list of files on-screen (it is divided by the scroll_choice variable)
	For example, scroll_choice = 1 means that files from id 12 to 23 will be shown etc...
*/
void draw_files_list()
{
	uint8_t i;
	
	for (i = 0; i < 12; i++)
	{
		Draw_TTF_Text(file_name[i+(scroll_choice*12)], 48,40+(16*i));
	}
}


static void Controls_filebrowser()
{	
	static uint8_t state_b[2];
	static uint8_t time_b[2];
	uint8_t i;
	
	for(i=0;i<2;i++)
	{
		if (button_state[i] == 2)
		{
			time_b[i]++;
			if (time_b[i] > 3)
				state_b[i] = 1;
			if (time_b[i] > 4)
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
	
	/* If Up button is pressed down... (or Left button held) */
	if (button_state[0] == 1 || state_b[0] == 1)
	{
		if (choice > 0) 
		{
			choice--;
			refresh_cursor(0);
			set_fileid();
		}
		else if (scroll_choice > 0)
		{
			choice = 11;
			scroll_choice = scroll_choice - 1;
			refresh_cursor(3);
			set_fileid();
		}
	}
	/* If Down button is pressed down... (or Right button held) */
	else if (button_state[1] == 1 || state_b[1] == 1)
	{
		/* Don't let the user to scroll more than there are files... */
		if (fileid_selected < numb_files)
		{
			if (choice < 11) 
			{
				choice++;	
				refresh_cursor(0);
				set_fileid();
			}
			/* If the user wants to go down and there are more files, change the files to shown to the next one (thanks to scroll_choice) */
			else if (numb_files > 10)
			{
				scroll_choice = scroll_choice + 1;
				choice = 0;
				set_fileid();
				refresh_cursor(3);
			}
		}
	}
}



int32_t File_Browser_file(struct file_struct* example)
{
	/* Buffer to hold =>the current directory */
	int8_t buf[MAX_LENGH];
	uint8_t result;
	uint8_t file_chosen;
	
	button_state[6] = 0;
	button_state[8] = 0;
	refresh_cursor(3);
	
	while (button_state[6] < 1)
	{
		/* Call function for input */
		controls();
		Controls_filebrowser();
		if (button_state[6] == 1 || button_state[8] == 1)
		{
			button_state[6] = 0;
			file_chosen = 0;
			return 0;
		}
			
		/* If Control/Return button is pressed... */
		if (button_state[4]==1 || button_state[5]==1)
		{
			/* If is is a folder, then go to that folder */
			if (file_type[fileid_selected] == F_C || choice == 0) 
			{
				goto_folder(example);	
			}
			else
			{
				currentdir = getcwd(cwdbuf, MAX_LENGH);
				snprintf(additional_file, MAX_LENGH, "%s/%s", currentdir, file_name[fileid_selected]);
				file_chosen = 1;
				button_state[6] = 1;
				return 4;
			}
		}
		Limit_FPS();
	}
	
	return 0;
}
