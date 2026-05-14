#pragma once
#include "siminterface.h"
#define BX_MAX_STATUSITEMS 10 //41
typedef struct {
	Bit16u  start_address;
	Bit8u   cs_start;
	Bit8u   cs_end;
	Bit16u  line_offset;
	Bit16u  line_compare;
	Bit8u   h_panning;
	Bit8u   v_panning;
	bool line_graphics;
	bool split_hpanning;
	Bit8u   blink_flags;
	Bit8u   actl_palette[16];
} bx_vga_tminfo_t;

BOCHSAPI extern class bx_gui_c* bx_gui;  //116

class BOCHSAPI bx_gui_c {
public: //127
	bx_gui_c(void);
	virtual ~bx_gui_c();
	virtual void statusbar_setitem_specific(int element, bool active, bool w) {} //160
	int register_statusitem(const char* text, bool auto_off = 0); //209
	void statusbar_setitem(int element, bool active, bool w = 0); //211
protected: //247
	unsigned bx_headerbar_entries; //300
	Bit8u vga_charmap[2][0x2000]; //310
	unsigned statusitem_count;//314
	int led_timer_index; //315
	struct {
		bool in_use;
		char text[8];
		bool active;
		bool mode; // read/write
		bool auto_off;
		Bit8u counter;
	} statusitem[BX_MAX_STATUSITEMS];
	disp_mode_t disp_mode; //325
	Bit8u* framebuffer; //332
	bool guest_textmode; //343
	Bit8u   guest_fwidth;
	Bit8u   guest_fheight;
	Bit16u  guest_xres;
	Bit16u  guest_yres;
	Bit8u   guest_bpp; //348

	bool snapshot_mode; //351
	struct {
		Bit8u blue;
		Bit8u green;
		Bit8u red;
		Bit8u reserved;
	} palette[256];
	Bit8u* snapshot_buffer;

#if BX_USE_GUI_CONSOLE  //370
	struct {
		bool present;
		bool running;
		Bit8u* screen;
		Bit8u* oldscreen;
		Bit8u saved_fwidth;
		Bit8u saved_fheight;
		Bit16u saved_xres;
		Bit16u saved_yres;
		Bit8u  saved_bpp;
		Bit8u saved_palette[32];
		Bit8u saved_charmap[0x2000];
		unsigned cursor_x;
		unsigned cursor_y;
		Bit16u cursor_addr;
		bx_vga_tminfo_t tminfo;
		Bit8u keys[16];
		Bit8u n_keys;
	} console;
	Bit32u marker_count; //397
#endif
	struct { //392
		bool present;
		bool active;
	} command_mode;

	struct { //399
#if BX_SHOW_IPS
		bool hide_ips;
#endif
		bool nokeyrepeat;
#if BX_DEBUGGER && BX_DEBUGGER_GUI
		bool enh_dbg_enabled;
		bool enh_dbg_global_ini;
#endif
	} gui_opts;
};

#define BX_KEY_PRESSED  0x00000000
#define BX_KEY_RELEASED 0x80000000

#define BX_KEY_UNHANDLED 0x10000000

enum {
	BX_KEY_CTRL_L,
	BX_KEY_SHIFT_L,

	BX_KEY_F1,
	BX_KEY_F2,
	BX_KEY_F3,
	BX_KEY_F4,
	BX_KEY_F5,
	BX_KEY_F6,
	BX_KEY_F7,
	BX_KEY_F8,
	BX_KEY_F9,
	BX_KEY_F10,
	BX_KEY_F11,
	BX_KEY_F12,

	BX_KEY_CTRL_R,
	BX_KEY_SHIFT_R,
	BX_KEY_CAPS_LOCK,
	BX_KEY_NUM_LOCK,
	BX_KEY_ALT_L,
	BX_KEY_ALT_R,

	BX_KEY_A,
	BX_KEY_B,
	BX_KEY_C,
	BX_KEY_D,
	BX_KEY_E,
	BX_KEY_F,
	BX_KEY_G,
	BX_KEY_H,
	BX_KEY_I,
	BX_KEY_J,
	BX_KEY_K,
	BX_KEY_L,
	BX_KEY_M,
	BX_KEY_N,
	BX_KEY_O,
	BX_KEY_P,
	BX_KEY_Q,
	BX_KEY_R,
	BX_KEY_S,
	BX_KEY_T,
	BX_KEY_U,
	BX_KEY_V,
	BX_KEY_W,
	BX_KEY_X,
	BX_KEY_Y,
	BX_KEY_Z,

	BX_KEY_0,
	BX_KEY_1,
	BX_KEY_2,
	BX_KEY_3,
	BX_KEY_4,
	BX_KEY_5,
	BX_KEY_6,
	BX_KEY_7,
	BX_KEY_8,
	BX_KEY_9,

	BX_KEY_ESC,

	BX_KEY_SPACE,
	BX_KEY_SINGLE_QUOTE,
	BX_KEY_COMMA,
	BX_KEY_PERIOD,
	BX_KEY_SLASH,

	BX_KEY_SEMICOLON,
	BX_KEY_EQUALS,

	BX_KEY_LEFT_BRACKET,
	BX_KEY_BACKSLASH,
	BX_KEY_RIGHT_BRACKET,
	BX_KEY_MINUS,
	BX_KEY_GRAVE,

	BX_KEY_BACKSPACE,
	BX_KEY_ENTER,
	BX_KEY_TAB,

	BX_KEY_LEFT_BACKSLASH,
	BX_KEY_PRINT,
	BX_KEY_SCRL_LOCK,
	BX_KEY_PAUSE,

	BX_KEY_INSERT,
	BX_KEY_DELETE,
	BX_KEY_HOME,
	BX_KEY_END,
	BX_KEY_PAGE_UP,
	BX_KEY_PAGE_DOWN,

	BX_KEY_KP_ADD,
	BX_KEY_KP_SUBTRACT,
	BX_KEY_KP_END,
	BX_KEY_KP_DOWN,
	BX_KEY_KP_PAGE_DOWN,
	BX_KEY_KP_LEFT,
	BX_KEY_KP_RIGHT,
	BX_KEY_KP_HOME,
	BX_KEY_KP_UP,
	BX_KEY_KP_PAGE_UP,
	BX_KEY_KP_INSERT,
	BX_KEY_KP_DELETE,
	BX_KEY_KP_5,

	BX_KEY_UP,
	BX_KEY_DOWN,
	BX_KEY_LEFT,
	BX_KEY_RIGHT,

	BX_KEY_KP_ENTER,
	BX_KEY_KP_MULTIPLY,
	BX_KEY_KP_DIVIDE,

	BX_KEY_WIN_L,
	BX_KEY_WIN_R,
	BX_KEY_MENU,

	BX_KEY_ALT_SYSREQ,
	BX_KEY_CTRL_BREAK,

	BX_KEY_INT_BACK,
	BX_KEY_INT_FORWARD,
	BX_KEY_INT_STOP,
	BX_KEY_INT_MAIL,
	BX_KEY_INT_SEARCH,
	BX_KEY_INT_FAV,
	BX_KEY_INT_HOME,

	BX_KEY_POWER_MYCOMP,
	BX_KEY_POWER_CALC,
	BX_KEY_POWER_SLEEP,
	BX_KEY_POWER_POWER,
	BX_KEY_POWER_WAKE,

	BX_KEY_NBKEYS
};