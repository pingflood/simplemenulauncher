#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL/SDL.h>


#ifdef RS97
//for soundcard
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <signal.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <fcntl.h> //for battery
//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif


#include "rs97.h"

extern SDL_Surface *screen, *backbuffer;

/* RS-97 specific things */
uint8_t tvout_enabled = 0, sdcard_mount = 0;
int32_t memdev = 0;
volatile uint32_t *memregs;

int16_t curMMCStatus, preMMCStatus;
int16_t getMMCStatus(void) {
	if (memdev > 0) return !(memregs[0x10500 >> 2] >> 0 & 0b1);
	return MMC_ERROR;
}

int32_t udcConnectedOnBoot;
int16_t getUDCStatus(void) {
	if (memdev > 0) return (memregs[0x10300 >> 2] >> 7 & 0b1);
	return UDC_ERROR;
}
int16_t tvOutPrev, tvOutConnected;
uint8_t getTVOutStatus() {
	if (memdev > 0) return !(memregs[0x10300 >> 2] >> 25 & 0b1);
	return 0;
}

int16_t volumeModePrev, volumeMode;
uint8_t getVolumeMode(uint8_t vol) {
	if (!vol) return VOLUME_MODE_MUTE;
	else if (memdev > 0 && !(memregs[0x10300 >> 2] >> 6 & 0b1)) return VOLUME_MODE_PHONES;
	return VOLUME_MODE_NORMAL;
}

void Init_Sound()
{
#ifdef RS97
	unsigned long soundDev = open("/dev/mixer", O_RDWR);
	int vol = (100 << 8) | 100;
	ioctl(soundDev, SOUND_MIXER_WRITE_VOLUME, &vol);
	close(soundDev);
#endif
}

void mountSd() {
	system("sleep 1; mount -t vfat -o rw,utf8 /dev/mmcblk$(( $(readlink /dev/root | head -c -3 | tail -c1) ^ 1 ))p1 /mnt/ext_sd");
}

void umountSd() {
	system("umount -fl /mnt/ext_sd");
}

/* SD Mount refers to mounting the external sd card. (GBA Slot on the RS-97)
 * It's a lot less problematic than the USB mounting but we still need to be careful.
 * */
void SD_Mount()
{
	if (getMMCStatus() == MMC_REMOVE)
	{
		if (sdcard_mount == 1)
		{
			umountSd();
		}
		else
		{
			return;
		}
	}
	
	if (getMMCStatus() == MMC_INSERT)
	{
		mountSd();
		sdcard_mount = 1;
	}
}

/* Right now, it is only executed at boot (because that's where it is the most reliable.
 * It might still launch again though... perhaps use an environment variable to prevent this ?
 *  */
void USB_Mount()
{
	uint8_t done;
	
	/* Do not prompt about USB if it's not plugged in */
	if (getUDCStatus() != UDC_CONNECT)
	{
		printf("getUDCStatus() %d (2=ERROR, 0=Non existant)\n", getUDCStatus());
		return;
	}
		
	done = prompt("MOUNT USB ?", "A BUTTON: YES", "B BUTTON: NO");
	
	/* Only go ahead if user asked to mount SD/USB*/
	if (done == 1)
	{
		/* This part (taken from GMenuNext) will mount the internal sd card. p3 is the partition id
		 * It used to be p4 on older releases and 97Next.
		 * */
		system("umount -fl /dev/mmcblk$(readlink /dev/root | head -c -3 | tail -c 1)p3");
		system("echo \"/dev/mmcblk$(readlink /dev/root | head -c -3 | tail -c 1)p3\" > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
		
		if (getMMCStatus() == MMC_INSERT) 
		{
			umountSd();
			system("echo '/dev/mmcblk$(( $(readlink /dev/root | head -c -3 | tail -c1) ^ 1 ))p1' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
		}
		
		USB_Mount_Loop();
		
		system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
		/* Note the vfat, this means that if you use ext3, you need to recompile it from source again
		 * Maybe i could allow setting this with an argv argument ?
		 * */
		system("mount /dev/mmcblk$(readlink /dev/root | head -c -3 | tail -c 1)p3 /mnt/int_sd -t vfat -o rw,utf8");
		
		if (getMMCStatus() == MMC_INSERT) 
		{
			system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
			mountSd();
		}
	}
}

void TV_Out()
{
	uint8_t done;
	
	/* Detect if TV Out is not plugged in */
	if (getTVOutStatus() == 0)
	{
		/* If yes, detect whetever it was enabled before.
		 * If not, then disable tv out and set value to 0.
		 * */
		if (tvout_enabled == 1)
		{
			system("echo 0 > /proc/jz/tvselect");
			tvout_enabled = 0;
			return;
		}
		/* If TVOut was not enabled previously and is not detected, then simply ignore */
		else
		{
			return;
		}
	}
	
	done = prompt("ENABLE TVOUT?", "A BUTTON: YES", "B BUTTON: NO");
	if (done == 1)
	{
		done = prompt("SELECT MODE", "A BUTTON: NTSC", "B BUTTON: PAL");
		system("echo 0 > /proc/jz/tvselect"); // always reset tv out
		switch(done)
		{
			case 1:
				system("echo 2 > /proc/jz/tvselect");
			break;
			case 2:
				system("echo 1 > /proc/jz/tvselect");
			break;
		}
	}
}


uint8_t Shutdown()
{
	uint8_t done;
	done = prompt("SHUTDOWN?", "A BUTTON: YES", "B BUTTON: NO");
	if (done == 1) return 3;
	else
	{
		done = prompt("REBOOT THEN?", "A BUTTON: YES", "B BUTTON: NO");
		if (done == 1) return 2;
	}
	
	return 0;
}
