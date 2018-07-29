#!/bin/sh
export SDL_NOMOUSE=1
export HOME="/mnt/int_sd"
cd /mnt/int_sd/smenu/

FILESIZE=$(stat -c%s "./smenu.elf")
if [ $FILESIZE -lt 10 ]; then
	rm smenu.elf
	cp smenu_copy.elf smenu.elf
else
	var=`echo "SAFE!" | "./smenu.elf" 1`
	if [ ! -z "$var" ]; then
		rm smenu.elf
		cp smenu_copy.elf smenu.elf
	fi
fi
./smenu.elf

