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
    max_xres = 640;
    max_yres = 480;
    x_tilesize = 16;
    y_tilesize = 24;
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
    if (snapshot_buffer != NULL) {
        delete[] snapshot_buffer;
    }
#if BX_USE_GUI_CONSOLE
    if (console.running) {
        //console_cleanup();  //自己注释，补2213指令的时候注释的，后面需要加上
    }
#endif
}

void bx_gui_c::init(int argc, char** argv, unsigned max_x, unsigned max_y,
    unsigned x_tile, unsigned y_tile)
{
    UNUSED(argc);
    UNUSED(argv);
    max_xres = max_x ? max_x : 640;
    max_yres = max_y ? max_y : 480;
    x_tilesize = x_tile ? x_tile : 16;
    y_tilesize = y_tile ? y_tile : 24;
    dimension_update(guest_xres, guest_yres, guest_fheight, guest_fwidth, guest_bpp);
}

void bx_gui_c::dimension_update(unsigned x, unsigned y, unsigned fheight,
    unsigned fwidth, unsigned bpp)
{
    Bit64u bytes;

    guest_xres = (Bit16u)(x ? x : 640);
    guest_yres = (Bit16u)(y ? y : 480);
    guest_fheight = (Bit8u)fheight;
    guest_fwidth = (Bit8u)fwidth;
    guest_bpp = (Bit8u)bpp;
    guest_textmode = (fheight != 0) && (fwidth != 0);

    bytes = (Bit64u)guest_xres * guest_yres * 4;
    if (bytes == 0) {
        bytes = (Bit64u)640 * 480 * 4;
    }
    delete[] framebuffer;
    framebuffer = new Bit8u[(size_t)bytes];
    memset(framebuffer, 0, (size_t)bytes);
}

void bx_gui_c::clear_screen(void)
{
    if (framebuffer != NULL) {
        memset(framebuffer, 0, (size_t)guest_xres * guest_yres * 4);
    }
}

bx_svga_tileinfo_t* bx_gui_c::graphics_tile_info(bx_svga_tileinfo_t* info)
{
    return graphics_tile_info_common(info);
}

bx_svga_tileinfo_t* bx_gui_c::graphics_tile_info_common(bx_svga_tileinfo_t* info)
{
    if (info == NULL) {
        return NULL;
    }
    info->bpp = 32;
    info->pitch = guest_xres * 4;
    info->red_shift = 16;
    info->green_shift = 8;
    info->blue_shift = 0;
    info->is_indexed = 0;
    info->is_little_endian = 1;
    info->red_mask = 0x00ff0000;
    info->green_mask = 0x0000ff00;
    info->blue_mask = 0x000000ff;
    info->snapshot_mode = snapshot_mode;
    return info;
}

Bit8u* bx_gui_c::graphics_tile_get(unsigned x, unsigned y, unsigned* w, unsigned* h)
{
    if (framebuffer == NULL) {
        dimension_update(guest_xres, guest_yres, guest_fheight, guest_fwidth, guest_bpp);
    }
    if ((x >= guest_xres) || (y >= guest_yres)) {
        if (w != NULL) {
            *w = 0;
        }
        if (h != NULL) {
            *h = 0;
        }
        return framebuffer;
    }
    if (w != NULL) {
        *w = (x + x_tilesize > guest_xres) ? (guest_xres - x) : x_tilesize;
    }
    if (h != NULL) {
        *h = (y + y_tilesize > guest_yres) ? (guest_yres - y) : y_tilesize;
    }
    return framebuffer + (((size_t)y * guest_xres + x) * 4);
}

void bx_gui_c::graphics_tile_update_common(Bit8u* tile, unsigned x, unsigned y)
{
    unsigned w, h;
    Bit8u* dst = graphics_tile_get(x * x_tilesize, y * y_tilesize, &w, &h);

    if ((tile == NULL) || (dst == NULL)) {
        return;
    }
    for (unsigned row = 0; row < h; row++) {
        memcpy(dst + ((size_t)row * guest_xres * 4),
            tile + ((size_t)row * x_tilesize * 4), (size_t)w * 4);
    }
}

void bx_gui_c::text_update_common(Bit8u* old_text, Bit8u* new_text,
    Bit16u cursor_address, bx_vga_tminfo_t* tm_info)
{
    UNUSED(cursor_address);
    UNUSED(tm_info);
    if ((old_text != NULL) && (new_text != NULL)) {
        memcpy(old_text, new_text, 0x20000);
    }
}

bool bx_gui_c::palette_change_common(Bit8u index, Bit8u red, Bit8u green, Bit8u blue)
{
    bool changed = (palette[index].red != red) ||
        (palette[index].green != green) ||
        (palette[index].blue != blue);

    palette[index].red = red;
    palette[index].green = green;
    palette[index].blue = blue;
    if (changed) {
        palette_change(index, red, green, blue);
    }
    return changed;
}

void bx_gui_c::get_capabilities(Bit16u* xres, Bit16u* yres, Bit16u* bpp)
{
    if (xres != NULL) {
        *xres = (Bit16u)max_xres;
    }
    if (yres != NULL) {
        *yres = (Bit16u)max_yres;
    }
    if (bpp != NULL) {
        *bpp = 32;
    }
}

void bx_gui_c::set_text_charmap(Bit8u map, Bit8u* fbuffer)
{
    if ((bx_gui != NULL) && (map < 2) && (fbuffer != NULL)) {
        memcpy(bx_gui->vga_charmap[map], fbuffer, 0x2000);
    }
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

void bx_gui_c::beep_on(float frequency)
{
    //BX_DEBUG(("GUI Beep ON (frequency=%.2f)", frequency));
}

void bx_gui_c::beep_off()
{
    //BX_DEBUG(("GUI Beep OFF"));
}
