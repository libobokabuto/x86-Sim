#pragma once
typedef struct BOCHSAPI {
	// standard argc,argv
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