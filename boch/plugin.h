#pragma once
#include "extplugin.h"
#define BX_PLUGIN_UNMAPPED  "unmapped" //43
#define BX_PLUGIN_BIOSDEV   "biosdev"//44
#define BX_PLUGIN_CMOS      "cmos" //45
#define BX_PLUGIN_PARALLEL  "parallel"//49
#define BX_PLUGIN_SERIAL    "serial"
#define BX_PLUGIN_KEYBOARD  "keyboard"
#define BX_PLUGIN_DMA       "dma" //54
#define BX_PLUGIN_PIC       "pic" //55
#define BX_PLUGIN_PIT       "pit"//56
#define BX_PLUGIN_PCI       "pci"  //57
#define BX_PLUGIN_SPEAKER   "speaker"//72


#define BX_REGISTER_DEVICE_DEVMODEL(a,b,c,d) pluginRegisterDeviceDevmodel(a,b,c,d)  //80
#define PLUG_get_plugins_count(type) bx_get_plugins_count_np(type)//117
#define PLUG_get_plugin_name(type,index) bx_get_plugin_name_np(type,index)//118
#define PLUG_load_plugin(name,type) {lib##name##_plugin_entry(NULL,type,PLUGIN_INIT);} //115
#define DEV_register_ioread_handler(b,c,d,e,f) bx_devices.register_io_read_handler(b,c,d,e,f) //124
#define DEV_register_iowrite_handler(b,c,d,e,f) bx_devices.register_io_write_handler(b,c,d,e,f) //125
#define DEV_register_default_ioread_handler(b,c,d,e) bx_devices.register_default_io_read_handler(b,c,d,e) //132
#define DEV_register_default_iowrite_handler(b,c,d,e) bx_devices.register_default_io_write_handler(b,c,d,e) //133
#define DEV_register_irq(b,c) bx_devices.register_irq(b,c) //134
#define DEV_unregister_irq(b,c) bx_devices.unregister_irq(b,c)//135
#define DEV_init_devices() {bx_devices.init(BX_MEM(0)); }  //140
#define DEV_reset_devices(type) {bx_devices.reset(type); } //141
#define DEV_register_timer(a,b,c,d,e,f) bx_pc_system.register_timer(a,b,c,d,e,f) //144
#define DEV_register_default_keyboard(a,b,c) (bx_devices.register_default_keyboard(a,b,c)) //147
#define DEV_register_default_mouse(a,b,c) (bx_devices.register_default_mouse(a,b,c)) //150
#define DEV_ioapic_present() (bx_devices.pluginIOAPIC != &bx_devices.stubIOAPIC)//155
#define DEV_ioapic_receive_eoi(a) (bx_devices.pluginIOAPIC->receive_eoi(a)) //157
#define DEV_ioapic_set_irq_level(a,b) (bx_devices.pluginIOAPIC->set_irq_level(a,b))//158
#define DEV_cmos_get_reg(a) (bx_devices.pluginCmosDevice->get_reg(a)) //161
#define DEV_cmos_set_reg(a,b) (bx_devices.pluginCmosDevice->set_reg(a,b)) //162
#define DEV_cmos_checksum() (bx_devices.pluginCmosDevice->checksum_cmos()) //163
#define DEV_kbd_set_indicator(a,b,c) (bx_devices.kbd_set_indicator(a,b,c)) //173

#define DEV_dma_raise_hlda() \
  (bx_devices.pluginDmaDevice->raise_HLDA())

#define DEV_pic_lower_irq(b)  (bx_devices.pluginPicDevice->lower_irq(b)) //206
#define DEV_pic_raise_irq(b)  (bx_devices.pluginPicDevice->raise_irq(b))
#define DEV_pic_set_mode(a,b) (bx_devices.pluginPicDevice->set_mode(a,b))
#define DEV_pic_iac()         (bx_devices.pluginPicDevice->IAC()) //209
#define DEV_speaker_beep_on(frequency) bx_devices.pluginSpeaker->beep_on(frequency)
#define DEV_speaker_beep_off() bx_devices.pluginSpeaker->beep_off()
#define DEV_speaker_set_line(a) bx_devices.pluginSpeaker->set_line(a)//242
#define DEV_sound_get_waveout(a) (bx_soundmod_ctl.get_waveout(a))//258
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
Bit8u bx_get_plugins_count_np(Bit16u type);
const char* bx_get_plugin_name_np(Bit16u type, Bit8u index);
#endif



#define PLUGIN_ENTRY_FOR_MODULE(mod) \
  int CDECL lib##mod##_plugin_entry(plugin_t *plugin, Bit16u type, Bit8u mode) //396

PLUGIN_ENTRY_FOR_MODULE(keyboard);
PLUGIN_ENTRY_FOR_MODULE(serial);//411
PLUGIN_ENTRY_FOR_MODULE(biosdev);//413
PLUGIN_ENTRY_FOR_MODULE(cmos); //414
PLUGIN_ENTRY_FOR_MODULE(dma); //415
PLUGIN_ENTRY_FOR_MODULE(pic);//416
PLUGIN_ENTRY_FOR_MODULE(pit);
PLUGIN_ENTRY_FOR_MODULE(parallel);//421
PLUGIN_ENTRY_FOR_MODULE(pci); //422
PLUGIN_ENTRY_FOR_MODULE(speaker);//438
