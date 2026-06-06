#pragma once
#include "extplugin.h"
#define BX_PLUGIN_UNMAPPED  "unmapped" //43
#define BX_PLUGIN_BIOSDEV   "biosdev"//44
#define BX_PLUGIN_VGA       "vga"
#define BX_PLUGIN_CMOS      "cmos" //45
#define BX_PLUGIN_FLOPPY    "floppy"
#define BX_PLUGIN_PARALLEL  "parallel"//49
#define BX_PLUGIN_SERIAL    "serial"
#define BX_PLUGIN_KEYBOARD  "keyboard"
#define BX_PLUGIN_HARDDRV   "harddrv"
#define BX_PLUGIN_DMA       "dma" //54
#define BX_PLUGIN_PIC       "pic" //55
#define BX_PLUGIN_PIT       "pit"//56
#define BX_PLUGIN_PCI       "pci"  //57
#define BX_PLUGIN_PCI2ISA   "pci2isa"//58
#define BX_PLUGIN_PCI_IDE   "pci_ide"
#define BX_PLUGIN_USB_UHCI  "usb_uhci"
#define BX_PLUGIN_SPEAKER   "speaker"//72
#define BX_PLUGIN_ACPI      "acpi" //73
#define BX_PLUGIN_IOAPIC    "ioapic"
#define BX_PLUGIN_HPET      "hpet"

#define BX_REGISTER_DEVICE_DEVMODEL(a,b,c,d) pluginRegisterDeviceDevmodel(a,b,c,d)  //80
#define PLUG_device_present(a) pluginDevicePresent(a)//82

#if BX_PLUGINS//84
#define PLUG_load_plugin(name,type) {bx_load_plugin(#name,type);}
// newer plugin macros for variable plugin handling
#define PLUG_get_plugins_count(type) bx_get_plugins_count(type)
#define PLUG_get_plugin_name(type,index) bx_get_plugin_name(type,index)
#define PLUG_get_plugin_flags(type,index) bx_get_plugin_flags(type,index)
#define PLUG_load_plugin_var(name,type) {bx_load_plugin(name,type);}
#define PLUG_load_opt_plugin(name) bx_load_plugin(name,PLUGTYPE_OPTIONAL)
#define PLUG_unload_opt_plugin(name) bx_unload_plugin(name,1)

#define DEV_register_ioread_handler(b,c,d,e,f)  pluginRegisterIOReadHandler(b,c,d,e,f)
#define DEV_register_iowrite_handler(b,c,d,e,f) pluginRegisterIOWriteHandler(b,c,d,e,f)
#define DEV_unregister_ioread_handler(b,c,d,e)  pluginUnregisterIOReadHandler(b,c,d,e)
#define DEV_unregister_iowrite_handler(b,c,d,e) pluginUnregisterIOWriteHandler(b,c,d,e)
#define DEV_register_ioread_handler_range(b,c,d,e,f,g)  pluginRegisterIOReadHandlerRange(b,c,d,e,f,g)
#define DEV_register_iowrite_handler_range(b,c,d,e,f,g) pluginRegisterIOWriteHandlerRange(b,c,d,e,f,g)
#define DEV_unregister_ioread_handler_range(b,c,d,e,f)  pluginUnregisterIOReadHandlerRange(b,c,d,e,f)
#define DEV_unregister_iowrite_handler_range(b,c,d,e,f) pluginUnregisterIOWriteHandlerRange(b,c,d,e,f)
#define DEV_register_default_ioread_handler(b,c,d,e) pluginRegisterDefaultIOReadHandler(b,c,d,e)
#define DEV_register_default_iowrite_handler(b,c,d,e) pluginRegisterDefaultIOWriteHandler(b,c,d,e)

#define DEV_register_irq(b,c) pluginRegisterIRQ(b,c)
#define DEV_unregister_irq(b,c) pluginUnregisterIRQ(b,c)

#else//111
#define PLUG_load_plugin(name,type) {lib##name##_plugin_entry(NULL,type,PLUGIN_INIT);} //115

#define PLUG_get_plugins_count(type) bx_get_plugins_count_np(type)//117
#define PLUG_get_plugin_name(type,index) bx_get_plugin_name_np(type,index)//118
#define PLUG_get_plugin_flags(type,index) bx_get_plugin_flags_np(type,index)
#define PLUG_load_plugin_var(name,type) bx_load_plugin_np(name,type)
#define PLUG_load_opt_plugin(name) bx_load_plugin_np(name,PLUGTYPE_OPTIONAL)
#define PLUG_unload_opt_plugin(name) bx_unload_opt_plugin(name,1)

#define DEV_register_ioread_handler(b,c,d,e,f) bx_devices.register_io_read_handler(b,c,d,e,f)
#define DEV_register_iowrite_handler(b,c,d,e,f) bx_devices.register_io_write_handler(b,c,d,e,f)
#define DEV_unregister_ioread_handler(b,c,d,e)  bx_devices.unregister_io_read_handler(b,c,d,e)
#define DEV_unregister_iowrite_handler(b,c,d,e) bx_devices.unregister_io_write_handler(b,c,d,e)
#define DEV_register_ioread_handler_range(b,c,d,e,f,g)  bx_devices.register_io_read_handler_range(b,c,d,e,f,g)
#define DEV_register_iowrite_handler_range(b,c,d,e,f,g) bx_devices.register_io_write_handler_range(b,c,d,e,f,g)
#define DEV_unregister_ioread_handler_range(b,c,d,e,f)  bx_devices.unregister_io_read_handler_range(b,c,d,e,f)
#define DEV_unregister_iowrite_handler_range(b,c,d,e,f) bx_devices.unregister_io_write_handler_range(b,c,d,e,f)
#define DEV_register_default_ioread_handler(b,c,d,e) bx_devices.register_default_io_read_handler(b,c,d,e)
#define DEV_register_default_iowrite_handler(b,c,d,e) bx_devices.register_default_io_write_handler(b,c,d,e)

#define DEV_register_irq(b,c) bx_devices.register_irq(b,c)
#define DEV_unregister_irq(b,c) bx_devices.unregister_irq(b,c)

#endif // #if BX_PLUGINS  137

///////// Common device macros
#define DEV_init_devices() {bx_devices.init(BX_MEM(0)); }  //140
#define DEV_reset_devices(type) {bx_devices.reset(type); } //141
#define DEV_register_state() {bx_devices.register_state(); }
#define DEV_after_restore_state() {bx_devices.after_restore_state(); }
#define DEV_register_timer(a,b,c,d,e,f) bx_pc_system.register_timer(a,b,c,d,e,f) //144
///////// Removable devices macros
#define DEV_register_default_keyboard(a,b,c) (bx_devices.register_default_keyboard(a,b,c)) //147
#define DEV_register_removable_keyboard(a,b,c,d) (bx_devices.register_removable_keyboard(a,b,c,d))
#define DEV_unregister_removable_keyboard(a) (bx_devices.unregister_removable_keyboard(a))
#define DEV_register_default_mouse(a,b,c) (bx_devices.register_default_mouse(a,b,c)) //150
#define DEV_register_removable_mouse(a,b,c) (bx_devices.register_removable_mouse(a,b,c))
#define DEV_unregister_removable_mouse(a) (bx_devices.unregister_removable_mouse(a))

///////// I/O APIC macros
#define DEV_ioapic_present() (bx_devices.pluginIOAPIC != &bx_devices.stubIOAPIC)//155
#define DEV_ioapic_set_enabled(a,b) (bx_devices.pluginIOAPIC->set_enabled(a,b))
#define DEV_ioapic_receive_eoi(a) (bx_devices.pluginIOAPIC->receive_eoi(a)) //157
#define DEV_ioapic_set_irq_level(a,b) (bx_devices.pluginIOAPIC->set_irq_level(a,b))//158

///////// CMOS macros
#define DEV_cmos_get_reg(a) (bx_devices.pluginCmosDevice->get_reg(a)) //161
#define DEV_cmos_set_reg(a,b) (bx_devices.pluginCmosDevice->set_reg(a,b)) //162
#define DEV_cmos_checksum() (bx_devices.pluginCmosDevice->checksum_cmos()) //163
#define DEV_cmos_enable_irq(a) (bx_devices.pluginCmosDevice->enable_irq(a))

///////// PIT macro
#define DEV_pit_enable_irq(a) (bx_devices.pluginPitDevice->enable_irq(a))
///////// keyboard macros
#define DEV_kbd_gen_scancode(key) (bx_devices.gen_scancode(key))
#define DEV_kbd_paste_bytes(bytes, count) (bx_devices.paste_bytes(bytes,count))
#define DEV_kbd_release_keys() (bx_devices.release_keys())
#define DEV_kbd_set_indicator(a,b,c) (bx_devices.kbd_set_indicator(a,b,c)) //173

///////// mouse macro
#define DEV_mouse_motion(dx, dy, dz, bs, absxy) (bx_devices.mouse_motion(dx, dy, dz, bs, absxy))

///////// hard drive macros
#define DEV_hd_read_handler(a, b, c) \
    (bx_devices.pluginHardDrive->virt_read_handler(b, c))
#define DEV_hd_write_handler(a, b, c, d) \
    (bx_devices.pluginHardDrive->virt_write_handler(b, c, d))
#define DEV_hd_bmdma_read_sector(a,b,c) bx_devices.pluginHardDrive->bmdma_read_sector(a,b,c)
#define DEV_hd_bmdma_write_sector(a,b) bx_devices.pluginHardDrive->bmdma_write_sector(a,b)
#define DEV_hd_bmdma_complete(a) bx_devices.pluginHardDrive->bmdma_complete(a)

#define DEV_bulk_io_quantum_requested() (bx_devices.bulkIOQuantumsRequested)
#define DEV_bulk_io_quantum_transferred() (bx_devices.bulkIOQuantumsTransferred)
#define DEV_bulk_io_host_addr() (bx_devices.bulkIOHostAddr)

///////// DMA macros
#define DEV_dma_register_8bit_channel(channel, dmaRead, dmaWrite, name) \
  (bx_devices.pluginDmaDevice->registerDMA8Channel(channel, dmaRead, dmaWrite, name))
#define DEV_dma_register_16bit_channel(channel, dmaRead, dmaWrite, name) \
  (bx_devices.pluginDmaDevice->registerDMA16Channel(channel, dmaRead, dmaWrite, name))
#define DEV_dma_unregister_channel(channel) \
  (bx_devices.pluginDmaDevice->unregisterDMAChannel(channel))
#define DEV_dma_set_drq(channel, val) \
  (bx_devices.pluginDmaDevice->set_DRQ(channel, val))
#define DEV_dma_get_tc() \
  (bx_devices.pluginDmaDevice->get_TC())
#define DEV_dma_raise_hlda() \
  (bx_devices.pluginDmaDevice->raise_HLDA())

///////// PIC macros
#define DEV_pic_lower_irq(b)  (bx_devices.pluginPicDevice->lower_irq(b)) //206
#define DEV_pic_raise_irq(b)  (bx_devices.pluginPicDevice->raise_irq(b))
#define DEV_pic_set_mode(a,b) (bx_devices.pluginPicDevice->set_mode(a,b))
#define DEV_pic_iac()         (bx_devices.pluginPicDevice->IAC()) //209

///////// VGA macros
#define DEV_vga_redraw_area(left, top, right, bottom) \
  (bx_devices.pluginVgaDevice->vga_redraw_area(left, top, right, bottom))
#define DEV_vga_get_text_snapshot(rawsnap, height, width) \
  (bx_devices.pluginVgaDevice->get_text_snapshot(rawsnap, height, width))
#define DEV_vga_refresh(a) \
  (bx_devices.pluginVgaDevice->refresh_display(bx_devices.pluginVgaDevice,a))
#define DEV_vga_set_override(a,b) (bx_devices.pluginVgaDevice->set_override(a,b))

///////// PCI macros
#define DEV_register_pci_handlers(a,b,c,d) \
  (bx_devices.register_pci_handlers(a,b,c,d,0))
#define DEV_register_pci_handlers2(a,b,c,d,e) \
  (bx_devices.register_pci_handlers(a,b,c,d,e))
#define DEV_pci_get_slot_mapping() bx_devices.pci_get_slot_mapping()
#define DEV_pci_get_confAddr() bx_devices.pci_get_confAddr()
#define DEV_pci_set_irq(a,b,c) bx_devices.pluginPci2IsaBridge->pci_set_irq(a,b,c)
#define DEV_pci_set_base_mem(a,b,c,d,e,f) \
  (bx_devices.pci_set_base_mem(a,b,c,d,e,f))
#define DEV_pci_set_base_io(a,b,c,d,e,f,g,h) \
  (bx_devices.pci_set_base_io(a,b,c,d,e,f,g,h))
#define DEV_ide_bmdma_present() bx_devices.pluginPciIdeController->bmdma_present()
#define DEV_ide_bmdma_set_irq(a) bx_devices.pluginPciIdeController->bmdma_set_irq(a)
#define DEV_ide_bmdma_start_transfer(a) \
  bx_devices.pluginPciIdeController->bmdma_start_transfer(a)
#define DEV_acpi_generate_smi(a) bx_devices.pluginACPIController->generate_smi(a)
#define DEV_agp_present() (bx_devices.is_agp_present())

///////// Speaker macros
#define DEV_speaker_beep_on(frequency) bx_devices.pluginSpeaker->beep_on(frequency)
#define DEV_speaker_beep_off() bx_devices.pluginSpeaker->beep_off()
#define DEV_speaker_set_line(a) bx_devices.pluginSpeaker->set_line(a)//242

///////// Memory macros
#define DEV_register_memory_handlers(param,rh,wh,b,e) \
    bx_devices.mem->registerMemoryHandlers(param,rh,wh,b,e)
#define DEV_unregister_memory_handlers(param,b,e) \
    bx_devices.mem->unregisterMemoryHandlers(param,b,e)
#define DEV_mem_set_memory_type(a,b,c) \
    bx_devices.mem->set_memory_type((memory_area_t)a,b,c)
#define DEV_mem_set_bios_write(a) bx_devices.mem->set_bios_write(a)
#define DEV_mem_set_bios_rom_access(a,b) bx_devices.mem->set_bios_rom_access(a,b)

///////// USB device macro
#define DEV_usb_init_device(a,b,c,d,p) bx_usbdev_ctl.init_device(a,b,(void**)c,d,p)
///////// Sound module macros
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
BOCHSAPI bool pluginDevicePresent(const char* name);

extern void bx_init_plugins(void); //347
extern void bx_reset_plugins(unsigned);//348
extern void bx_plugins_register_state(void);
extern void bx_plugins_after_restore_state(void);
#if !BX_PLUGINS
extern plugin_t bx_builtin_plugins[]; //353
Bit8u bx_get_plugins_count_np(Bit16u type);
const char* bx_get_plugin_name_np(Bit16u type, Bit8u index);
Bit8u bx_get_plugin_flags_np(Bit16u type, Bit8u index);
int bx_load_plugin_np(const char* name, Bit16u type);
int bx_unload_opt_plugin(const char* name, bool devflag);
#endif
#if BX_PLUGINS && defined(WIN32)

#elif BX_PLUGINS

#else
#define PLUGIN_ENTRY_FOR_MODULE(mod) \
  int CDECL lib##mod##_plugin_entry(plugin_t *plugin, Bit16u type, Bit8u mode) //396
#define PLUGIN_ENTRY_FOR_GUI_MODULE(mod) \
  int CDECL lib##mod##_gui_plugin_entry(plugin_t *plugin, Bit16u type, Bit8u mode)

// device plugins
PLUGIN_ENTRY_FOR_MODULE(harddrv);
PLUGIN_ENTRY_FOR_MODULE(keyboard);
PLUGIN_ENTRY_FOR_MODULE(serial);//411
PLUGIN_ENTRY_FOR_MODULE(biosdev);//413
PLUGIN_ENTRY_FOR_MODULE(cmos); //414
PLUGIN_ENTRY_FOR_MODULE(dma); //415
PLUGIN_ENTRY_FOR_MODULE(pic);//416
PLUGIN_ENTRY_FOR_MODULE(pit);
PLUGIN_ENTRY_FOR_MODULE(vga);
PLUGIN_ENTRY_FOR_MODULE(floppy);
PLUGIN_ENTRY_FOR_MODULE(parallel);//421
PLUGIN_ENTRY_FOR_MODULE(pci); //422
PLUGIN_ENTRY_FOR_MODULE(pci2isa);//423
PLUGIN_ENTRY_FOR_MODULE(pci_ide);
PLUGIN_ENTRY_FOR_MODULE(usb_uhci);//426
PLUGIN_ENTRY_FOR_MODULE(speaker);//438
PLUGIN_ENTRY_FOR_MODULE(acpi);//439
PLUGIN_ENTRY_FOR_MODULE(ioapic);
PLUGIN_ENTRY_FOR_MODULE(hpet);
PLUGIN_ENTRY_FOR_MODULE(unmapped);
// gui plugins
PLUGIN_ENTRY_FOR_GUI_MODULE(nogui);
PLUGIN_ENTRY_FOR_GUI_MODULE(win32);

#endif