#!/bin/sh
export SDL_NOMOUSE=1
export HOME="/mnt/int_sd"
cd /mnt/int_sd/apps/smenu/
rm -f smenu_copy.elf
cp smenu.elf smenu_copy.elf
./smenu_copy.elf
