#define _CRT_SECURE_NO_WARNINGS
#include "bochs.h"

#include "iodev.h"

#if BX_NETWORKING

#endif
#if BX_SUPPORT_SOUNDLOW
#include "soundmod.h"
#endif
#if BX_SUPPORT_PCIUSB
#include "usb_common.h"
#endif
#include "param_names.h"
#include <assert.h>

#include "debug.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#if defined(macintosh)
// Work around a bug in SDL 1.2.4 on MacOS X, which redefines getenv to
// SDL_getenv, but then neglects to provide SDL_getenv.  It happens
// because we are defining -Dmacintosh.
#undef getenv
#endif

const char** config_interface_list;
const char** display_library_list;
const char** vga_extension_names;
const char** vga_extension_plugins;
const char** pcislot_dev_list;
int bochsrc_include_level = 0;

#define LOG_THIS genlog->

extern bx_debug_t bx_dbg;

int bx_split_option_list(const char* msg, const char* rawopt, char** argv, int max_argv)
{//2405
    char* ptr, * ptr2, * tmpstr;
    int argc = 0, i;

    char* options = new char[strlen(rawopt) + 1];
    strcpy(options, rawopt);
    ptr = strtok(options, ",");
    while (ptr && strcmp(ptr, "none")) {
        if (argc < max_argv) {
            tmpstr = new char[strlen(ptr) + 1];
            strcpy(tmpstr, ptr);
            ptr2 = tmpstr;
            while (isspace(*ptr2)) ptr2++;
            i = (int)strlen(ptr2) - 1;
            while ((i >= 0) && isspace(ptr2[i])) {
                ptr2[i] = 0;
                i--;
            }
            if (strlen(ptr2) > 0) {
                argv[argc++] = strdup(ptr2);
            }
            delete[] tmpstr;
        }
        else {
            //BX_ERROR(("%s: too many parameters, max is %d", msg, max_argv));
        }
        ptr = strtok(NULL, ",");
    }
    delete[] options;
    return argc;
}

const char* get_builtin_variable(const char* varname)
{//1914
#ifdef WIN32
    int code;
    DWORD size;
    DWORD type = 0;
    HKEY hkey;
    char keyname[80];
    static char data[MAX_PATH];
#endif

    if (strlen(varname) < 1) return NULL;
    else {
        if (!strcmp(varname, "BXSHARE")) {
#ifdef WIN32
            wsprintf(keyname, "Software\\Bochs");
            code = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_READ, &hkey);
            if (code == ERROR_SUCCESS) {
                data[0] = 0;
                size = MAX_PATH;
                if (RegQueryValueEx(hkey, "BX_SHARE_PATH", NULL, (LPDWORD)&type, (LPBYTE)data,
                    (LPDWORD)&size) == ERROR_SUCCESS) {
                    RegCloseKey(hkey);
                    return data;
                }
                else if (RegQueryValueEx(hkey, "", NULL, (LPDWORD)&type, (LPBYTE)data,
                    (LPDWORD)&size) == ERROR_SUCCESS) {
                    RegCloseKey(hkey);
                    return data;
                }
                else {
                    RegCloseKey(hkey);
                    return NULL;
                }
            }
            else {
                return NULL;
            }
#else
            return BX_SHARE_PATH;
#endif
        }
        return NULL;
    }
}

void get_bxshare_path(char* path)
{//1957
    const char* varptr = NULL;

#if BX_HAVE_GETENV
    varptr = getenv("BXSHARE");
#endif
    if (varptr != NULL) {
        sprintf(path, "%s", varptr);
    }
    else {
        varptr = get_builtin_variable("BXSHARE");
        if (varptr != NULL) {
            sprintf(path, "%s", varptr);
        }
        else {
            strcpy(path, ".");
        }
    }
}
