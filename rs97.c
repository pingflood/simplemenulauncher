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
#include <sys/mman.h>
#include <dirent.h>
#endif

#include "rs97.h"

extern SDL_Surface *screen, *backbuffer, *img, *power_bmp, *usb_bmp[2];

/* RS-97 specific things */
uint8_t tvout_enabled = 0, sdcard_mount = 0;
int32_t memdev = 0;
uint32_t backlight_v = 75;
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

void SetCPU(uint32_t mhz)
{
#ifdef RS97
	if (memdev > 0) 
	{
		uint32_t m = mhz / 6;
		memregs[0x10 >> 2] = (m << 24) | 0x090520;
	}
#endif
}

void HW_Init()
{
#ifdef RS97
	uint32_t soundDev = open("/dev/mixer", O_RDWR);
	int32_t vol = (100 << 8) | 100;
	
	/* Init memory registers */
	memdev = open("/dev/mem", O_RDWR);
	if (memdev > 0) 
	{
		memregs = (uint32_t*)mmap(0, 0x20000, PROT_READ | PROT_WRITE, MAP_SHARED, memdev, 0x10000000);
		if (memregs == MAP_FAILED) 
		{
			printf("Could not mmap hardware registers!\n");
			close(memdev);
		}
	}
	ioctl(soundDev, SOUND_MIXER_WRITE_VOLUME, &vol);
	close(soundDev);
	
	/* Set CPU clock to its default */
	SetCPU(528);
	
#endif
}

void HW_Deinit()
{
}


void Increase_Backlight()
{
	char buf[34] = {0};
	backlight_v = backlight_v + 25;
	/* 0 means that the screen is shut off, lowest backlight is 1 */
	if (backlight_v > 100) backlight_v = 1;
	
	sprintf(buf, "echo %d > /proc/jz/lcd_backlight", backlight_v);
	
	system(buf);
}

void mountSd()
{
	system("sleep 1; mount -t vfat -o rw,utf8 /dev/mmcblk$(( $(readlink /dev/root | head -c -3 | tail -c1) ^ 1 ))p1 /mnt/ext_sd");
}

void umountSd()
{
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
	
	if (getMMCStatus() == MMC_INSERT && sdcard_mount == 0)
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
		
	if (usb_bmp[0] && usb_bmp[1])
	{
		done = prompt_img(usb_bmp[0]);
		if (done == 3) done = 1;
		else done = 0;
	}
	else
	{
		done = prompt("MOUNT USB ?", "A BUTTON: YES", "B BUTTON: NO");
	}
	
	/* Only go ahead if user asked to mount SD/USB*/
	if (done == 1)
	{
		/* This part (taken from GMenuNext) will mount the internal sd card. p3 is the partition id
		 * It used to be p4 on older releases and 97Next.
		 * */
		system("umount -fl /dev/mmcblk$(readlink /dev/root | head -c -3 | tail -c 1)p3");
		/* Sleep for a while then file checking it after unmounting both partitions */
		system("sleep 1; /sbin/fsck -y $(readlink -f /dev/root | head -c -2)3");
		
		system("echo \"/dev/mmcblk$(readlink /dev/root | head -c -3 | tail -c 1)p3\" > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
		
		if (getMMCStatus() == MMC_INSERT) 
		{
			umountSd();
			system("echo '/dev/mmcblk$(( $(readlink /dev/root | head -c -3 | tail -c1) ^ 1 ))p1' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
			/* File system external SD card only if it exists */
			system("/sbin/fsck -y /dev/mmcblk$(( $(readlink /dev/root | head -c -3 | tail -c1) ^ 1 ))p1");
		}
		
		USB_Mount_Loop();
		
		/* File checking the internal sd card */
		system("/sbin/fsck -y $(readlink -f /dev/root | head -c -2)3");
		
		system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun0/file");
		/* Note the vfat, this means that if you use ext3, you need to recompile it from source again
		 * Maybe i could allow setting this with an argv argument ?
		 * */
		system("mount /dev/mmcblk$(readlink /dev/root | head -c -3 | tail -c 1)p3 /mnt/int_sd -t vfat -o rw,utf8");
		
		if (getMMCStatus() == MMC_INSERT) 
		{
			/* Now file check the sd card after unmounting */
			system("/sbin/fsck -y /dev/mmcblk$(( $(readlink /dev/root | head -c -3 | tail -c1) ^ 1 ))p1");
			system("echo '' > /sys/devices/platform/musb_hdrc.0/gadget/gadget-lun1/file");
			mountSd();
		}
		
		system("sync; sync; sync");
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
	
	if (tvout_enabled == 0)
	{
		done = prompt("ENABLE TVOUT?", "A BUTTON: YES", "B BUTTON: NO");
		if (done == 1)
		{
			done = prompt("SELECT MODE", "A BUTTON: PAL", "B BUTTON: NTSC");
			system("echo 0 > /proc/jz/tvselect"); // always reset tv out
			switch(done)
			{
				case 1:
					system("echo 2 > /proc/jz/tvselect");
					tvout_enabled = 1;
				break;
				case 2:
					system("echo 1 > /proc/jz/tvselect");
					tvout_enabled = 1;
				break;
			}
		}
	}
}

void Unmount_all()
{
	umountSd();
	system("umount -fl /dev/mmcblk$(readlink /dev/root | head -c -3 | tail -c 1)p3");
	system("echo 0 > /proc/jz/tvselect");
	system("/sbin/swapoff -a");
	system("sync; sync; sync");
	system("sleep 2");
}


void Suspend_Mode()
{
	SDL_Event event;
	uint8_t done = 1;
	uint8_t *keystate;
	
	#ifdef RS97
	char buf[34] = {0};
	system("echo 0 > /proc/jz/lcd_backlight");
	SetCPU(344);
	#endif
	
	while(done == 1)
	{
		keystate = SDL_GetKeyState(NULL);
		if (keystate[SDLK_END] || keystate[SDLK_3]) done = 0;
		SDL_PollEvent(&event);
		SDL_Delay(384);
	}
	
	/* Set brightness and CPU speed back */ 
	#ifdef RS97
	SetCPU(528);
	sprintf(buf, "echo %d > /proc/jz/lcd_backlight", backlight_v);
	system(buf);
	#endif
}

uint8_t Shutdown()
{
	uint8_t done;
	
	/* If you can't load Power button's image then fallback to text*/
	if (!power_bmp)
	{
		done = prompt("SHUTDOWN?", "A BUTTON: YES", "B BUTTON: NO");
		if (done == 1) return 3;
		else
		{
			done = prompt("REBOOT THEN?", "A BUTTON: YES", "B BUTTON: NO");
			if (done == 1) return 2;
		}
	}
	/* If not then show dem the improved menu */
	else
	{
		done = prompt_img(power_bmp);
		/* 1 means that they left the loop so ignore */
		if (done == 1) 
		{
			return 0;
		}
		/* Enter suspend mode */
		else if (done == 5) 
		{
			Suspend_Mode();
		}
		else return done;
	}
	
	return 0;
}
