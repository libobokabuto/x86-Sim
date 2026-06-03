#define _CRT_SECURE_NO_WARNINGS
#include "bochs.h"
#if BX_SUPPORT_CDROM

#include "cdrom.h"
#include "cdrom_win32.h"

#define LOG_THIS /* no SMF tricks here, not needed */

extern "C" {
#include <errno.h>
}

#if defined(WIN32)
// windows.h included by bochs.h
#include <winioctl.h>


static BOOL isWindowsXP;

#define BX_CD_FRAMESIZE 2048
#define CD_FRAMESIZE    2048

// READ_TOC_EX structure(s) and #defines

#define CDROM_READ_TOC_EX_FORMAT_TOC      0x00
#define CDROM_READ_TOC_EX_FORMAT_SESSION  0x01
#define CDROM_READ_TOC_EX_FORMAT_FULL_TOC 0x02
#define CDROM_READ_TOC_EX_FORMAT_PMA      0x03
#define CDROM_READ_TOC_EX_FORMAT_ATIP     0x04
#define CDROM_READ_TOC_EX_FORMAT_CDTEXT   0x05

#define IOCTL_CDROM_BASE              FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_READ_TOC_EX       CTL_CODE(IOCTL_CDROM_BASE, 0x0015, METHOD_BUFFERED, FILE_READ_ACCESS)
#ifndef IOCTL_DISK_GET_LENGTH_INFO
#define IOCTL_DISK_GET_LENGTH_INFO    CTL_CODE(IOCTL_DISK_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

typedef struct _CDROM_READ_TOC_EX {
    UCHAR Format : 4;
    UCHAR Reserved1 : 3; // future expansion
    UCHAR Msf : 1;
    UCHAR SessionTrack;
    UCHAR Reserved2;     // future expansion
    UCHAR Reserved3;     // future expansion
} CDROM_READ_TOC_EX, * PCDROM_READ_TOC_EX;

typedef struct _TRACK_DATA {
    UCHAR Reserved;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, * PTRACK_DATA;

typedef struct _CDROM_TOC_SESSION_DATA {
    // Header
    UCHAR Length[2];  // add two bytes for this field
    UCHAR FirstCompleteSession;
    UCHAR LastCompleteSession;
    // One track, representing the first track
    // of the last finished session
    TRACK_DATA TrackData[1];
} CDROM_TOC_SESSION_DATA, * PCDROM_TOC_SESSION_DATA;

// End READ_TOC_EX structure(s) and #defines

#include <stdio.h>

BOOL Is_WinXP_SP2_or_Later()
{
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;
    int op = VER_GREATER_EQUAL;

    // Initialize the OSVERSIONINFOEX structure.

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 5;
    osvi.dwMinorVersion = 1;
    osvi.wServicePackMajor = 2;
    osvi.wServicePackMinor = 0;

    // Initialize the condition mask.

    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, op);
    VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, op);

    // Perform the test.

    return VerifyVersionInfo(
        &osvi,
        VER_MAJORVERSION | VER_MINORVERSION |
        VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        dwlConditionMask);
}

cdrom_win32_c::cdrom_win32_c(const char* dev)
{
    char prefix[6];

    sprintf(prefix, "CD%d", ++bx_cdrom_count);
    //put(prefix);
    fd = -1; // File descriptor not yet allocated

    if (dev == NULL) {
        path = NULL;
    }
    else {
        path = strdup(dev);
    }
    using_file = 0;
    isWindowsXP = Is_WinXP_SP2_or_Later();
}

cdrom_win32_c::~cdrom_win32_c(void)
{
    if (fd >= 0) {
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        fd = -1;
    }
}



#endif /* WIN32 */
#endif /* if BX_SUPPORT_CDROM */
