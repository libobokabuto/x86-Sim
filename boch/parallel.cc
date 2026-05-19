#define  _CRT_SECURE_NO_WARNINGS
#include "iodev.h"
#include "parallel.h"

#define LOG_THIS theParallelDevice->

bx_parallel_c* theParallelDevice = NULL;

void parport_init_options(void)
{
    /*
    char name[4], label[80], descr[80];

    bx_list_c* ports = (bx_list_c*)SIM->get_param("ports");
    bx_list_c* parallel = new bx_list_c(ports, "parallel", "Parallel Port Options");
    parallel->set_options(parallel->SHOW_PARENT);
    for (int i = 0; i < BX_N_PARALLEL_PORTS; i++) {
        sprintf(name, "%d", i + 1);
        sprintf(label, "Parallel Port %d", i + 1);
        bx_list_c* menu = new bx_list_c(parallel, name, label);
        menu->set_options(menu->SERIES_ASK);
        sprintf(label, "Enable parallel port #%d", i + 1);
        sprintf(descr, "Controls whether parallel port #%d is installed or not", i + 1);
        bx_param_bool_c* enabled = new bx_param_bool_c(menu, "enabled", label, descr,
            (i == 0) ? 1 : 0);  // only enable #1 by default
        sprintf(label, "Parallel port #%d output file", i + 1);
        sprintf(descr, "Data written to parport#%d by the guest OS is written to this file", i + 1);
        bx_param_filename_c* path = new bx_param_filename_c(menu, "file", label, descr,
            "", BX_PATHNAME_LEN);
        path->set_extension("out");
        bx_list_c* deplist = new bx_list_c(NULL);
        deplist->add(path);
        enabled->set_dependent_list(deplist);
    }
    */
}

PLUGIN_ENTRY_FOR_MODULE(parallel)
{
    if (mode == PLUGIN_INIT) {
        theParallelDevice = new bx_parallel_c();
        BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theParallelDevice, BX_PLUGIN_PARALLEL);
        // add new configuration parameters for the config interface
        parport_init_options();
        // register add-on options for bochsrc and command line
        //SIM->register_addon_option("parport1", parport_options_parser, parport_options_save);
        //SIM->register_addon_option("parport2", parport_options_parser, NULL);
    }
    else if (mode == PLUGIN_FINI) {
        delete theParallelDevice;
        //SIM->unregister_addon_option("parport1");
        //SIM->unregister_addon_option("parport2");
        //bx_list_c* ports = (bx_list_c*)SIM->get_param("ports");
        //ports->remove("parallel");
    }
    else if (mode == PLUGIN_PROBE) {
        return (int)PLUGTYPE_OPTIONAL;
    }
    return 0; // Success
}

bx_parallel_c::bx_parallel_c()
{
    //put("parallel", "PAR");
    for (int i = 0; i < BX_PARPORT_MAXDEV; i++) {
        memset(&s[i], 0, sizeof(bx_par_t));
    }
}

bx_parallel_c::~bx_parallel_c()
{
    for (int i = 0; i < BX_PARPORT_MAXDEV; i++) {
        if (s[i].output != NULL)
            fclose(s[i].output);
    }
    //bx_list_c* misc_rt = (bx_list_c*)SIM->get_param(BXPN_MENU_RUNTIME_MISC);
    //misc_rt->remove("parport");
    //SIM->get_bochs_root()->remove("parallel");
    //BX_DEBUG(("Exit"));
}

void bx_parallel_c::init(void)
{//自己修改
    const Bit8u port = 0;
    const Bit16u base = 0x0378;
    const Bit8u irq = 7;
    const char* name = "Parallel Port 1";

    BX_PAR_THIS s[port].IRQ = irq;

    DEV_register_ioread_handler(this, read_handler, base, name, 1);
    DEV_register_ioread_handler(this, read_handler, base + 1, name, 1);
    DEV_register_ioread_handler(this, read_handler, base + 2, name, 1);

    DEV_register_iowrite_handler(this, write_handler, base, name, 1);
    DEV_register_iowrite_handler(this, write_handler, base + 2, name, 1);

    BX_PAR_THIS s[port].data = 0;

    BX_PAR_THIS s[port].STATUS.error = 1;
    BX_PAR_THIS s[port].STATUS.slct = 1;
    BX_PAR_THIS s[port].STATUS.pe = 0;
    BX_PAR_THIS s[port].STATUS.ack = 1;
    BX_PAR_THIS s[port].STATUS.busy = 1;

    BX_PAR_THIS s[port].CONTROL.strobe = 0;
    BX_PAR_THIS s[port].CONTROL.autofeed = 0;
    BX_PAR_THIS s[port].CONTROL.init = 1;
    BX_PAR_THIS s[port].CONTROL.slct_in = 1;
    BX_PAR_THIS s[port].CONTROL.irq = 0;
    BX_PAR_THIS s[port].CONTROL.input = 0;

    BX_PAR_THIS s[port].file = NULL;
    BX_PAR_THIS s[port].output = NULL;
    BX_PAR_THIS s[port].file_changed = 0;
    BX_PAR_THIS s[port].initmode = 0;

    //BX_INFO(("parallel port 1 at 0x%04x irq %d", base, irq));
}

void bx_parallel_c::reset(unsigned type)
{
}

void bx_parallel_c::virtual_printer(Bit8u port)
{//自己修改了
    if (BX_PAR_THIS s[port].STATUS.slct) {
        if (BX_PAR_THIS s[port].file_changed) {
            if ((BX_PAR_THIS s[port].file != NULL) &&
                !BX_PAR_THIS s[port].file->isempty() &&
                (BX_PAR_THIS s[port].output == NULL)) {
                BX_PAR_THIS s[port].output = fopen(BX_PAR_THIS s[port].file->getptr(), "wb");
                if (!BX_PAR_THIS s[port].output){}
                    //BX_ERROR(("Could not open '%s' to write parport%d output",
                        //BX_PAR_THIS s[port].file->getptr(), port + 1));
            }
            BX_PAR_THIS s[port].file_changed = 0;
        }
        if (BX_PAR_THIS s[port].output != NULL) {
            fputc(BX_PAR_THIS s[port].data, BX_PAR_THIS s[port].output);
            fflush(BX_PAR_THIS s[port].output);
        }
        if (BX_PAR_THIS s[port].CONTROL.irq == 1) {
            DEV_pic_raise_irq(BX_PAR_THIS s[port].IRQ);
        }
        BX_PAR_THIS s[port].STATUS.ack = 0;
        BX_PAR_THIS s[port].STATUS.busy = 1;
    }
    else {
        //BX_ERROR(("data is valid, but printer is offline"));
    }
}

Bit32u bx_parallel_c::read_handler(void* this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_PAR_SMF
    bx_parallel_c* class_ptr = (bx_parallel_c*)this_ptr;
    return class_ptr->read(address, io_len);
}

Bit32u bx_parallel_c::read(Bit32u address, unsigned io_len)
{
#else
    UNUSED(this_ptr);
#endif  // !BX_USE_PAR_SMF
    Bit8u offset;
    Bit8u port = 0;
    Bit32u retval;

    offset = address & 0x07;
    switch (address & 0x03f8) {
    case 0x0378: port = 0; break;
    case 0x0278: port = 1; break;
    }

    switch (offset) {
    case BX_PAR_DATA:
        if (!BX_PAR_THIS s[port].CONTROL.input) {
            return (Bit32u)BX_PAR_THIS s[port].data;
        }
        else {
            //BX_ERROR(("read: input mode not supported"));
            return (0xFF);
        }
        break;
    case BX_PAR_STAT:
    {
        retval = ((BX_PAR_THIS s[port].STATUS.busy << 7) |
            (BX_PAR_THIS s[port].STATUS.ack << 6) |
            (BX_PAR_THIS s[port].STATUS.pe << 5) |
            (BX_PAR_THIS s[port].STATUS.slct << 4) |
            (BX_PAR_THIS s[port].STATUS.error << 3));
        if (BX_PAR_THIS s[port].STATUS.ack == 0) {
            BX_PAR_THIS s[port].STATUS.ack = 1;
            if (BX_PAR_THIS s[port].CONTROL.irq == 1) {
                DEV_pic_lower_irq(BX_PAR_THIS s[port].IRQ);
            }
        }
        if (BX_PAR_THIS s[port].initmode == 1) {
            BX_PAR_THIS s[port].STATUS.busy = 1;
            BX_PAR_THIS s[port].STATUS.slct = 1;
            BX_PAR_THIS s[port].STATUS.ack = 0;
            if (BX_PAR_THIS s[port].CONTROL.irq == 1) {
                DEV_pic_raise_irq(BX_PAR_THIS s[port].IRQ);
            }
            BX_PAR_THIS s[port].initmode = 0;
        }
        //BX_DEBUG(("read: parport%d status register returns 0x%02x", port + 1, retval));
        return retval;
    }
    break;
    case BX_PAR_CTRL:
    {
        retval = ((BX_PAR_THIS s[port].CONTROL.input << 5) |
            (BX_PAR_THIS s[port].CONTROL.irq << 4) |
            (BX_PAR_THIS s[port].CONTROL.slct_in << 3) |
            (BX_PAR_THIS s[port].CONTROL.init << 2) |
            (BX_PAR_THIS s[port].CONTROL.autofeed << 1) |
            (Bit8u)BX_PAR_THIS s[port].CONTROL.strobe);
        //BX_DEBUG(("read: parport%d control register returns 0x%02x", port + 1, retval));
        return retval;
    }
    break;
    }
    return(0);
}


void bx_parallel_c::write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_PAR_SMF
    bx_parallel_c* class_ptr = (bx_parallel_c*)this_ptr;

    class_ptr->write(address, value, io_len);
}

void bx_parallel_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
    UNUSED(this_ptr);
#endif  // !BX_USE_PAR_SMF
    Bit8u offset;
    Bit8u port = 0;
    char name[16];

    offset = address & 0x07;
    switch (address & 0x03f8) {
    case 0x0378: port = 0; break;
    case 0x0278: port = 1; break;
    }

    switch (offset) {
    case BX_PAR_DATA:
        BX_PAR_THIS s[port].data = (Bit8u)value;
        //BX_DEBUG(("write: parport%d data output register = 0x%02x", port + 1, (Bit8u)value));
        break;
    case BX_PAR_CTRL:
    {
        if ((value & 0x01) == 0x01) {
            if (BX_PAR_THIS s[port].CONTROL.strobe == 0) {
                BX_PAR_THIS s[port].CONTROL.strobe = 1;
                virtual_printer(port); // data is valid now
            }
        }
        else {
            if (BX_PAR_THIS s[port].CONTROL.strobe == 1) {
                BX_PAR_THIS s[port].CONTROL.strobe = 0;
            }
        }
        BX_PAR_THIS s[port].CONTROL.autofeed = ((value & 0x02) == 0x02);
        if ((value & 0x04) == 0x04) {
            if (BX_PAR_THIS s[port].CONTROL.init == 0) {
                BX_PAR_THIS s[port].CONTROL.init = 1;
                BX_PAR_THIS s[port].STATUS.busy = 0;
                BX_PAR_THIS s[port].STATUS.slct = 0;
                BX_PAR_THIS s[port].initmode = 1;
                //BX_DEBUG(("parport%d: printer init requested", port + 1));
            }
        }
        else {
            if (BX_PAR_THIS s[port].CONTROL.init == 1) {
                BX_PAR_THIS s[port].CONTROL.init = 0;
            }
        }
        if ((value & 0x08) == 0x08) {
            if (BX_PAR_THIS s[port].CONTROL.slct_in == 0) {
                BX_PAR_THIS s[port].CONTROL.slct_in = 1;
                //BX_DEBUG(("parport%d: printer now online", port + 1));
            }
        }
        else {
            if (BX_PAR_THIS s[port].CONTROL.slct_in == 1) {
                BX_PAR_THIS s[port].CONTROL.slct_in = 0;
                //BX_DEBUG(("parport%d: printer now offline", port + 1));
            }
        }
        BX_PAR_THIS s[port].STATUS.slct = BX_PAR_THIS s[port].CONTROL.slct_in;
        if ((value & 0x10) == 0x10) {
            if (BX_PAR_THIS s[port].CONTROL.irq == 0) {
                BX_PAR_THIS s[port].CONTROL.irq = 1;
                sprintf(name, "Parallel Port %d", port + 1);
                DEV_register_irq(BX_PAR_THIS s[port].IRQ, name);
                //BX_DEBUG(("parport%d: irq mode selected", port + 1));
            }
        }
        else {
            if (BX_PAR_THIS s[port].CONTROL.irq == 1) {
                BX_PAR_THIS s[port].CONTROL.irq = 0;
                sprintf(name, "Parallel Port %d", port + 1);
                DEV_unregister_irq(BX_PAR_THIS s[port].IRQ, name);
                //BX_DEBUG(("parport%d: polling mode selected", port + 1));
            }
        }
        if ((value & 0x20) == 0x20) {
            if (BX_PAR_THIS s[port].CONTROL.input == 0) {
                BX_PAR_THIS s[port].CONTROL.input = 1;
                //BX_DEBUG(("parport%d: data input mode selected", port + 1));
            }
        }
        else {
            if (BX_PAR_THIS s[port].CONTROL.input == 1) {
                BX_PAR_THIS s[port].CONTROL.input = 0;
                //BX_DEBUG(("parport%d: data output mode selected", port + 1));
            }
        }
        if ((value & 0xC0) > 0) {
            //BX_ERROR(("write: parport%d: unsupported control bit ignored", port + 1));
        }
    }
    break;
    }
}
