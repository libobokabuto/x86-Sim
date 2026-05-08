#pragma once
#define PLUGTYPE_NULL      0x00
#define PLUGTYPE_CORE      0x01
#define PLUGTYPE_STANDARD  0x02 //32
#define PLUGTYPE_OPTIONAL  0x04 //33
#define PLUGTYPE_VGA       0x08
#define PLUGTYPE_CI        0x80 //36
#define PLUGTYPE_GUI      0x100

#define PLUGFLAG_PCI 0x01

#define PLUGIN_FINI  0
#define PLUGIN_INIT  1
#define PLUGIN_PROBE 2

typedef int (CDECL* plugin_entry_t)(struct _plugin_t* plugin, Bit16u type, Bit8u mode);

typedef struct _plugin_t
{
#if BX_PLUGINS
    char* name;
#if defined(WIN32)
    HINSTANCE handle;
#else
    lt_dlhandle handle;
#endif
#else
    const char* name;
#endif
    Bit16u type;
    Bit8u flags;
    plugin_entry_t plugin_entry;
    bool initialized;
    Bit16u loadtype;

#if BX_PLUGINS
    struct _plugin_t* next;
#endif
} plugin_t;