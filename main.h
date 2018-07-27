#ifndef _MAIN_H
#define _MAIN_H

#define MAX_LENGH 512
#define OUR_PATH_MAX 512
#define MAX_NAME_SIZE 64
#define MAX_ELEMENTS 128

#define TITLE_WINDOW "Simple Menu Launcher"
#define FORMAT_FILE ".elf"
#define EXECUTABLE_NAME "filebrws.elf"

#define DEFAULT_TEXT "Select a file to launch"
#define DEFAULT_TEXT_SEARCH "What file do you want to run with it ?"

#define INST_TEXT "[A] Start/Go to directory, [B/SELECT] Exit"
#define INST_TEXT_SEARCH "[A] Choose file, [B] Cancel"

#define FILE_DELETED "File deleted !"

#define CONFIRM_DELETE_FOLDER "Delete this folder ?"
#define CONFIRM_DELETE_FILE "Delete this file ?"

#define CONFIRM_REBOOT "What do you want to do ?"

#define YES_CHOOSE "REBOOT"
#define NO_CHOOSE "SHUTDOWN"

#define REBOOT_CHOOSE "REBOOT"
#define POWEROFF_CHOOSE "SHUTDOWN"
#define PAD_UP			keystate[SDLK_UP]
#define PAD_DOWN		keystate[SDLK_DOWN]
#define PAD_LEFT		keystate[SDLK_LEFT]
#define PAD_RIGHT		keystate[SDLK_RIGHT]

#define PAD_CONFIRM		keystate[SDLK_LCTRL]
#define PAD_CANCEL		keystate[SDLK_LALT]
#define PAD_CONFIRM2	keystate[SDLK_RETURN]
#define PAD_DELETE		keystate[SDLK_BACKSPACE]
#define PAD_QUIT		keystate[SDLK_ESCAPE]

#define PAD_LEFT_SHOULDER		keystate[SDLK_TAB]
#define PAD_RIGHT_SHOULDER		keystate[SDLK_BACKSPACE]

#define PAD_BRIGHTNESS		keystate[SDLK_3]
#define PAD_HOME		keystate[SDLK_END]

#define BLUE_C  255
#define BLUE_C_SH  254
#define RED_C  1700
#define WHITE_C 65535
#define TUR_C 750
#define F_C  1023


#define COLOR_BG           	SDL_MapRGB(backbuffer->format,5,3,2)
#define COLOR_INACTIVE_ITEM SDL_MapRGB(backbuffer->format,255,255,255)
#define COLOR_SELECT       	SDL_MapRGB(backbuffer->format,0,0,255)


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

void init (void);
void controls ();

void refresh_cursor(uint8_t all);
void draw_files_list();

uint8_t is_folder(int8_t* str1);

void goto_folder();
void remove_file();
uint8_t clear_dir(int8_t* which_dir);

void set_fileid();

#endif
