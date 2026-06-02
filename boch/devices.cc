#define _CRT_SECURE_NO_WARNINGS
#include "iodev.h"
#include "virt_timer.h"
#include "instrument.h"
#include "soundmod.h"
#include "debug.h"
#define LOG_THIS bx_devices.

/* main memory size (in Kbytes)
 * subtract 1k for extended BIOS area
 * report only base memory, not extended mem
 */
#define BASE_MEMORY_IN_K  640

bx_devices_c bx_devices;

bx_devices_c::bx_devices_c()
{
    //put("devices", "DEV");

    read_port_to_handler = NULL;
    write_port_to_handler = NULL;
    io_read_handlers.next = NULL;
    io_read_handlers.handler_name = NULL;
    io_write_handlers.next = NULL;
    io_write_handlers.handler_name = NULL;
    init_stubs();

    for (unsigned i = 0; i < BX_MAX_IRQS; i++) {
        irq_handler_name[i] = NULL;
    }
    timer_handle = BX_NULL_TIMER_HANDLE;
    mouse_captured = false;
    mouse_type = 0;
    paste.buf = NULL;
    paste.buf_len = 0;
    paste.buf_ptr = 0;
    paste.delay = 1;
    paste.counter = 0;
    paste.service = 0;
    paste.stop = 0;
    sound_device_count = 0;
}

bx_devices_c::~bx_devices_c()
{

    timer_handle = BX_NULL_TIMER_HANDLE;
    //bx_hdimage_ctl.exit();
#if BX_NETWORKING
    //bx_netmod_ctl.exit();
#endif
#if BX_SUPPORT_SOUNDLOW
    bx_soundmod_ctl.exit();
#endif
#if BX_SUPPORT_PCIUSB
    //bx_usbdev_ctl.exit();
#endif
    if (bx_gui != NULL) {
        delete bx_gui;
        bx_gui = NULL;
    }

}

void bx_devices_c::init_stubs()
{
    pluginCmosDevice = &stubCmos;
    pluginDmaDevice = &stubDma;
    pluginHardDrive = &stubHardDrive;
    pluginPicDevice = &stubPic;
    pluginPitDevice = &stubPit;
    pluginSpeaker = &stubSpeaker;
    pluginVgaDevice = &stubVga;
#if BX_SUPPORT_IODEBUG
    pluginIODebug = &stubIODebug;
#endif
#if BX_SUPPORT_APIC
    pluginIOAPIC = &stubIOAPIC;
#endif
#if BX_SUPPORT_GAMEPORT
    pluginGameport = &stubGameport;
#endif
#if BX_SUPPORT_PCI
    pluginPci2IsaBridge = &stubPci2Isa;
    pluginPciIdeController = &stubPciIde;
    pluginACPIController = &stubACPIController;
#endif
}

void bx_devices_c::init(BX_MEM_C* newmem)
{
    //110-382
#if BX_SUPPORT_PCI
    // Source:
    // unsigned chipset = SIM->get_param_enum(BXPN_PCI_CHIPSET)->get();
    // SIM处理：直接赋值。1 = BX_PCI_CHIPSET_I440FX
    unsigned chipset = 1;
    unsigned max_pci_slots = BX_N_PCI_SLOTS;
#endif

#if BX_SUPPORT_PCIUSB
    bool load_uhci = false;
#endif

    unsigned i, argc;
    const char def_name[] = "Default";
    const char* options;
    char* argv[16];

    mem = newmem;
    if (bx_gui == NULL) {
        bx_gui = new bx_gui_c();
    }

    /* set builtin default handlers, will be overwritten by the real default handler */
    register_default_io_read_handler(NULL, &default_read_handler, def_name, 7);
    io_read_handlers.next = &io_read_handlers;
    io_read_handlers.prev = &io_read_handlers;
    io_read_handlers.usage_count = 0; // not used with the default handler

    register_default_io_write_handler(NULL, &default_write_handler, def_name, 7);
    io_write_handlers.next = &io_write_handlers;
    io_write_handlers.prev = &io_write_handlers;
    io_write_handlers.usage_count = 0; // not used with the default handler

    read_port_to_handler = new struct io_handler_struct* [PORTS];
    write_port_to_handler = new struct io_handler_struct* [PORTS];

    /* set handlers to the default one */
    for (i = 0; i < PORTS; i++) {
        read_port_to_handler[i] = &io_read_handlers;
        write_port_to_handler[i] = &io_write_handlers;
    }

    // removable devices init
    for (i = 0; i < 2; i++) {
        bx_keyboard[i].dev = NULL;
        bx_keyboard[i].gen_scancode = NULL;
        bx_keyboard[i].led_mask = 0;
    }
    for (i = 0; i < BX_KEY_NBKEYS; i++) {
        bx_keyboard[0].bxkey_state[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        bx_mouse[i].dev = NULL;
        bx_mouse[i].enq_event = NULL;
        bx_mouse[i].enabled_changed = NULL;
    }
    mouse_captured = true;
    mouse_type = 0;
    paste.buf = NULL;
    paste.buf_len = 0;
    paste.buf_ptr = 0;
    paste.delay = 1;
    paste.counter = 0;
    paste.service = 0;
    paste.stop = 0;

    // Source: devices init early initializes virtual timers here.
    // 需要先补 bx_virt_timer_c::init()，否则这行会编译不过。
    bx_virt_timer.init();

    // Source also has bx_slowdown_timer.init().
    // 当前裁剪项目没有 slowdown_timer.cc/.h，先不打开。
    // bx_slowdown_timer.init();
    // 
    // Source:
    // pci.enabled = SIM->get_param_bool(BXPN_PCI_ENABLED)->get();
    //
    // SIM 处理：当前要对齐 PCI I/O 配置端口，所以直接启用。
    pci.enabled = true;

    if (pci.enabled) {
#if BX_SUPPORT_PCI
        if (chipset == BX_PCI_CHIPSET_I430FX) {
            pci.advopts = (BX_PCI_ADVOPT_NOHPET | BX_PCI_ADVOPT_NOACPI | BX_PCI_ADVOPT_NOAGP);
        }
        else if (chipset == BX_PCI_CHIPSET_I440FX) {
            pci.advopts = BX_PCI_ADVOPT_NOAGP;
        }
        else {
            pci.advopts = 0;
        }

        // Source also parses BXPN_PCI_ADV_OPTS from SIM here.
        // SIM 处理：先跳过，使用上面固定的 pci.advopts。
        PLUG_load_plugin(pci, PLUGTYPE_CORE);
        PLUG_load_plugin(pci2isa, PLUGTYPE_CORE);
    }
#if BX_SUPPORT_PCIUSB
    if ((chipset == BX_PCI_CHIPSET_I440FX) ||
        (chipset == BX_PCI_CHIPSET_I440BX)) {
        // UHCI is a part of the PIIX3/PIIX4, so load it later in the visible chain.
        load_uhci = true;
    }
#endif
    if ((pci.advopts & BX_PCI_ADVOPT_NOACPI) == 0) {
        PLUG_load_plugin(acpi, PLUGTYPE_STANDARD);
    }

    if ((pci.advopts & BX_PCI_ADVOPT_NOHPET) == 0) {
        PLUG_load_plugin(hpet, PLUGTYPE_STANDARD);
    }

#endif
    
    PLUG_load_plugin(cmos, PLUGTYPE_CORE); //238

    PLUG_load_plugin(dma, PLUGTYPE_CORE); //239

    PLUG_load_plugin(pic, PLUGTYPE_CORE);//240

    PLUG_load_plugin(pit, PLUGTYPE_CORE);

    if (pluginVgaDevice == &stubVga) {
        PLUG_load_plugin(vga, PLUGTYPE_VGA);
    }

    PLUG_load_plugin(floppy, PLUGTYPE_CORE);

#if BX_SUPPORT_APIC
    PLUG_load_plugin(ioapic, PLUGTYPE_STANDARD);
#endif

    PLUG_load_plugin(keyboard, PLUGTYPE_STANDARD); //250

    const bool harddrv_enabled = true;

    if (harddrv_enabled) {
        PLUG_load_plugin(harddrv, PLUGTYPE_STANDARD);
#if BX_SUPPORT_PCI
        if (pci.enabled) {
            PLUG_load_plugin(pci_ide, PLUGTYPE_STANDARD);
        }
#endif
    }

    PLUG_load_plugin(biosdev, PLUGTYPE_OPTIONAL);

#if BX_SUPPORT_PCIUSB
    if (load_uhci) {
        // UHCI is a part of the PIIX3/PIIX4, so load / enable it
        if (!PLUG_device_present("usb_uhci")) {
            PLUG_load_plugin(usb_uhci, PLUGTYPE_OPTIONAL);
        }
        
    }
#endif

    PLUG_load_plugin(parallel, PLUGTYPE_OPTIONAL); //自己加的按 PLUGTYPE_OPTIONAL 加载，不按 CORE

    PLUG_load_plugin(serial, PLUGTYPE_OPTIONAL);//同parallel一样

    PLUG_load_plugin(speaker, PLUGTYPE_OPTIONAL);

    register_io_read_handler(this, &read_handler, 0x0092,
        "Port 92h System Control", 1);
    register_io_write_handler(this, &write_handler, 0x0092,
        "Port 92h System Control", 1);
    
#if BX_SUPPORT_PCI
    if (pci.enabled) {
        pci.num_pci_handlers = 0;

        /* set unused elements to appropriate values */
        for (i = 0; i < BX_MAX_PCI_DEVICES; i++) {
            pci.pci_handler[i].handler = NULL;
        }

        for (i = 0; i < 0x101; i++) {
            pci.handler_id[i] = BX_MAX_PCI_DEVICES;  // not assigned
        }

        for (i = 0; i < BX_N_PCI_SLOTS; i++) {
            pci.slot_used[i] = false;  // no device connected
        }

        if (chipset == BX_PCI_CHIPSET_I440BX) {
            pci.map_slot_to_dev = 8;
        }
        else {
            pci.map_slot_to_dev = 2;
        }

        // confAddr accepts dword i/o only
        DEV_register_ioread_handler(this, read_handler, 0x0CF8, "PCI confAddr", 4);
        DEV_register_iowrite_handler(this, write_handler, 0x0CF8, "PCI confAddr", 4);

        for (i = 0x0CFC; i <= 0x0CFF; i++) {
            DEV_register_ioread_handler(this, read_handler, i, "PCI confData", 7);
            DEV_register_iowrite_handler(this, write_handler, i, "PCI confData", 7);
        }
    }
#endif

    // misc. CMOS memory size
    const Bit64u base_memory_in_k = 640;
    Bit64u memory_in_k = mem->get_memory_len() / 1024;

    Bit64u extended_memory_in_k = memory_in_k > 1024 ? (memory_in_k - 1024) : 0;
    if (extended_memory_in_k > 0xfc00)
        extended_memory_in_k = 0xfc00;

    DEV_cmos_set_reg(0x15, (Bit8u)(base_memory_in_k & 0xff));
    DEV_cmos_set_reg(0x16, (Bit8u)((base_memory_in_k >> 8) & 0xff));
    DEV_cmos_set_reg(0x17, (Bit8u)(extended_memory_in_k & 0xff));
    DEV_cmos_set_reg(0x18, (Bit8u)((extended_memory_in_k >> 8) & 0xff));
    DEV_cmos_set_reg(0x30, (Bit8u)(extended_memory_in_k & 0xff));
    DEV_cmos_set_reg(0x31, (Bit8u)((extended_memory_in_k >> 8) & 0xff));

    Bit64u extended_memory_in_64k =
        memory_in_k > 16384 ? (memory_in_k - 16384) / 64 : 0;
    if (extended_memory_in_64k > 0xbf00)
        extended_memory_in_64k = 0xbf00;

    DEV_cmos_set_reg(0x34, (Bit8u)(extended_memory_in_64k & 0xff));
    DEV_cmos_set_reg(0x35, (Bit8u)((extended_memory_in_64k >> 8) & 0xff));


    bx_init_plugins(); //354
    DEV_cmos_checksum();//357
}

void bx_devices_c::reset(unsigned type)
{
#if BX_SUPPORT_PCI
    if (pci.enabled) {
        pci.confAddr = 0;
    }
#endif
    mem->disable_smram();
    bx_reset_plugins(type);
    release_keys();
    if (paste.buf != NULL) {
        paste.stop = 1;
    }
}

void bx_devices_c::register_state(void)
{
}

void bx_devices_c::after_restore_state(void)
{
}

void bx_devices_c::exit(void)
{
    if (paste.buf != NULL) {
        delete[] paste.buf;
        paste.buf = NULL;
    }
    init_stubs();
}

Bit32u bx_devices_c::read_handler(void* this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_DEV_SMF
    bx_devices_c* class_ptr = (bx_devices_c*)this_ptr;
    return class_ptr->read(address, io_len);
}

Bit32u bx_devices_c::read(Bit32u address, unsigned io_len)
{
#else
    UNUSED(this_ptr);
#endif  // !BX_USE_DEV_SMF

    switch (address) {
    case 0x0092:
        //BX_DEBUG(("port92h read partially supported!!!"));
        //BX_DEBUG(("  returning %02x", (unsigned)(BX_GET_ENABLE_A20() << 1)));
        return(BX_GET_ENABLE_A20() << 1);
#if BX_SUPPORT_PCI
    case 0x0CF8:
        return BX_DEV_THIS pci.confAddr;
    case 0x0CFC:
    case 0x0CFD:
    case 0x0CFE:
    case 0x0CFF:
    {
        Bit32u handle, retval = 0xffffffff;
        Bit8u regnum;
        Bit16u bus_devfunc;

        if ((BX_DEV_THIS pci.confAddr & 0x80fe0000) == 0x80000000) {
            bus_devfunc = (BX_DEV_THIS pci.confAddr >> 8) & 0x1ff;
            regnum = (BX_DEV_THIS pci.confAddr & 0xfc) + (address & 0x03);
            if (bus_devfunc <= 0x100) {
                handle = BX_DEV_THIS pci.handler_id[bus_devfunc];
                if ((io_len <= 4) && (handle < BX_MAX_PCI_DEVICES)) {
                    retval = BX_DEV_THIS pci.pci_handler[handle].handler->pci_read_handler(regnum, io_len);
                }
            }
        }
        return retval;
    }
#endif
    }

    //BX_PANIC(("unsupported IO read to port 0x%x", (unsigned)address));
    return(0xffffffff);
}

void bx_devices_c::write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_DEV_SMF
    bx_devices_c* class_ptr = (bx_devices_c*)this_ptr;
    class_ptr->write(address, value, io_len);
}

void bx_devices_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
    UNUSED(this_ptr);
#endif  // !BX_USE_DEV_SMF
#if BX_SUPPORT_PCI
    Bit8u bus, devfunc, handle;
    Bit16u bus_devfunc;
    bx_pci_device_c* dev = NULL;
#endif

    switch (address) {
    case 0x0092:
        //BX_DEBUG(("port92h write of %02x partially supported!!!", (unsigned)value));
        //BX_DEBUG(("A20: set_enable_a20() called"));
        BX_SET_ENABLE_A20((value & 0x02) >> 1);
        //BX_DEBUG(("A20: now %u", (unsigned)BX_GET_ENABLE_A20()));
        if (value & 0x01) { /* high speed reset */
            //BX_INFO(("iowrite to port0x92 : reset resquested"));
            bx_pc_system.Reset(BX_RESET_SOFTWARE);
        }
        break;
#if BX_SUPPORT_PCI
    case 0xCF8:
        BX_DEV_THIS pci.confAddr = value;
        if ((value & 0x80000000) == 0x80000000) {
            bus = (BX_DEV_THIS pci.confAddr >> 16) & 0xff;
            devfunc = (BX_DEV_THIS pci.confAddr >> 8) & 0xff;
            bus_devfunc = (bus << 8) | devfunc;
            if (bus_devfunc <= 0x100) {
                handle = BX_DEV_THIS pci.handler_id[bus_devfunc];
                if (handle != BX_MAX_PCI_DEVICES) {
                    dev = BX_DEV_THIS pci.pci_handler[handle].handler;
                }
            }
            if ((bus == 0) && (devfunc == 0x00)) {
                //BX_DEBUG(("%s register 0x%02x selected", dev->get_name(), value & 0xfc));
            }
            else if (dev != NULL) {
                //BX_DEBUG(("PCI: request for bus %d device %d function %d (%s)", bus,
                    //(devfunc >> 3), devfunc & 0x07, dev->get_name()));
            }
            else if (bus == 1) {
               // BX_DEBUG(("PCI: request for AGP bus device %d function %d", (devfunc >> 3),
                    //devfunc & 0x07));
            }
            else {
                //BX_DEBUG(("PCI: request for bus %d device %d function %d", bus,
                    //(devfunc >> 3), devfunc & 0x07));
            }
        }
        break;

    case 0xCFC:
    case 0xCFD:
    case 0xCFE:
    case 0xCFF:
        if ((BX_DEV_THIS pci.confAddr & 0x80fe0000) == 0x80000000) {
            bus_devfunc = (BX_DEV_THIS pci.confAddr >> 8) & 0x1ff;
            Bit8u regnum = (BX_DEV_THIS pci.confAddr & 0xfc) + (address & 0x03);
            if (bus_devfunc <= 0x100) {
                handle = BX_DEV_THIS pci.handler_id[bus_devfunc];
                if ((io_len <= 4) && (handle < BX_MAX_PCI_DEVICES)) {
                    BX_DEV_THIS pci.pci_handler[handle].handler->pci_write_handler_common(regnum, value, io_len);
                }
            }
        }
        break;
#endif
    default:
        //BX_PANIC(("IO write to port 0x%x", (unsigned)address));
        break;
    }
}

Bit32u bx_devices_c::default_read_handler(void* this_ptr, Bit32u address, unsigned io_len)
{    //602
    UNUSED(this_ptr);
    return 0xffffffff;
}

void bx_devices_c::default_write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
    //610
    UNUSED(this_ptr);
}

void bx_devices_c::timer_handler(void* this_ptr)
{
    bx_devices_c* class_ptr = (bx_devices_c*)this_ptr;
    class_ptr->timer();
}

void bx_devices_c::timer(void)
{
    if (++paste.counter >= paste.delay) {
        service_paste_buf();
        paste.counter = 0;
    }
}

bool bx_devices_c::register_irq(unsigned irq, const char* name)
{
    //634
    if (irq >= BX_MAX_IRQS) {
        //BX_PANIC(("IO device %s registered with IRQ=%d above %u",
            //name, irq, (unsigned)BX_MAX_IRQS - 1));
        return false;
    }
    if (irq_handler_name[irq]) {
        //BX_PANIC(("IRQ %u conflict, %s with %s", irq, irq_handler_name[irq], name));
        return false;
    }
    irq_handler_name[irq] = new char[strlen(name) + 1];
    strcpy(irq_handler_name[irq], name);
    return true;
}

bool bx_devices_c::unregister_irq(unsigned irq, const char* name)
{
    if (irq >= BX_MAX_IRQS) {
        //BX_PANIC(("IO device %s tried to unregister IRQ %d above %u",
            //name, irq, (unsigned)BX_MAX_IRQS - 1));
        return false;
    }
    if (!irq_handler_name[irq]) {
        //BX_INFO(("IO device %s tried to unregister IRQ %d, not registered", name, irq));
        return false;
    }

    if (strcmp(irq_handler_name[irq], name)) {
        //BX_INFO(("IRQ %u not registered to %s but to %s", irq,
            //name, irq_handler_name[irq]));
        return false;
    }
    delete[] irq_handler_name[irq];
    irq_handler_name[irq] = NULL;
    return true;
}

bool bx_devices_c::register_io_read_handler(void* this_ptr, bx_read_handler_t f,
    Bit32u addr, const char* name, Bit8u mask)
{
    addr &= 0xffff;

    if (!f)
        return false;

    /* first check if the port already has a handlers != the default handler */
    if (read_port_to_handler[addr] &&
        read_port_to_handler[addr] != &io_read_handlers) { // the default
        //BX_ERROR(("IO device address conflict(read) at IO address %Xh", (unsigned)addr));
        //BX_ERROR(("  conflicting devices: %s & %s",
            //read_port_to_handler[addr]->handler_name, name));
        return false;
    }

    /* first find existing handle for function or create new one */
    struct io_handler_struct* curr = &io_read_handlers;
    struct io_handler_struct* io_read_handler = NULL;
    do {
        if (curr->funct == f &&
            curr->mask == mask &&
            curr->this_ptr == this_ptr &&
            !strcmp(curr->handler_name, name)) { // really want the same name too
            io_read_handler = curr;
            break;
        }
        curr = curr->next;
    } while (curr->next != &io_read_handlers);

    if (!io_read_handler) {
        io_read_handler = new struct io_handler_struct;
        io_read_handler->funct = (void*)f;
        io_read_handler->this_ptr = this_ptr;
        io_read_handler->handler_name = new char[strlen(name) + 1];
        strcpy(io_read_handler->handler_name, name);
        io_read_handler->mask = mask;
        io_read_handler->usage_count = 0;
        // add the handler to the double linked list of handlers
        io_read_handlers.prev->next = io_read_handler;
        io_read_handler->next = &io_read_handlers;
        io_read_handler->prev = io_read_handlers.prev;
        io_read_handlers.prev = io_read_handler;
    }

    io_read_handler->usage_count++;
    read_port_to_handler[addr] = io_read_handler;
    return true; // address mapped successfully
}

bool bx_devices_c::register_io_write_handler(void* this_ptr, bx_write_handler_t f,
    Bit32u addr, const char* name, Bit8u mask)
{
    addr &= 0xffff;

    if (!f)
        return false;

    /* first check if the port already has a handlers != the default handler */
    if (write_port_to_handler[addr] &&
        write_port_to_handler[addr] != &io_write_handlers) { // the default
        //BX_ERROR(("IO device address conflict(write) at IO address %Xh", (unsigned)addr));
        //BX_ERROR(("  conflicting devices: %s & %s",
            //write_port_to_handler[addr]->handler_name, name));
        return false;
    }

    /* first find existing handle for function or create new one */
    struct io_handler_struct* curr = &io_write_handlers;
    struct io_handler_struct* io_write_handler = NULL;
    do {
        if (curr->funct == f &&
            curr->mask == mask &&
            curr->this_ptr == this_ptr &&
            !strcmp(curr->handler_name, name)) { // really want the same name too
            io_write_handler = curr;
            break;
        }
        curr = curr->next;
    } while (curr->next != &io_write_handlers);

    if (!io_write_handler) {
        io_write_handler = new struct io_handler_struct;
        io_write_handler->funct = (void*)f;
        io_write_handler->this_ptr = this_ptr;
        io_write_handler->handler_name = new char[strlen(name) + 1];
        strcpy(io_write_handler->handler_name, name);
        io_write_handler->mask = mask;
        io_write_handler->usage_count = 0;
        // add the handler to the double linked list of handlers
        io_write_handlers.prev->next = io_write_handler;
        io_write_handler->next = &io_write_handlers;
        io_write_handler->prev = io_write_handlers.prev;
        io_write_handlers.prev = io_write_handler;
    }

    io_write_handler->usage_count++;
    write_port_to_handler[addr] = io_write_handler;
    return true; // address mapped successfully
}

bool bx_devices_c::register_io_read_handler_range(void* this_ptr, bx_read_handler_t f,
    Bit32u begin_addr, Bit32u end_addr,
    const char* name, Bit8u mask)
{
    Bit32u addr;
    begin_addr &= 0xffff;
    end_addr &= 0xffff;

    if (end_addr < begin_addr) {
        //BX_ERROR(("!!! end_addr < begin_addr !!!"));
        return false;
    }

    if (!f) {
        //BX_ERROR(("!!! f == NULL !!!"));
        return false;
    }

    /* first check if the port already has a handlers != the default handler */
    for (addr = begin_addr; addr <= end_addr; addr++) {
        if (read_port_to_handler[addr] &&
            read_port_to_handler[addr] != &io_read_handlers) { // the default
            //BX_ERROR(("IO device address conflict(read) at IO address %Xh",
                //(unsigned)addr));
            //BX_ERROR(("  conflicting devices: %s & %s",
                //read_port_to_handler[addr]->handler_name, name));
            return false;
        }
    }

    /* first find existing handle for function or create new one */
    struct io_handler_struct* curr = &io_read_handlers;
    struct io_handler_struct* io_read_handler = NULL;
    do {
        if (curr->funct == f &&
            curr->mask == mask &&
            curr->this_ptr == this_ptr &&
            !strcmp(curr->handler_name, name)) {
            io_read_handler = curr;
            break;
        }
        curr = curr->next;
    } while (curr->next != &io_read_handlers);

    if (!io_read_handler) {
        io_read_handler = new struct io_handler_struct;
        io_read_handler->funct = (void*)f;
        io_read_handler->this_ptr = this_ptr;
        io_read_handler->handler_name = new char[strlen(name) + 1];
        strcpy(io_read_handler->handler_name, name);
        io_read_handler->mask = mask;
        io_read_handler->usage_count = 0;
        // add the handler to the double linked list of handlers
        io_read_handlers.prev->next = io_read_handler;
        io_read_handler->next = &io_read_handlers;
        io_read_handler->prev = io_read_handlers.prev;
        io_read_handlers.prev = io_read_handler;
    }

    io_read_handler->usage_count += end_addr - begin_addr + 1;
    for (addr = begin_addr; addr <= end_addr; addr++)
        read_port_to_handler[addr] = io_read_handler;
    return true; // address mapped successfully
}

bool bx_devices_c::register_io_write_handler_range(void* this_ptr, bx_write_handler_t f,
    Bit32u begin_addr, Bit32u end_addr,
    const char* name, Bit8u mask)
{
    Bit32u addr;
    begin_addr &= 0xffff;
    end_addr &= 0xffff;

    if (end_addr < begin_addr) {
        //BX_ERROR(("!!! end_addr < begin_addr !!!"));
        return false;
    }

    if (!f) {
        //BX_ERROR(("!!! f == NULL !!!"));
        return false;
    }

    /* first check if the port already has a handlers != the default handler */
    for (addr = begin_addr; addr <= end_addr; addr++) {
        if (write_port_to_handler[addr] &&
            write_port_to_handler[addr] != &io_write_handlers) { // the default
            //BX_ERROR(("IO device address conflict(read) at IO address %Xh",
                //(unsigned)addr));
            //BX_ERROR(("  conflicting devices: %s & %s",
               // write_port_to_handler[addr]->handler_name, name));
            return false;
        }
    }

    /* first find existing handle for function or create new one */
    struct io_handler_struct* curr = &io_write_handlers;
    struct io_handler_struct* io_write_handler = NULL;
    do {
        if (curr->funct == f &&
            curr->mask == mask &&
            curr->this_ptr == this_ptr &&
            !strcmp(curr->handler_name, name)) {
            io_write_handler = curr;
            break;
        }
        curr = curr->next;
    } while (curr->next != &io_write_handlers);

    if (!io_write_handler) {
        io_write_handler = new struct io_handler_struct;
        io_write_handler->funct = (void*)f;
        io_write_handler->this_ptr = this_ptr;
        io_write_handler->handler_name = new char[strlen(name) + 1];
        strcpy(io_write_handler->handler_name, name);
        io_write_handler->mask = mask;
        io_write_handler->usage_count = 0;
        // add the handler to the double linked list of handlers
        io_write_handlers.prev->next = io_write_handler;
        io_write_handler->next = &io_write_handlers;
        io_write_handler->prev = io_write_handlers.prev;
        io_write_handlers.prev = io_write_handler;
    }

    io_write_handler->usage_count += end_addr - begin_addr + 1;
    for (addr = begin_addr; addr <= end_addr; addr++)
        write_port_to_handler[addr] = io_write_handler;
    return true; // address mapped successfully
}

bool bx_devices_c::register_default_io_read_handler(void* this_ptr, bx_read_handler_t f,
    const char* name, Bit8u mask) 
{  //905
    io_read_handlers.funct = (void*)f;
    io_read_handlers.this_ptr = this_ptr;
    if (io_read_handlers.handler_name) {
        delete[] io_read_handlers.handler_name;
    }
    io_read_handlers.handler_name = new char[strlen(name) + 1];
    strcpy(io_read_handlers.handler_name, name);
    io_read_handlers.mask = mask;

    return true;
}

bool bx_devices_c::register_default_io_write_handler(void* this_ptr, bx_write_handler_t f,
    const char* name, Bit8u mask)
{
    //920
    io_write_handlers.funct = (void*)f;
    io_write_handlers.this_ptr = this_ptr;
    if (io_write_handlers.handler_name) {
        delete[] io_write_handlers.handler_name;
    }
    io_write_handlers.handler_name = new char[strlen(name) + 1];
    strcpy(io_write_handlers.handler_name, name);
    io_write_handlers.mask = mask;

    return true;
}

bool bx_devices_c::unregister_io_read_handler(void* this_ptr, bx_read_handler_t f, Bit32u addr, Bit8u mask)
{
    addr &= 0xffff;

    struct io_handler_struct* io_read_handler = read_port_to_handler[addr];

    //BX_INFO(("Unregistering I/O read handler at %#x", addr));

    if (!io_read_handler) {
        //BX_ERROR((">>> NO IO_READ_HANDLER <<<"));
        return false;
    }

    if (io_read_handler == &io_read_handlers) {
        //BX_ERROR((">>> CANNOT UNREGISTER THE DEFAULT IO_READ_HANDLER <<<"));
        return false; // cannot unregister the default handler
    }

    if (io_read_handler->funct != f) {
        //BX_ERROR((">>> NOT THE SAME IO_READ_HANDLER FUNC <<<"));
        return false;
    }

    if (io_read_handler->this_ptr != this_ptr) {
        //BX_ERROR((">>> NOT THE SAME IO_READ_HANDLER THIS_PTR <<<"));
        return false;
    }

    if (io_read_handler->mask != mask) {
        //BX_ERROR((">>> NOT THE SAME IO_READ_HANDLER MASK <<<"));
        return false;
    }

    read_port_to_handler[addr] = &io_read_handlers; // reset to default
    io_read_handler->usage_count--;

    if (!io_read_handler->usage_count) { // kill this handler entry
        io_read_handler->prev->next = io_read_handler->next;
        io_read_handler->next->prev = io_read_handler->prev;
        delete[] io_read_handler->handler_name;
        delete io_read_handler;
    }
    return true;
}

bool bx_devices_c::unregister_io_write_handler(void* this_ptr, bx_write_handler_t f,
    Bit32u addr, Bit8u mask)
{
    addr &= 0xffff;

    struct io_handler_struct* io_write_handler = write_port_to_handler[addr];

    if (!io_write_handler)
        return false;

    if (io_write_handler == &io_write_handlers)
        return false; // cannot unregister the default handler

    if (io_write_handler->funct != f)
        return false;

    if (io_write_handler->this_ptr != this_ptr)
        return false;

    if (io_write_handler->mask != mask)
        return false;

    write_port_to_handler[addr] = &io_write_handlers; // reset to default
    io_write_handler->usage_count--;

    if (!io_write_handler->usage_count) { // kill this handler entry
        io_write_handler->prev->next = io_write_handler->next;
        io_write_handler->next->prev = io_write_handler->prev;
        delete[] io_write_handler->handler_name;
        delete io_write_handler;
    }
    return true;
}

bool bx_devices_c::unregister_io_read_handler_range(void* this_ptr, bx_read_handler_t f,
    Bit32u begin, Bit32u end, Bit8u mask)
{
    begin &= 0xffff;
    end &= 0xffff;
    bool ret = true;

    /*
     * the easy way this time
     */
    for (Bit32u addr = begin; addr <= end; addr++)
        if (!unregister_io_read_handler(this_ptr, f, addr, mask))
            ret = false;

    return ret;
}

bool bx_devices_c::unregister_io_write_handler_range(void* this_ptr, bx_write_handler_t f,
    Bit32u begin, Bit32u end, Bit8u mask)
{
    begin &= 0xffff;
    end &= 0xffff;
    bool ret = true;

    /*
     * the easy way this time
     */
    for (Bit32u addr = begin; addr <= end; addr++)
        if (!unregister_io_write_handler(this_ptr, f, addr, mask))
            ret = false;

    return ret;
}


Bit32u BX_CPP_AttrRegparmN(2)
bx_devices_c::inp(Bit16u addr, unsigned io_len)
{ //1054
    struct io_handler_struct* io_read_handler;
    Bit32u ret;
    BX_INSTR_INP(addr, io_len);
    io_read_handler = read_port_to_handler[addr];
    if (io_read_handler->mask & io_len) {
        ret = ((bx_read_handler_t)io_read_handler->funct)(io_read_handler->this_ptr, (Bit32u)addr, io_len);
    }
    else {
        switch (io_len) {
        case 1: ret = 0xff; break;
        case 2: ret = 0xffff; break;
        default: ret = 0xffffffff; break;
        }
        if (addr != 0x0cf8) { // don't flood the logfile when probing PCI
            //BX_ERROR(("read from port 0x%04x with len %d returns 0x%x", addr, io_len, ret));
        }
    }
    BX_INSTR_INP2(addr, io_len, ret);
    BX_DBG_IO_REPORT(addr, io_len, BX_READ, ret);

    return ret;
}

void BX_CPP_AttrRegparmN(3)   //1086
bx_devices_c::outp(Bit16u addr, Bit32u value, unsigned io_len)
{
    struct io_handler_struct* io_write_handler;

    BX_INSTR_OUTP(addr, io_len, value);
    BX_DBG_IO_REPORT(addr, io_len, BX_WRITE, value);

    io_write_handler = write_port_to_handler[addr];
    if (io_write_handler->mask & io_len) {
        ((bx_write_handler_t)io_write_handler->funct)(io_write_handler->this_ptr, (Bit32u)addr, value, io_len);
    }
    else if (addr != 0x0cf8) { // don't flood the logfile when probing PCI
        //BX_ERROR(("write to port 0x%04x with len %d ignored", addr, io_len));
    }
}

bool bx_devices_c::is_agp_present(void)
{
#if BX_SUPPORT_PCI
    return (pci.enabled && ((pci.advopts & BX_PCI_ADVOPT_NOAGP) == 0));
#else
    return false;
#endif
}

void bx_devices_c::add_sound_device(void)
{
    sound_device_count++;
}

void bx_devices_c::remove_sound_device(void)
{
    sound_device_count--;
}

void bx_devices_c::register_default_keyboard(void* dev, bx_kbd_gen_scancode_t kbd_gen_scancode,
    bx_kbd_get_elements_t kbd_get_elements)
{ //1134
    if (bx_keyboard[0].dev == NULL) {
        bx_keyboard[0].dev = dev;
        bx_keyboard[0].gen_scancode = kbd_gen_scancode;
        bx_keyboard[0].get_elements = kbd_get_elements;
        bx_keyboard[0].led_mask = BX_KBD_LED_MASK_ALL;
        // add keyboard LEDs to the statusbar
        // 补2213条先暂时注释底下三行代码
        //statusbar_id[BX_KBD_LED_NUM] = bx_gui->register_statusitem("NUM");
        //statusbar_id[BX_KBD_LED_CAPS] = bx_gui->register_statusitem("CAPS");
        //statusbar_id[BX_KBD_LED_SCRL] = bx_gui->register_statusitem("SCRL");
    }
}

void bx_devices_c::register_removable_keyboard(void* dev, bx_kbd_gen_scancode_t kbd_gen_scancode,
    bx_kbd_get_elements_t kbd_get_elements,
    Bit8u led_mask)
{
    if (bx_keyboard[1].dev == NULL) {
        bx_keyboard[1].dev = dev;
        bx_keyboard[1].gen_scancode = kbd_gen_scancode;
        bx_keyboard[1].get_elements = kbd_get_elements;
        bx_keyboard[0].led_mask &= ~led_mask;
        bx_keyboard[1].led_mask = led_mask;
    }
}

void bx_devices_c::unregister_removable_keyboard(void* dev)
{
    if (dev == bx_keyboard[1].dev) {
        bx_keyboard[1].dev = NULL;
        bx_keyboard[1].gen_scancode = NULL;
        bx_keyboard[0].led_mask |= bx_keyboard[1].led_mask;
        bx_keyboard[1].led_mask = 0;
    }
}

void bx_devices_c::register_default_mouse(void* dev, bx_mouse_enq_t mouse_enq,
    bx_mouse_enabled_changed_t mouse_enabled_changed)
{ //1172
    if (bx_mouse[0].dev == NULL) {
        bx_mouse[0].dev = dev;
        bx_mouse[0].enq_event = mouse_enq;
        bx_mouse[0].enabled_changed = mouse_enabled_changed;
    }
}

void bx_devices_c::register_removable_mouse(void* dev, bx_mouse_enq_t mouse_enq,
    bx_mouse_enabled_changed_t mouse_enabled_changed)
{
    if (bx_mouse[1].dev == NULL) {
        bx_mouse[1].dev = dev;
        bx_mouse[1].enq_event = mouse_enq;
        bx_mouse[1].enabled_changed = mouse_enabled_changed;
    }
}

void bx_devices_c::unregister_removable_mouse(void* dev)
{
    if (dev == bx_mouse[1].dev) {
        bx_mouse[1].dev = NULL;
        bx_mouse[1].enq_event = NULL;
        bx_mouse[1].enabled_changed = NULL;
    }
}

void bx_devices_c::gen_scancode(Bit32u key)
{//1202
    bool ret = 0;

    bx_keyboard[0].bxkey_state[key & 0xff] = ((key & BX_KEY_RELEASED) == 0);
    if ((paste.buf != NULL) && (!paste.service)) {
        paste.stop = 1;
        return;
    }
    if (bx_keyboard[1].dev != NULL) {
        ret = bx_keyboard[1].gen_scancode(bx_keyboard[1].dev, key);
    }
    if ((ret == 0) && (bx_keyboard[0].dev != NULL)) {
        bx_keyboard[0].gen_scancode(bx_keyboard[0].dev, key);
    }
}

Bit8u bx_devices_c::kbd_get_elements(void)
{
    if ((bx_keyboard[1].dev != NULL) && (bx_keyboard[1].get_elements != NULL)) {
        return bx_keyboard[1].get_elements(bx_keyboard[1].dev);
    }
    if ((bx_keyboard[0].dev != NULL) && (bx_keyboard[0].get_elements != NULL)) {
        return bx_keyboard[0].get_elements(bx_keyboard[0].dev);
    }
    return BX_KBD_ELEMENTS;
}

void bx_devices_c::release_keys()
{//1230
    for (int i = 0; i < BX_KEY_NBKEYS; i++) {
        if (bx_keyboard[0].bxkey_state[i]) {
            gen_scancode(i | BX_KEY_RELEASED);
            bx_keyboard[0].bxkey_state[i] = 0;
        }
    }
}

void bx_devices_c::kbd_set_indicator(Bit8u devid, Bit8u ledid, bool state)
{ //1326
    if (bx_keyboard[devid].led_mask & (1 << ledid)) {
        //bx_gui->statusbar_setitem(statusbar_id[ledid], state, devid);
    }
}

void bx_devices_c::service_paste_buf(void)
{
    if (paste.buf == NULL) {
        return;
    }
    delete[] paste.buf;
    paste.buf = NULL;
    paste.buf_len = 0;
    paste.buf_ptr = 0;
    paste.stop = 0;
    paste.service = 0;
}

void bx_devices_c::paste_delay_changed(Bit32u value)
{
    paste.delay = value / BX_IODEV_HANDLER_PERIOD;
    if (paste.delay == 0) {
        paste.delay = 1;
    }
    paste.counter = 0;
}

void bx_devices_c::paste_bytes(Bit8u* data, Bit32s length)
{
    if (paste.buf != NULL) {
        delete[] paste.buf;
    }
    paste.buf = data;
    paste.buf_ptr = 0;
    paste.buf_len = (length > 0) ? (Bit32u)length : 0;
    paste.stop = 0;
    paste.service = 0;
    service_paste_buf();
}

void bx_devices_c::mouse_enabled_changed(bool enabled)
{
    mouse_captured = enabled;

    if ((bx_mouse[1].dev != NULL) && (bx_mouse[1].enabled_changed != NULL)) {
        bx_mouse[1].enabled_changed(bx_mouse[1].dev, enabled);
        return;
    }
    if ((bx_mouse[0].dev != NULL) && (bx_mouse[0].enabled_changed != NULL)) {
        bx_mouse[0].enabled_changed(bx_mouse[0].dev, enabled);
    }
}

void bx_devices_c::mouse_motion(int delta_x, int delta_y, int delta_z, unsigned button_state, bool absxy)
{
    if (!mouse_captured) {
        return;
    }
    if ((bx_mouse[1].dev != NULL) && (bx_mouse[1].enq_event != NULL)) {
        bx_mouse[1].enq_event(bx_mouse[1].dev, delta_x, delta_y, delta_z, button_state, absxy);
        return;
    }
    if ((bx_mouse[0].dev != NULL) && (bx_mouse[0].enq_event != NULL)) {
        bx_mouse[0].enq_event(bx_mouse[0].dev, delta_x, delta_y, delta_z, button_state, absxy);
    }
}

#if BX_SUPPORT_PCI//1367

bool bx_devices_c::register_pci_handlers(bx_pci_device_c* dev,
    Bit8u* devfunc, const char* name,
    const char* descr, Bit8u bus)
{
    unsigned i, handle, max_pci_slots = BX_N_PCI_SLOTS;
    int first_free_slot = -1;
    Bit16u bus_devfunc = *devfunc;
    char devname[80];
    const char* device;

    const unsigned chipset = BX_PCI_CHIPSET_I440FX; //自己加的


    if (strcmp(name, "pci") && strcmp(name, "pci2isa") && strcmp(name, "pci_ide")
        && ((*devfunc & 0xf8) == 0x00)) {
        if ((chipset == BX_PCI_CHIPSET_I440BX) &&
            (is_agp_present())) {
            max_pci_slots = 4;
        }
        if (bus == 0) {
            for (i = 0; i < max_pci_slots; i++) {
                sprintf(devname, "pci.slot.%d", i + 1);

                // Source: device = SIM->get_param_enum(devname)->get_selected();
                // No SIM: treat every unconfigured slot as "none".
                device = "none";

                if (strcmp(device, "none")) {
                    if (!strcmp(name, device) && !pci.slot_used[i]) {
                        *devfunc = ((i + pci.map_slot_to_dev) << 3) | (*devfunc & 0x07);
                        pci.slot_used[i] = true;
                        //BX_INFO(("PCI slot #%d used by plugin '%s'", i + 1, name));
                        break;
                    }
                }
                else if (first_free_slot == -1) {
                    first_free_slot = i;
                }
            }
            if ((*devfunc & 0xf8) == 0x00) {
                // auto-assign device to PCI slot if possible
                if (first_free_slot != -1) {
                    i = (unsigned)first_free_slot;
                    sprintf(devname, "pci.slot.%d", i + 1);

                    //SIM->get_param_enum(devname)->set_by_name(name);

                    *devfunc = ((i + pci.map_slot_to_dev) << 3) | (*devfunc & 0x07);
                    pci.slot_used[i] = true;
                    //BX_INFO(("PCI slot #%d used by plugin '%s'", i + 1, name));
                }
                else {
                    //BX_ERROR(("Plugin '%s' not connected to a PCI slot", name));
                    return false;
                }
            }
            bus_devfunc = *devfunc;
        }
        else if ((bus == 1) && (max_pci_slots == 4)) {
            pci.slot_used[4] = true;
            bus_devfunc = 0x100;
        }
        else {
            //BX_PANIC(("Invalid bus number #%d", bus));
            return false;
        }
    }
    /* check if device/function is available */
    if (pci.handler_id[bus_devfunc] == BX_MAX_PCI_DEVICES) {
        if (pci.num_pci_handlers >= BX_MAX_PCI_DEVICES) {
            //BX_INFO(("too many PCI devices installed."));
            //BX_PANIC(("  try increasing BX_MAX_PCI_DEVICES"));
            return false;
        }
        handle = pci.num_pci_handlers++;
        pci.pci_handler[handle].handler = dev;
        pci.handler_id[bus_devfunc] = handle;
        if (bus_devfunc < 0x100) {
            //BX_INFO(("%s present at device %d, function %d", descr, *devfunc >> 3,
                //*devfunc & 0x07));
        }
        else {
            //BX_INFO(("%s present on AGP bus device #0", descr));
        }
        dev->set_name(descr);
        return true; // device/function mapped successfully
    }
    return false; // device/function not available, return false.
}

bool bx_devices_c::pci_set_base_mem(void* this_ptr, memory_handler_t f1, memory_handler_t f2,
    Bit32u* addr, Bit8u* pci_conf, unsigned size)
{
    Bit32u oldbase = *addr, newbase;
    Bit32u mask = ~(size - 1);
    Bit8u pci_flags = pci_conf[0x00] & 0x0f;
    if ((pci_flags & 0x06) > 0) {
        //BX_ERROR(("Ignoring PCI base memory flag 0x%02x for now", pci_flags));
    }
    pci_conf[0x00] &= (mask & 0xf0);
    pci_conf[0x01] &= (mask >> 8) & 0xff;
    pci_conf[0x02] &= (mask >> 16) & 0xff;
    pci_conf[0x03] &= (mask >> 24) & 0xff;
    newbase = ReadHostDWordFromLittleEndian((Bit32u*)pci_conf);
    pci_conf[0x00] |= pci_flags;
    if (newbase != mask && newbase != oldbase) { // skip PCI probe
        if (oldbase > 0) {
            DEV_unregister_memory_handlers(this_ptr, oldbase, oldbase + size - 1);
        }
        if (newbase > 0) {
            DEV_register_memory_handlers(this_ptr, f1, f2, newbase, newbase + size - 1);
        }
        *addr = newbase;
        return true;
    }
    return false;
}

bool bx_devices_c::pci_set_base_io(void* this_ptr, bx_read_handler_t f1, bx_write_handler_t f2,
    Bit32u* addr, Bit8u* pci_conf, unsigned size,
    const Bit8u* iomask, const char* name)
{
    unsigned i;
    Bit32u oldbase = *addr, newbase;
    Bit16u mask = ~(size - 1);
    Bit8u pci_flags = pci_conf[0x00] & 0x03;
    pci_conf[0x00] &= (mask & 0xfc);
    pci_conf[0x01] &= (mask >> 8);
    newbase = ReadHostDWordFromLittleEndian((Bit32u*)pci_conf);
    pci_conf[0x00] |= pci_flags;
    if (((newbase & 0xfffc) != mask) && (newbase != oldbase)) { // skip PCI probe
        if (oldbase > 0) {
            for (i = 0; i < size; i++) {
                if (iomask[i] > 0) {
                    DEV_unregister_ioread_handler(this_ptr, f1, oldbase + i, iomask[i]);
                    DEV_unregister_iowrite_handler(this_ptr, f2, oldbase + i, iomask[i]);
                }
            }
        }
        if (newbase > 0) {
            for (i = 0; i < size; i++) {
                if (iomask[i] > 0) {
                    DEV_register_ioread_handler(this_ptr, f1, newbase + i, name, iomask[i]);
                    DEV_register_iowrite_handler(this_ptr, f2, newbase + i, name, iomask[i]);
                }
            }
        }
        *addr = newbase;
        return true;
    }
    return false;
}

#undef LOG_THIS
#define LOG_THIS

void bx_pci_device_c::init_pci_conf(Bit16u vid, Bit16u did, Bit8u rev,
    Bit32u classc, Bit8u headt, Bit8u intpin)
{
    memset(pci_conf, 0, 256);
    pci_conf[0x00] = (Bit8u)(vid & 0xff);
    pci_conf[0x01] = (Bit8u)(vid >> 8);
    pci_conf[0x02] = (Bit8u)(did & 0xff);
    pci_conf[0x03] = (Bit8u)(did >> 8);
    pci_conf[0x08] = rev;
    pci_conf[0x09] = (Bit8u)(classc & 0xff);
    pci_conf[0x0a] = (Bit8u)((classc >> 8) & 0xff);
    pci_conf[0x0b] = (Bit8u)((classc >> 16) & 0xff);
    pci_conf[0x0e] = headt;
    pci_conf[0x3d] = intpin;
}

void bx_pci_device_c::init_bar_io(Bit8u num, Bit16u size, bx_read_handler_t rh,
    bx_write_handler_t wh, const Bit8u* mask)
{
    if (num < 6) {
        pci_bar[num].type = BX_PCI_BAR_TYPE_IO;
        pci_bar[num].size = size;
        pci_bar[num].io.rh = rh;
        pci_bar[num].io.wh = wh;
        pci_bar[num].io.mask = mask;
        pci_conf[0x10 + num * 4] = 0x01;
    }
}

void bx_pci_device_c::init_bar_mem(Bit8u num, Bit32u size, memory_handler_t rh,
    memory_handler_t wh)
{
    if (num < 6) {
        pci_bar[num].type = BX_PCI_BAR_TYPE_MEM;
        pci_bar[num].size = size;
        pci_bar[num].mem.rh = rh;
        pci_bar[num].mem.wh = wh;
    }
}

void bx_pci_device_c::register_pci_state(bx_list_c* list)
{
    new bx_shadow_data_c(list, "pci_conf", pci_conf, 256, 1);
}

void bx_pci_device_c::after_restore_pci_state(memory_handler_t mem_read_handler)
{
    for (int i = 0; i < 6; i++) {
        if (pci_bar[i].type == BX_PCI_BAR_TYPE_MEM) {
            if (DEV_pci_set_base_mem(this, pci_bar[i].mem.rh, pci_bar[i].mem.wh,
                &pci_bar[i].addr, &pci_conf[0x10 + i * 4],
                pci_bar[i].size)) {
                //BX_INFO(("BAR #%d: mem base address = 0x%08x", i, pci_bar[i].addr));
                pci_bar_change_notify();
            }
        }
        else if (pci_bar[i].type == BX_PCI_BAR_TYPE_IO) {
            if (DEV_pci_set_base_io(this, pci_bar[i].io.rh, pci_bar[i].io.wh,
                &pci_bar[i].addr, &pci_conf[0x10 + i * 4],
                pci_bar[i].size, pci_bar[i].io.mask, pci_name)) {
                //BX_INFO(("BAR #%d: i/o base address = 0x%04x", i, pci_bar[i].addr));
                pci_bar_change_notify();
            }
        }
    }
    if (pci_rom_size > 0) {
        if (DEV_pci_set_base_mem(this, mem_read_handler, NULL, &pci_rom_address,
            &pci_conf[0x30], pci_rom_size)) {
            //BX_INFO(("new ROM address: 0x%08x", pci_rom_address));
        }
    }
}

void bx_pci_device_c::load_pci_rom(const char* path)
{
    struct stat stat_buf;
    int fd, ret;
    unsigned long size, max_size, offset;

    if ((path == NULL) || (*path == '\0')) {
        pci_rom_size = 0;
        return;
    }

    fd = open(path, O_RDONLY
#ifdef O_BINARY
        | O_BINARY
#endif
    );
    if (fd < 0) {
        pci_rom_size = 0;
        return;
    }

    ret = fstat(fd, &stat_buf);
    if (ret != 0) {
        close(fd);
        pci_rom_size = 0;
        return;
    }

    max_size = 0x20000;
    size = (unsigned long)stat_buf.st_size;
    if ((size == 0) || (size > max_size) || ((size % 512) != 0)) {
        close(fd);
        pci_rom_size = 0;
        return;
    }

    while ((size - 1) < max_size) {
        max_size >>= 1;
    }

    if (pci_rom != NULL) {
        delete[] pci_rom;
        pci_rom = NULL;
    }

    pci_rom_size = (max_size << 1);
    pci_rom = new Bit8u[pci_rom_size];
    memset(pci_rom, 0xff, pci_rom_size);

    offset = 0;
    size = (unsigned long)stat_buf.st_size;
    while (offset < size) {
        ret = read(fd, (void*)(pci_rom + offset), size - offset);
        if (ret <= 0) {
            delete[] pci_rom;
            pci_rom = NULL;
            pci_rom_size = 0;
            close(fd);
            return;
        }
        offset += ret;
    }
    close(fd);
}

void bx_pci_device_c::pci_write_handler_common(Bit8u address, Bit32u value, unsigned io_len)
{
    Bit8u bnum, value8, oldval;
    bool bar_change = 0, rom_change = 0;

    // ignore readonly registers
    if ((address < 4) || ((address > 7) && (address < 12)) || (address == 14) ||
        (address == 0x3d)) {
        //BX_DEBUG(("write to r/o PCI register 0x%02x ignored", address));
        return;
    }

    // handle base address registers if header type bit #0 and #1 are clear
    if (((pci_conf[0x0e] & 0x03) == 0) && (address >= 0x10) && (address < 0x28)) {
        bnum = ((address - 0x10) >> 2);
        if (pci_bar[bnum].type != BX_PCI_BAR_TYPE_NONE) {
            //BX_DEBUG_PCI_WRITE(address, value, io_len);
            for (unsigned i = 0; i < io_len; i++) {
                value8 = (value >> (i * 8)) & 0xff;
                oldval = pci_conf[address + i];
                if (((address + i) & 0x03) == 0) {
                    if (pci_bar[bnum].type == BX_PCI_BAR_TYPE_IO) {
                        value8 = (value8 & 0xfc) | 0x01;
                    }
                    else {
                        value8 = (value8 & 0xf0) | (oldval & 0x0f);
                    }
                }
                bar_change |= (value8 != oldval);
                pci_conf[address + i] = value8;
            }
            if (bar_change) {
                if (pci_bar[bnum].type == BX_PCI_BAR_TYPE_IO) {
                    if (DEV_pci_set_base_io(this, pci_bar[bnum].io.rh, pci_bar[bnum].io.wh,
                        &pci_bar[bnum].addr, &pci_conf[0x10 + bnum * 4],
                        pci_bar[bnum].size, pci_bar[bnum].io.mask, pci_name)) {
                        //BX_INFO(("BAR #%d: i/o base address = 0x%04x", bnum, pci_bar[bnum].addr));
                        pci_bar_change_notify();
                    }
                }
                else {
                    if (DEV_pci_set_base_mem(this, pci_bar[bnum].mem.rh, pci_bar[bnum].mem.wh,
                        &pci_bar[bnum].addr, &pci_conf[0x10 + bnum * 4],
                        pci_bar[bnum].size)) {
                        //BX_INFO(("BAR #%d: mem base address = 0x%08x", bnum, pci_bar[bnum].addr));
                        pci_bar_change_notify();
                    }
                }
            }
        }
    }
    else if ((address & 0xfc) == 0x30) {
        //BX_DEBUG_PCI_WRITE(address, value, io_len);
        value &= (0xfffffc01 >> ((address & 0x03) * 8));
        for (unsigned i = 0; i < io_len; i++) {
            value8 = (value >> (i * 8)) & 0xff;
            oldval = pci_conf[address + i];
            rom_change |= (value8 != oldval);
            pci_conf[address + i] = value8;
        }
        if (rom_change) {
            if (DEV_pci_set_base_mem(this, pci_rom_read_handler, NULL,
                &pci_rom_address, &pci_conf[0x30],
                pci_rom_size)) {
                //BX_INFO(("new ROM address = 0x%08x", pci_rom_address));
            }
        }
    }
    else if (address == 0x3c) {
        value8 = (Bit8u)value;
        if (value8 != pci_conf[0x3c]) {
            if (pci_conf[0x3d] != 0) {
                //BX_INFO(("new IRQ line = %d", value8));
            }
            pci_conf[0x3c] = value8;
        }
    }
    else {
        pci_write_handler(address, value, io_len);
    }
}

Bit32u bx_pci_device_c::pci_read_handler(Bit8u address, unsigned io_len)
{
    Bit32u value = 0;

    for (unsigned i = 0; i < io_len; i++) {
        value |= (pci_conf[address + i] << (i * 8));
    }

    //BX_DEBUG_PCI_READ(address, value, io_len);

    return value;
}

#endif

