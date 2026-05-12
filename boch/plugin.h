#pragma once
#include "extplugin.h"
#define BX_PLUGIN_UNMAPPED  "unmapped" //43
#define BX_PLUGIN_CMOS      "cmos" //45
#define BX_PLUGIN_PCI       "pci"  //57
#define BX_REGISTER_DEVICE_DEVMODEL(a,b,c,d) pluginRegisterDeviceDevmodel(a,b,c,d)  //80
#define PLUG_load_plugin(name,type) {lib##name##_plugin_entry(NULL,type,PLUGIN_INIT);} //115
#define DEV_register_ioread_handler(b,c,d,e,f) bx_devices.register_io_read_handler(b,c,d,e,f) //124
#define DEV_register_iowrite_handler(b,c,d,e,f) bx_devices.register_io_write_handler(b,c,d,e,f) //125
#define DEV_register_default_ioread_handler(b,c,d,e) bx_devices.register_default_io_read_handler(b,c,d,e) //132
#define DEV_register_default_iowrite_handler(b,c,d,e) bx_devices.register_default_io_write_handler(b,c,d,e) //133
#define DEV_register_irq(b,c) bx_devices.register_irq(b,c) //134
#define DEV_init_devices() {bx_devices.init(BX_MEM(0)); }  //140
#define DEV_ioapic_receive_eoi(a) (bx_devices.pluginIOAPIC->receive_eoi(a))
#define DEV_cmos_checksum() (bx_devices.pluginCmosDevice->checksum_cmos()) //163
#define DEV_pic_lower_irq(b)  (bx_devices.pluginPicDevice->lower_irq(b)) //206
typedef struct _device_t
{
    //281
    const char* name;
    plugin_t* plugin;
    Bit16u       plugtype;

    class bx_devmodel_c* devmodel;  // BBD hack

    struct _device_t* next;
} device_t;

extern device_t* devices;//293

BOCHSAPI void pluginRegisterDeviceDevmodel(plugin_t* plugin, Bit16u type, bx_devmodel_c* dev, const char* name); //303
extern void bx_init_plugins(void); //347
#if !BX_PLUGINS
extern plugin_t bx_builtin_plugins[]; //353
#endif
#define PLUGIN_ENTRY_FOR_MODULE(mod) \
  int CDECL lib##mod##_plugin_entry(plugin_t *plugin, Bit16u type, Bit8u mode) //396

PLUGIN_ENTRY_FOR_MODULE(cmos); //414
PLUGIN_ENTRY_FOR_MODULE(pci); //422
