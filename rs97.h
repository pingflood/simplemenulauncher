#ifndef _RS97_H
#define _RS97_H

#include <stdio.h>
#include <stdint.h>
#include <SDL/SDL.h>

#define VOLUME_MODE_MUTE 0
#define VOLUME_MODE_PHONES 1
#define VOLUME_MODE_NORMAL 2

#define UDC_REMOVE 0
#define UDC_CONNECT 1
#define UDC_ERROR 2

#define MMC_REMOVE 0
#define MMC_INSERT 1
#define MMC_ERROR 2

extern uint8_t tvout_enabled, sdcard_mount;
extern uint8_t button_time[], button_state[];

extern void HW_Init();
extern void HW_Deinit();
extern void SD_Mount();
extern void USB_Mount();
extern void TV_Out();
extern void USB_Mount_Loop();
extern int16_t getUDCStatus();
extern uint8_t Shutdown();
extern void Battery_Status();

extern void Increase_Backlight();
extern void SetCPU(uint32_t mhz);

extern void Unmount_all();

extern uint8_t prompt(uint8_t* text, uint8_t* yes_text, uint8_t* no_text);
extern uint8_t prompt_img(SDL_Surface* img_todraw);

#endif
