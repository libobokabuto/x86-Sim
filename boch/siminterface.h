#pragma once
#include "paramtree.h"

enum {  //462
	BX_MOUSE_TYPE_NONE,
	BX_MOUSE_TYPE_PS2,
	BX_MOUSE_TYPE_IMPS2,
#if BX_SUPPORT_BUSMOUSE
	BX_MOUSE_TYPE_INPORT,
	BX_MOUSE_TYPE_BUS,
#endif
	BX_MOUSE_TYPE_SERIAL,
	BX_MOUSE_TYPE_SERIAL_WHEEL,
	BX_MOUSE_TYPE_SERIAL_MSYS
};

enum disp_mode_t { DISP_MODE_CONFIG = 100, DISP_MODE_SIM }; //606

typedef struct BOCHSAPI {  //809
	// standard argc,argv
	//809
	int argc;
	char** argv;
#ifdef WIN32
	char initial_dir[MAX_PATH];
#endif
#ifdef __WXMSW__
	// these are only used when compiling with wxWidgets.  This gives us a
	// place to store the data that was passed to WinMain.
	HINSTANCE hInstance;
	HINSTANCE hPrevInstance;
	LPSTR m_lpCmdLine;
	int nCmdShow;
#endif
} bx_startup_flags_t;

BOCHSAPI extern bx_startup_flags_t bx_startup_flags;
BOCHSAPI extern bool bx_user_quit;