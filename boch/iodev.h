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

#if BX_USE_DEV_SMF
#  define BX_DEV_SMF  static
#  define BX_DEV_THIS bx_devices.
#else
#  define BX_DEV_SMF
#  define BX_DEV_THIS this->
#endif

class BOCHSAPI bx_devmodel_c {
public:
    virtual ~bx_devmodel_c() {}
    virtual void init(void) {}
    virtual void reset(unsigned type) {}
    virtual void register_state(void) {}
    virtual void after_restore_state(void) {}
};

class bx_list_c;
class device_image_t;
class cdrom_base_c;

#if BX_SUPPORT_PCI
#define BX_DEBUG_PCI_READ(addr, value, io_len) do {} while (0)
#define BX_DEBUG_PCI_WRITE(addr, value, io_len) do {} while (0)

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

    virtual Bit32u pci_read_handler(Bit8u address, unsigned io_len);
    void pci_write_handler_common(Bit8u address, Bit32u value, unsigned io_len);
    virtual void pci_write_handler(Bit8u address, Bit32u value, unsigned io_len) {}
    virtual void pci_bar_change_notify(void) {}

    void init_pci_conf(Bit16u vid, Bit16u did, Bit8u rev, Bit32u classc,
        Bit8u headt, Bit8u intpin);
    void init_bar_io(Bit8u num, Bit16u size, bx_read_handler_t rh,
        bx_write_handler_t wh, const Bit8u* mask);
    void init_bar_mem(Bit8u num, Bit32u size, memory_handler_t rh, memory_handler_t wh);
    void register_pci_state(bx_list_c* list);
    void after_restore_pci_state(memory_handler_t mem_read_handler);
    void load_pci_rom(const char* path);
    void set_name(const char* name) { pci_name = name; }
    const char* get_name(void) { return pci_name; }

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
#define STUBFUNC(dev,method) do {} while (0)


class BOCHSAPI bx_hard_drive_stub_c : public bx_devmodel_c {
    //196
public:
    virtual Bit32u virt_read_handler(Bit32u address, unsigned io_len) { return 0; }
    virtual void virt_write_handler(Bit32u address, Bit32u value, unsigned io_len) {}

    virtual bool bmdma_read_sector(Bit8u channel, Bit8u* buffer, Bit32u* sector_size) {
        //STUBFUNC(HD, bmdma_read_sector); 
         return 0;
    }
    virtual bool bmdma_write_sector(Bit8u channel, Bit8u* buffer) {
        //STUBFUNC(HD, bmdma_write_sector); 
        return 0;
    }
    virtual void bmdma_complete(Bit8u channel) {
        //STUBFUNC(HD, bmdma_complete);
    }
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
    virtual void enable_irq(bool enabled) {}
};
class BOCHSAPI bx_dma_stub_c : public bx_devmodel_c {
    //235
public:
    virtual bool registerDMA8Channel(
        unsigned channel,
        Bit16u(*dmaRead)(Bit8u* data_byte, Bit16u maxlen),
        Bit16u(*dmaWrite)(Bit8u* data_byte, Bit16u maxlen),
        const char* name)
    {
        //STUBFUNC(dma, registerDMA8Channel); 
        return false;
    }
    virtual bool registerDMA16Channel(
        unsigned channel,
        Bit16u(*dmaRead)(Bit16u* data_word, Bit16u maxlen),
        Bit16u(*dmaWrite)(Bit16u* data_word, Bit16u maxlen),
        const char* name)
    {
        //STUBFUNC(dma, registerDMA16Channel); 
        return false;
    }
    virtual bool unregisterDMAChannel(unsigned channel) {
        //STUBFUNC(dma, unregisterDMAChannel); 
         return false;
    }
    virtual bool get_TC(void) {
        //STUBFUNC(dma, get_TC); 
        return false;
    }
    virtual void set_DRQ(unsigned channel, bool val) {
        //STUBFUNC(dma, set_DRQ);
    }
    virtual void raise_HLDA(void) {
        //STUBFUNC(dma, raise_HLDA);
    }
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
public:
    virtual void vga_redraw_area(unsigned x0, unsigned y0, unsigned width,
        unsigned height) {}

    virtual Bit8u mem_read(bx_phy_address addr) {
        return 0;
    }

    virtual void mem_write(bx_phy_address addr, Bit8u value) {}

    virtual void get_text_snapshot(Bit8u** text_snapshot,
        unsigned* txHeight, unsigned* txWidth) {
        if (text_snapshot != NULL) {
            *text_snapshot = NULL;
        }
        if (txHeight != NULL) {
            *txHeight = 0;
        }
        if (txWidth != NULL) {
            *txWidth = 0;
        }
    }

    virtual void set_override(bool enabled, void* dev) {}

    virtual void refresh_display(void* this_ptr, bool redraw) {}
};
class BOCHSAPI bx_speaker_stub_c : public bx_devmodel_c {
public:
    virtual void beep_on(float frequency) {
        bx_gui->beep_on(frequency);
    }
    virtual void beep_off() {
        bx_gui->beep_off();
    }
    virtual void set_line(bool level) {}
};

#if BX_SUPPORT_PCI
class BOCHSAPI bx_pci2isa_stub_c : public bx_pci_device_c {
public:
public:
    virtual void pci_set_irq(Bit8u devfunc, unsigned line, bool level) {
        //STUBFUNC(pci2isa, pci_set_irq);
    }
};

class BOCHSAPI bx_pci_ide_stub_c : public bx_pci_device_c {
public:
    virtual bool bmdma_present(void) {
        return 0;
    }
    virtual void bmdma_start_transfer(Bit8u channel) {}
    virtual void bmdma_set_irq(Bit8u channel) {}
};

class BOCHSAPI bx_acpi_ctrl_stub_c : public bx_pci_device_c {
public:
    virtual void generate_smi(Bit8u value) {}
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
    virtual void set_enabled(bool enabled, Bit16u base_offset) {}
    virtual void receive_eoi(Bit8u vector) {}//359
    virtual void set_irq_level(Bit8u int_in, bool level) {}
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
    void reset(unsigned type);
    void exit(void);
    void register_state(void);
    void after_restore_state(void);

    BX_MEM_C* mem;//392
    bool register_io_read_handler(void* this_ptr, bx_read_handler_t f,
        Bit32u addr, const char* name, Bit8u mask); //393
    bool unregister_io_read_handler(void* this_ptr, bx_read_handler_t f,
        Bit32u addr, Bit8u mask);
    bool register_io_write_handler(void* this_ptr, bx_write_handler_t f,
        Bit32u addr, const char* name, Bit8u mask); //398
    bool unregister_io_write_handler(void* this_ptr, bx_write_handler_t f,
        Bit32u addr, Bit8u mask);
    bool register_io_read_handler_range(void* this_ptr, bx_read_handler_t f,
        Bit32u begin_addr, Bit32u end_addr,
        const char* name, Bit8u mask);
    bool register_io_write_handler_range(void* this_ptr, bx_write_handler_t f,
        Bit32u begin_addr, Bit32u end_addr,
        const char* name, Bit8u mask);
    bool unregister_io_read_handler_range(void* this_ptr, bx_read_handler_t f,
        Bit32u begin, Bit32u end, Bit8u mask);
    bool unregister_io_write_handler_range(void* this_ptr, bx_write_handler_t f,
        Bit32u begin, Bit32u end, Bit8u mask);
    bool register_default_io_read_handler(void* this_ptr, bx_read_handler_t f, const char* name, Bit8u mask);//411
    bool register_default_io_write_handler(void* this_ptr, bx_write_handler_t f, const char* name, Bit8u mask);//412
    bool register_irq(unsigned irq, const char* name); //413
    bool unregister_irq(unsigned irq, const char* name);//414
    Bit32u inp(Bit16u addr, unsigned io_len) BX_CPP_AttrRegparmN(2);
    void   outp(Bit16u addr, Bit32u value, unsigned io_len) BX_CPP_AttrRegparmN(3); //416

    void register_default_keyboard(void* dev, bx_kbd_gen_scancode_t kbd_gen_scancode,
        bx_kbd_get_elements_t kbd_get_elements); //414
    void register_removable_keyboard(void* dev, bx_kbd_gen_scancode_t kbd_gen_scancode,
        bx_kbd_get_elements_t kbd_get_elements,
        Bit8u led_mask);
    void unregister_removable_keyboard(void* dev);
    
    void register_default_mouse(void* dev, bx_mouse_enq_t mouse_enq, bx_mouse_enabled_changed_t mouse_enabled_changed); //424
    void register_removable_mouse(void* dev, bx_mouse_enq_t mouse_enq, bx_mouse_enabled_changed_t mouse_enabled_changed);
    void unregister_removable_mouse(void* dev);
    void gen_scancode(Bit32u key);//427
    Bit8u kbd_get_elements(void);
    void release_keys(void);//429
    void paste_bytes(Bit8u* data, Bit32s length);
    void kbd_set_indicator(Bit8u devid, Bit8u ledid, bool state); //431
    void mouse_enabled_changed(bool enabled);
    void mouse_motion(int delta_x, int delta_y, int delta_z, unsigned button_state, bool absxy);
    void add_sound_device(void);
    void remove_sound_device(void);//435
#if BX_SUPPORT_PCI
    Bit32u pci_get_confAddr(void) { return pci.confAddr; }
    Bit32u pci_get_slot_mapping(void) { return pci.map_slot_to_dev; }
    bool register_pci_handlers(bx_pci_device_c* device, Bit8u* devfunc,
        const char* name, const char* descr, Bit8u bus = 0);
    bool pci_set_base_mem(void* this_ptr, memory_handler_t f1, memory_handler_t f2,
        Bit32u* addr, Bit8u* pci_conf, unsigned size);
    bool pci_set_base_io(void* this_ptr, bx_read_handler_t f1, bx_write_handler_t f2,
        Bit32u* addr, Bit8u* pci_conf, unsigned size,
        const Bit8u* iomask, const char* name);
#endif
    bool is_agp_present();
    static void timer_handler(void*);
    void timer(void);

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
    Bit8u* bulkIOHostAddr;
    unsigned bulkIOQuantumsRequested;
    unsigned bulkIOQuantumsTransferred;

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

    static Bit32u read_handler(void* this_ptr, Bit32u address, unsigned io_len);
    static void   write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);

    static Bit32u default_read_handler(void* this_ptr, Bit32u address, unsigned io_len);//535
    static void   default_write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);//536
    
    void paste_delay_changed(Bit32u value);
    void service_paste_buf(void);

    bool mouse_captured;
    Bit8u mouse_type;

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

    struct {
        Bit8u* buf;     // ptr to bytes to be pasted, or NULL if none in progress
        Bit32u buf_len; // length of pastebuf
        Bit32u buf_ptr; // ptr to next byte to be added to hw buffer
        Bit32u delay;   // number of timer events before paste
        Bit32u counter; // count before paste
        bool service;   // set to 1 when gen_scancode() is called from paste service
        bool stop;      // stop the current paste operation on keypress or hardware reset
    } paste;

    struct {
        bool enabled;

#if BX_SUPPORT_PCI
        Bit32u advopts;
        Bit8u handler_id[0x101];  // 256 PCI devices/functions + 1 AGP device
        struct {
            bx_pci_device_c* handler;
        } pci_handler[BX_MAX_PCI_DEVICES];
        unsigned num_pci_handlers;

        Bit8u map_slot_to_dev;
        bool slot_used[BX_N_PCI_SLOTS];

        Bit32u confAddr;
#endif
    } pci;
    int timer_handle;
    int statusbar_id[3];
    Bit8u sound_device_count; //610
};

BX_CPP_INLINE void DEV_MEM_READ_PHYSICAL(bx_phy_address phy_addr, unsigned len, Bit8u* ptr)
{
    unsigned remainingInPage = 0x1000 - (phy_addr & 0xfff);
    if (len <= remainingInPage) {
        BX_MEM(0)->readPhysicalPage(NULL, phy_addr, len, ptr);
    }
    else {
        BX_MEM(0)->readPhysicalPage(NULL, phy_addr, remainingInPage, ptr);
        ptr += remainingInPage;
        phy_addr += remainingInPage;
        len -= remainingInPage;
        BX_MEM(0)->readPhysicalPage(NULL, phy_addr, len, ptr);
    }
}

BX_CPP_INLINE void DEV_MEM_READ_PHYSICAL_DMA(bx_phy_address phy_addr, unsigned len, Bit8u* ptr)
{
    while (len > 0) {
        unsigned remainingInPage = 0x1000 - (phy_addr & 0xfff);
        if (len < remainingInPage) remainingInPage = len;
        BX_MEM(0)->dmaReadPhysicalPage(phy_addr, remainingInPage, ptr);
        ptr += remainingInPage;
        phy_addr += remainingInPage;
        len -= remainingInPage;
    }
}

BX_CPP_INLINE void DEV_MEM_WRITE_PHYSICAL(bx_phy_address phy_addr, unsigned len, Bit8u* ptr)
{
    unsigned remainingInPage = 0x1000 - (phy_addr & 0xfff);
    if (len <= remainingInPage) {
        BX_MEM(0)->writePhysicalPage(NULL, phy_addr, len, ptr);
    }
    else {
        BX_MEM(0)->writePhysicalPage(NULL, phy_addr, remainingInPage, ptr);
        ptr += remainingInPage;
        phy_addr += remainingInPage;
        len -= remainingInPage;
        BX_MEM(0)->writePhysicalPage(NULL, phy_addr, len, ptr);
    }
}

BX_CPP_INLINE void DEV_MEM_WRITE_PHYSICAL_DMA(bx_phy_address phy_addr, unsigned len, Bit8u* ptr)
{
    while (len > 0) {
        unsigned remainingInPage = 0x1000 - (phy_addr & 0xfff);
        if (len < remainingInPage) remainingInPage = len;
        BX_MEM(0)->dmaWritePhysicalPage(phy_addr, remainingInPage, ptr);
        ptr += remainingInPage;
        phy_addr += remainingInPage;
        len -= remainingInPage;
    }
}

BOCHSAPI extern bx_devices_c bx_devices;
