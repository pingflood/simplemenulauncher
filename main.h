#ifndef _MAIN_H
#define _MAIN_H

#define MAX_LENGH 2048

#define TITLE_WINDOW "Simple Menu Launcher"
#define FORMAT_FILE ".elf"
#define EXECUTABLE_NAME "filebrws.elf"

#define DEFAULT_TEXT "Select a file to launch"
#define DEFAULT_TEXT_SEARCH "What file do you want to run with it ?"

#define INST_TEXT "[A] Start/Go to directory, [SELECT] Exit"
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

#define PAD_QUIT		keystate[SDLK_ESCAPE]

void init (void);
void controls ();

void refresh_cursor(uint8_t all);
void draw_files_list();

uint8_t is_folder(int8_t* str1);
void list_all_files(int8_t* directory);

void goto_folder();
void remove_file();
uint8_t clear_dir(int8_t* which_dir);

void set_fileid();

#endif
