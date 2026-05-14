#pragma once

extern const unsigned char translation8042[256];

typedef struct {
	const char* make;
	const char* brek;
} scancode;

// Scancodes table
extern scancode scancodes[BX_KEY_NBKEYS][3];