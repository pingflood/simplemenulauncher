#!/bin/sh
export SDL_NOMOUSE=1
export HOME="/mnt/int_sd"
cd /mnt/int_sd/smenu/
var=`echo "SAFE!" | "./smenu.elf" 1`
if [ ! -z "$var" ]; then
	rm smenu.elf
	cp smenu_copy.elf smenu.elf
fi
./smenu.elf

