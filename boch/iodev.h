#pragma once
#include "bochs.h"
#include "plugin.h"
#include "param_names.h"
#include "pc_system.h"
#include "memory-bochs.h"
#include "siminterface.h"
#include "gui.h"

#define BX_MAX_IRQS 16 //39

#define BX_KBD_LED_NUM  0
#define BX_KBD_LED_CAPS 1
#define BX_KBD_LED_SCRL 2
#define BX_KBD_LED_MASK_NUM 1
#define BX_KBD_LED_MASK_ALL 7

#define BX_KBD_ELEMENTS 16

/* size of internal buffer for mouse devices */
#define BX_MOUSE_BUFF_SIZE 48

#define BX_DMA_BUFFER_SIZE 512

#define BX_MAX_PCI_DEVICES 20

typedef Bit32u(*bx_read_handler_t)(void*, Bit32u, unsigned);//59
typedef void   (*bx_write_handler_t)(void*, Bit32u, Bit32u, unsigned); //60
typedef bool (*bx_kbd_gen_scancode_t)(void*, Bit32u); //62
typedef Bit8u(*bx_kbd_get_elements_t)(void*);
typedef void (*bx_mouse_enq_t)(void*, int, int, int, unsigned, bool);
typedef void (*bx_mouse_enabled_changed_t)(void*, bool);

class BOCHSAPI bx_devmodel_c {
public:
    virtual ~bx_devmodel_c() {}
    virtual void init(void) {}
    

};

#if BX_SUPPORT_PCI
//#define BX_DEBUG_PCI_READ(addr, value, io_len) \
  if (io_len == 1) \
    BX_DEBUG(("read  PCI register 0x%02X value 0x%02X (len=1)", address, value)); \
  else if (io_len == 2) \
    BX_DEBUG(("read  PCI register 0x%02X value 0x%04X (len=2)", address, value)); \
  else if (io_len == 4) \
    BX_DEBUG(("read  PCI register 0x%02X value 0x%08X (len=4)", address, value));

//#define BX_DEBUG_PCI_WRITE(addr, value, io_len) \
  if (io_len == 1) \
    BX_DEBUG(("write PCI register 0x%02X value 0x%02X (len=1)", addr, value)); \
  else if (io_len == 2) \
    BX_DEBUG(("write PCI register 0x%02X value 0x%04X (len=2)", addr, value)); \
  else if (io_len == 4) \
    BX_DEBUG(("write PCI register 0x%02X value 0x%08X (len=4)", addr, value));

enum {
    BX_PCI_BAR_TYPE_NONE = 0,
    BX_PCI_BAR_TYPE_MEM = 1,
    BX_PCI_BAR_TYPE_IO = 2
};

#define BX_PCI_ADVOPT_NOACPI 0x01
#define BX_PCI_ADVOPT_NOHPET 0x02
#define BX_PCI_ADVOPT_NOAGP  0x04

typedef struct {
    //132
    Bit8u  type;
    Bit32u size;
    Bit32u addr;
    union {
        struct {
            memory_handler_t rh;
            memory_handler_t wh;
            const Bit8u* dummy;
        } mem;
        struct {
            bx_read_handler_t rh;
            bx_write_handler_t wh;
            const Bit8u* mask;
        } io;
    };
} bx_pci_bar_t;

class BOCHSAPI bx_pci_device_c : public bx_devmodel_c {
    //150
public:
    bx_pci_device_c() : pci_rom(NULL), pci_rom_size(0) {
        for (int i = 0; i < 6; i++) memset(&pci_bar[i], 0, sizeof(bx_pci_bar_t));
    }
    virtual ~bx_pci_device_c() {
        if (pci_rom != NULL) delete[] pci_rom;
    }
protected:
    const char* pci_name;
    Bit8u pci_conf[256];
    bx_pci_bar_t pci_bar[6];
    Bit8u* pci_rom;
    Bit32u pci_rom_address;
    Bit32u pci_rom_size;
    memory_handler_t pci_rom_read_handler;


};
#endif
//192
#define STUBFUNC(dev,method) \
   pluginlog->panic("%s called in %s stub. you must not have loaded the %s plugin", #dev, #method, #dev)


class BOCHSAPI bx_hard_drive_stub_c : public bx_devmodel_c {
    //196
public:
};
class BOCHSAPI bx_cmos_stub_c : public bx_devmodel_c {
public:
    virtual Bit32u get_reg(Bit8u reg) {
        //STUBFUNC(cmos, get_reg); 
        return 0;
    }
    virtual void set_reg(Bit8u reg, Bit32u val) {
        //STUBFUNC(cmos, set_reg);
    }
    virtual void checksum_cmos(void) {
        //STUBFUNC(cmos, checksum);
    }
    virtual void enable_irq(bool enabled) {
        //STUBFUNC(cmos, enable_irq);
    }
};
class BOCHSAPI bx_pit_stub_c : public bx_devmodel_c {
    //228
public:
};
class BOCHSAPI bx_dma_stub_c : public bx_devmodel_c {
    //235
public:
};

class BOCHSAPI bx_pic_stub_c : public bx_devmodel_c {
public:
    virtual void raise_irq(unsigned irq_no) {
        //STUBFUNC(pic, raise_irq);
    }
    virtual void lower_irq(unsigned irq_no) {
        //STUBFUNC(pic, lower_irq);
    }
    virtual void set_mode(bool ma_sl, Bit8u mode) {
        //STUBFUNC(pic, set_mode);
    }
    virtual Bit8u IAC(void) {
        //STUBFUNC(pic, IAC);
        return 0;
    }
};
class BOCHSAPI bx_vga_stub_c
#if BX_SUPPORT_PCI
    : public bx_pci_device_c
#else
    : public bx_devmodel_c
#endif
{
};
class BOCHSAPI bx_speaker_stub_c : public bx_devmodel_c {
public:
};
#if BX_SUPPORT_PCI
class BOCHSAPI bx_pci2isa_stub_c : public bx_pci_device_c {
public:
};

class BOCHSAPI bx_pci_ide_stub_c : public bx_pci_device_c {
public:
};
class BOCHSAPI bx_acpi_ctrl_stub_c : public bx_pci_device_c {
public:
};
#endif
#if BX_SUPPORT_IODEBUG
class BOCHSAPI bx_iodebug_stub_c : public bx_devmodel_c {
public:
    virtual void mem_write(BX_CPU_C* cpu, bx_phy_address addr, unsigned len, void* data) {}
    virtual void mem_read(BX_CPU_C* cpu, bx_phy_address addr, unsigned len, void* data) {}
};
#endif
#if BX_SUPPORT_APIC
class BOCHSAPI bx_ioapic_stub_c : public bx_devmodel_c {
public:

    virtual void receive_eoi(Bit8u vector) {}//359
};
#endif

#if BX_SUPPORT_GAMEPORT
class BOCHSAPI bx_game_stub_c : public bx_devmodel_c {
public:

};
#endif

class BOCHSAPI bx_devices_c {
    //374-614
public:
	bx_devices_c();
	~bx_devices_c();
    void init_stubs(void);
    void init(BX_MEM_C*);//383
    BX_MEM_C* mem;//392
    bool register_io_read_handler(void* this_ptr, bx_read_handler_t f,
        Bit32u addr, const char* name, Bit8u mask); //393
    bool register_io_write_handler(void* this_ptr, bx_write_handler_t f,
        Bit32u addr, const char* name, Bit8u mask); //398
    bool register_default_io_read_handler(void* this_ptr, bx_read_handler_t f, const char* name, Bit8u mask);//411
    bool register_default_io_write_handler(void* this_ptr, bx_write_handler_t f, const char* name, Bit8u mask);//412
    bool register_irq(unsigned irq, const char* name); //413
    void register_default_keyboard(void* dev, bx_kbd_gen_scancode_t kbd_gen_scancode,
        bx_kbd_get_elements_t kbd_get_elements); //414
    Bit32u inp(Bit16u addr, unsigned io_len) BX_CPP_AttrRegparmN(2);
    void   outp(Bit16u addr, Bit32u value, unsigned io_len) BX_CPP_AttrRegparmN(3); //416
    void register_default_mouse(void* dev, bx_mouse_enq_t mouse_enq, bx_mouse_enabled_changed_t mouse_enabled_changed); //424
    void kbd_set_indicator(Bit8u devid, Bit8u ledid, bool state); //431
    bx_cmos_stub_c* pluginCmosDevice;
    bx_dma_stub_c* pluginDmaDevice;
    bx_hard_drive_stub_c* pluginHardDrive;
    bx_pic_stub_c* pluginPicDevice;
    bx_pit_stub_c* pluginPitDevice;
    bx_speaker_stub_c* pluginSpeaker;
    bx_vga_stub_c* pluginVgaDevice;
#if BX_SUPPORT_IODEBUG
    bx_iodebug_stub_c* pluginIODebug;
#endif
#if BX_SUPPORT_APIC
    bx_ioapic_stub_c* pluginIOAPIC;
#endif
#if BX_SUPPORT_GAMEPORT
    bx_game_stub_c* pluginGameport;
#endif
#if BX_SUPPORT_PCI
    bx_pci2isa_stub_c* pluginPci2IsaBridge;
    bx_pci_ide_stub_c* pluginPciIdeController;
    bx_acpi_ctrl_stub_c* pluginACPIController;
#endif

    bx_cmos_stub_c stubCmos;  //477
    bx_dma_stub_c  stubDma;
    bx_hard_drive_stub_c stubHardDrive;
    bx_pic_stub_c  stubPic;
    bx_pit_stub_c  stubPit;
    bx_speaker_stub_c stubSpeaker;
    bx_vga_stub_c  stubVga;
#if BX_SUPPORT_IODEBUG
    bx_iodebug_stub_c stubIODebug;
#endif
#if BX_SUPPORT_APIC
    bx_ioapic_stub_c stubIOAPIC;
#endif
#if BX_SUPPORT_GAMEPORT
    bx_game_stub_c stubGameport;
#endif
#if BX_SUPPORT_PCI
    bx_pci2isa_stub_c stubPci2Isa;
    bx_pci_ide_stub_c stubPciIde;
    bx_acpi_ctrl_stub_c stubACPIController;
#endif
private:
    struct io_handler_struct { //511
        struct io_handler_struct* next;
        struct io_handler_struct* prev;
        void* funct; // C++ type checking is great, but annoying
        void* this_ptr;
        char* handler_name;  // name of device
        int usage_count;
        Bit8u mask;          // io_len mask
    };
    struct io_handler_struct io_read_handlers;//520
    struct io_handler_struct io_write_handlers;//521
#define PORTS 0x10000
    struct io_handler_struct** read_port_to_handler;
    struct io_handler_struct** write_port_to_handler; //524

    char* irq_handler_name[BX_MAX_IRQS]; //528
    static Bit32u default_read_handler(void* this_ptr, Bit32u address, unsigned io_len);//535
    static void   default_write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);//536
    
    
    struct { //545
        void* dev;
        bx_mouse_enq_t enq_event;
        bx_mouse_enabled_changed_t enabled_changed;
    } bx_mouse[2];

    struct {
        void* dev;
        bx_kbd_gen_scancode_t gen_scancode;
        bx_kbd_get_elements_t get_elements;
        Bit8u led_mask;
        bool bxkey_state[BX_KEY_NBKEYS];
    } bx_keyboard[2];
    int timer_handle;
    int statusbar_id[3];
    Bit8u sound_device_count; //610
};


BOCHSAPI extern bx_devices_c bx_devices;