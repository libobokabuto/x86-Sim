#pragma once
#define BX_KEYMAP_UNKNOWN   0xFFFFFFFF

// Structure of an element of the keymap table
typedef struct BOCHSAPI {
	Bit32u baseKey;   // base key
	Bit32u modKey;   // modifier key that must be held down
	Bit32s ascii;    // ascii equivalent, if any
	Bit32u hostKey;  // value that the host's OS or library recognizes
} BXKeyEntry;

class BOCHSAPI bx_keymap_c  {
public:
	bx_keymap_c(void);
	~bx_keymap_c(void);

	void   loadKeymap(const char* prefix, Bit32u(*)(const char*));
	void   loadKeymap(Bit32u(*)(const char*), const char* filename);
	bool   isKeymapLoaded();

	BXKeyEntry* findHostKey(Bit32u hostkeynum);
	BXKeyEntry* findAsciiChar(Bit8u ascii);
	const char* getBXKeyName(Bit32u key);

private:
	Bit32u convertStringToBXKey(const char*);

	BXKeyEntry* keymapTable;
	Bit16u   keymapCount;
};

BOCHSAPI extern bx_keymap_c bx_keymap;