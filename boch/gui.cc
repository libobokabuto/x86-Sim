#define _CRT_SECURE_NO_WARNINGS	
#include <signal.h>
#include "iodev.h"

bx_gui_c* bx_gui = NULL;

#define BX_GUI_THIS bx_gui->
#define LOG_THIS BX_GUI_THIS

#define BX_KEY_UNKNOWN 0x7fffffff
#define N_USER_KEYS 38

typedef struct {
    const char* key;
    Bit32u symbol;
} user_key_t;

static user_key_t user_keys[N_USER_KEYS] =
{
  { "f1",    BX_KEY_F1 },
  { "f2",    BX_KEY_F2 },
  { "f3",    BX_KEY_F3 },
  { "f4",    BX_KEY_F4 },
  { "f5",    BX_KEY_F5 },
  { "f6",    BX_KEY_F6 },
  { "f7",    BX_KEY_F7 },
  { "f8",    BX_KEY_F8 },
  { "f9",    BX_KEY_F9 },
  { "f10",   BX_KEY_F10 },
  { "f11",   BX_KEY_F11 },
  { "f12",   BX_KEY_F12 },
  { "alt",   BX_KEY_ALT_L },
  { "bksl",  BX_KEY_BACKSLASH },
  { "bksp",  BX_KEY_BACKSPACE },
  { "ctrl",  BX_KEY_CTRL_L },
  { "del",   BX_KEY_DELETE },
  { "down",  BX_KEY_DOWN },
  { "end",   BX_KEY_END },
  { "enter", BX_KEY_ENTER },
  { "esc",   BX_KEY_ESC },
  { "home",  BX_KEY_HOME },
  { "ins",   BX_KEY_INSERT },
  { "left",  BX_KEY_LEFT },
  { "menu",  BX_KEY_MENU },
  { "minus", BX_KEY_MINUS },
  { "pgdwn", BX_KEY_PAGE_DOWN },
  { "pgup",  BX_KEY_PAGE_UP },
  { "plus",  BX_KEY_KP_ADD },
  { "right", BX_KEY_RIGHT },
  { "shift", BX_KEY_SHIFT_L },
  { "space", BX_KEY_SPACE },
  { "tab",   BX_KEY_TAB },
  { "up",    BX_KEY_UP },
  { "win",   BX_KEY_WIN_L },
  { "print", BX_KEY_PRINT },
  { "power", BX_KEY_POWER_POWER },
  { "scrlck", BX_KEY_SCRL_LOCK }
};

bx_gui_c::bx_gui_c(void) : disp_mode(DISP_MODE_SIM)
{
    //put("GUI"); // Init in specific_init
    bx_headerbar_entries = 0;
    statusitem_count = 0;
    led_timer_index = BX_NULL_TIMER_HANDLE;
    framebuffer = NULL;
    guest_textmode = 1;
    guest_fwidth = 8;
    guest_fheight = 16;
    guest_xres = 640;
    guest_yres = 480;
    guest_bpp = 8;
    snapshot_mode = 0;
    snapshot_buffer = NULL;
    command_mode.present = 0;
    command_mode.active = 0;
    marker_count = 0;
    memset(palette, 0, sizeof(palette));
    memset(vga_charmap[0], 0, 0x2000);
    memset(vga_charmap[1], 0, 0x2000);
    memset(&gui_opts, 0, sizeof(gui_opts));
}

bx_gui_c::~bx_gui_c()
{
    if (framebuffer != NULL) {
        delete[] framebuffer;
    }
#if BX_USE_GUI_CONSOLE
    if (console.running) {
        //console_cleanup();  //自己注释，补2213指令的时候注释的，后面需要加上
    }
#endif
}

int bx_gui_c::register_statusitem(const char* text, bool auto_off)
{ //974
    unsigned id = statusitem_count;

    for (unsigned i = 0; i < statusitem_count; i++) {
        if (!statusitem[i].in_use) {
            id = i;
            break;
        }
    }
    if (id == statusitem_count) {
        if (statusitem_count == BX_MAX_STATUSITEMS) {
            return -1;
        }
        else {
            statusitem_count++;
        }
    }
    statusitem[id].in_use = 1;
    strncpy(statusitem[id].text, text, 8);
    statusitem[id].text[7] = 0;
    statusitem[id].auto_off = auto_off;
    statusitem[id].counter = 0;
    statusitem[id].active = 0;
    statusitem[id].mode = 0;
    statusbar_setitem_specific(id, 0, 0);
    return id;
}

void bx_gui_c::statusbar_setitem(int element, bool active, bool w)
{
    if (element < 0) {
        for (unsigned i = 0; i < statusitem_count; i++) {
            statusbar_setitem_specific(i, 0, 0);
        }
    }
    else if ((unsigned)element < statusitem_count) {
        if ((active != statusitem[element].active) ||
            (w != statusitem[element].mode)) {
            statusbar_setitem_specific(element, active, w);
            statusitem[element].active = active;
            statusitem[element].mode = w;
        }
        if (active && statusitem[element].auto_off) {
            statusitem[element].counter = 5;
        }
    }
}