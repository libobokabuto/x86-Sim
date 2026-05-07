#define _CRT_SECURE_NO_WARNINGS
#include "iodev.h"

#include "instrument.h"

#include "debug.h"
bx_devices_c bx_devices;

bx_devices_c::bx_devices_c()
{
    //52
}

bx_devices_c::~bx_devices_c()
{
    //70
}


void bx_devices_c::init(BX_MEM_C* newmem)
{
    //110
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

    PLUG_load_plugin(pci, PLUGTYPE_CORE);
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