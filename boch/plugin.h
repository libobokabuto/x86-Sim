#pragma once
#include "extplugin.h"
#define BX_PLUGIN_UNMAPPED  "unmapped" //43
#define BX_PLUGIN_KEYBOARD  "keyboard"
#define BX_PLUGIN_CMOS      "cmos" //45
#define BX_PLUGIN_DMA       "dma" //54
#define BX_PLUGIN_PCI       "pci"  //57
#define BX_REGISTER_DEVICE_DEVMODEL(a,b,c,d) pluginRegisterDeviceDevmodel(a,b,c,d)  //80
#define PLUG_load_plugin(name,type) {lib##name##_plugin_entry(NULL,type,PLUGIN_INIT);} //115
#define DEV_register_ioread_handler(b,c,d,e,f) bx_devices.register_io_read_handler(b,c,d,e,f) //124
#define DEV_register_iowrite_handler(b,c,d,e,f) bx_devices.register_io_write_handler(b,c,d,e,f) //125
#define DEV_register_default_ioread_handler(b,c,d,e) bx_devices.register_default_io_read_handler(b,c,d,e) //132
#define DEV_register_default_iowrite_handler(b,c,d,e) bx_devices.register_default_io_write_handler(b,c,d,e) //133
#define DEV_register_irq(b,c) bx_devices.register_irq(b,c) //134
#define DEV_init_devices() {bx_devices.init(BX_MEM(0)); }  //140
#define DEV_reset_devices(type) {bx_devices.reset(type); } //141
#define DEV_register_timer(a,b,c,d,e,f) bx_pc_system.register_timer(a,b,c,d,e,f) //144
#define DEV_register_default_keyboard(a,b,c) (bx_devices.register_default_keyboard(a,b,c)) //147
#define DEV_register_default_mouse(a,b,c) (bx_devices.register_default_mouse(a,b,c)) //150
#define DEV_ioapic_receive_eoi(a) (bx_devices.pluginIOAPIC->receive_eoi(a)) //157
#define DEV_cmos_get_reg(a) (bx_devices.pluginCmosDevice->get_reg(a)) //161
#define DEV_cmos_set_reg(a,b) (bx_devices.pluginCmosDevice->set_reg(a,b)) //162
#define DEV_cmos_checksum() (bx_devices.pluginCmosDevice->checksum_cmos()) //163
#define DEV_kbd_set_indicator(a,b,c) (bx_devices.kbd_set_indicator(a,b,c)) //173

#define DEV_dma_raise_hlda() \
  (bx_devices.pluginDmaDevice->raise_HLDA())

#define DEV_pic_lower_irq(b)  (bx_devices.pluginPicDevice->lower_irq(b)) //206
#define DEV_pic_raise_irq(b)  (bx_devices.pluginPicDevice->raise_irq(b))
#define DEV_pic_iac()         (bx_devices.pluginPicDevice->IAC()) //209
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
extern void bx_reset_plugins(unsigned);//348
#if !BX_PLUGINS
extern plugin_t bx_builtin_plugins[]; //353
#endif
#define PLUGIN_ENTRY_FOR_MODULE(mod) \
  int CDECL lib##mod##_plugin_entry(plugin_t *plugin, Bit16u type, Bit8u mode) //396

PLUGIN_ENTRY_FOR_MODULE(keyboard);
PLUGIN_ENTRY_FOR_MODULE(cmos); //414
PLUGIN_ENTRY_FOR_MODULE(dma); //415
PLUGIN_ENTRY_FOR_MODULE(pci); //422
