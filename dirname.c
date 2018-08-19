#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "main.h"
/* Code taken from Android's NDK */

/*
 * WHY DIRNAME DOESN'T WORK PROPERLY ON MUSL ????
 * HOW CAN YOU FUCK UP SOMETHING SO TRIVIAL ?
 * I'm forced to use this crap because the libc doesn't work on our platform.
 * (Or maybe our old kernel is to be blamed ?? I don't know and i don't have a UART to debug)
 * 
 * Btw, if your platform is not degenerate, then just use dirname (in lowercase).
 * It's possible that it doesn't like the way i parse my array.
 * But i don't see anything wrong with them ???
 * */
 
/* Missing stubs */

int dirname_r(const char*  path, char*  buffer, size_t  bufflen)
{
	int errno;
    const char *endp, *startp;
    int         result, len;

    /* Empty or NULL string gets treated as "." */
    if (path == NULL || *path == '\0') {
        startp = ".";
        len  = 1;
        goto Exit;
    }

    /* Strip trailing slashes */
    endp = path + strlen(path) - 1;
    while (endp > path && *endp == '/')
        endp--;

    /* Find the start of the dir */
    while (endp > path && *endp != '/')
        endp--;

    /* Either the dir is "/" or there are no slashes */
    if (endp == path) {
        startp = (*endp == '/') ? "/" : ".";
        len  = 1;
        goto Exit;
    }

    do {
        endp--;
    } while (endp > path && *endp == '/');

    startp = path;
    len = endp - startp +1;

Exit:
    result = len;
    if (len+1 > OUR_PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }
    if (buffer == NULL)
        return result;

    if (len > (int)bufflen-1) {
        len    = (int)bufflen-1;
        result = -1;
        errno  = ERANGE;
    }

    if (len >= 0) {
        memcpy( buffer, startp, len );
        buffer[len] = 0;
    }
    return result;
}

char* DirName(const char*  path) 
{
    static char*  bname = NULL;
    int           ret;

    if (bname == NULL) {
        bname = (char *)malloc(OUR_PATH_MAX);
        if (bname == NULL)
            return(NULL);
    }

    ret = dirname_r(path, bname, OUR_PATH_MAX);
    return (ret < 0) ? NULL : bname;
}
