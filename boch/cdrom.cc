
#include "bochs.h"
#include "cdrom.h"

#include <stdio.h>

#define LOG_THIS /* no SMF tricks here, not needed */
#define BX_CD_FRAMESIZE 2048
unsigned int bx_cdrom_count = 0;