#ifndef _BROWSER_H
#define _BROWSER_H

#include <stdint.h>

extern int32_t File_Browser_file(struct file_struct* example);
extern void list_all_files(int8_t* directory, struct file_struct* example);

#endif
