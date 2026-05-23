#define _CRT_SECURE_NO_WARNINGS
#include "iodev.h"

#include "instrument.h"

#include "debug.h"
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
    sound_device_count = 0;
}

bx_devices_c::~bx_devices_c()
{
#if 0
    timer_handle = BX_NULL_TIMER_HANDLE;
    bx_hdimage_ctl.exit();
#if BX_NETWORKING
    bx_netmod_ctl.exit();
#endif
#if BX_SUPPORT_SOUNDLOW
    bx_soundmod_ctl.exit();
#endif
#if BX_SUPPORT_PCIUSB
    bx_usbdev_ctl.exit();
#endif
#endif

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
    unsigned chipset = 1;
    unsigned max_pci_slots = BX_N_PCI_SLOTS;
#endif
    unsigned i, argc;
    const char def_name[] = "Default";
    const char* options;
    char* argv[16];

    mem = newmem;

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

    PLUG_load_plugin(pci, PLUGTYPE_CORE); //216

    PLUG_load_plugin(cmos, PLUGTYPE_CORE); //238

    PLUG_load_plugin(dma, PLUGTYPE_CORE); //239
    PLUG_load_plugin(pic, PLUGTYPE_CORE);//240

    PLUG_load_plugin(keyboard, PLUGTYPE_STANDARD); //250

    PLUG_load_plugin(parallel, PLUGTYPE_OPTIONAL); //自己加的按 PLUGTYPE_OPTIONAL 加载，不按 CORE

    PLUG_load_plugin(serial, PLUGTYPE_OPTIONAL);//同parallel一样

    PLUG_load_plugin(biosdev, PLUGTYPE_OPTIONAL);

    
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

void bx_devices_c::register_default_mouse(void* dev, bx_mouse_enq_t mouse_enq,
    bx_mouse_enabled_changed_t mouse_enabled_changed)
{ //1172
    if (bx_mouse[0].dev == NULL) {
        bx_mouse[0].dev = dev;
        bx_mouse[0].enq_event = mouse_enq;
        bx_mouse[0].enabled_changed = mouse_enabled_changed;
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
