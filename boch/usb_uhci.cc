#define _CRT_SECURE_NO_WARNINGS
#define BX_PLUGGABLE

#include "iodev.h"

#if BX_SUPPORT_PCI && BX_SUPPORT_USB_UHCI

#include "pci.h"
#include "usb_common.h"
#include "uhci_core.h"
#include "usb_uhci.h"

#define LOG_THIS theUSB_UHCI->

bx_usb_uhci_c* theUSB_UHCI = NULL;

#endif // BX_SUPPORT_PCI && BX_SUPPORT_USB_UHCI

Bit32s usb_uhci_options_parser(const char* context, int num_params, char* params[])
{
    UNUSED(context);
    UNUSED(num_params);
    UNUSED(params);
    return 0;
}

Bit32s usb_uhci_options_save(FILE* fp)
{
    UNUSED(fp);
    return 0;
}

PLUGIN_ENTRY_FOR_MODULE(usb_uhci)
{
    if (mode == PLUGIN_INIT) {
        theUSB_UHCI = new bx_usb_uhci_c();
        BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theUSB_UHCI, BX_PLUGIN_USB_UHCI);
        // add new configuration parameter for the config interface
        //SIM->init_usb_options("UHCI", "uhci", USB_UHCI_PORTS, 0);
        // register add-on option for bochsrc and command line
        //SIM->register_addon_option("usb_uhci", usb_uhci_options_parser, usb_uhci_options_save);
    }
    else if (mode == PLUGIN_FINI) {
        //SIM->unregister_addon_option("usb_uhci");
        //bx_list_c* menu = (bx_list_c*)SIM->get_param("ports.usb");
        delete theUSB_UHCI;
        theUSB_UHCI = NULL;
        //menu->remove("uhci");
    }
    else if (mode == PLUGIN_PROBE) {
        return (int)PLUGTYPE_OPTIONAL;
    }
    else if (mode == PLUGIN_FLAGS) {
        return PLUGFLAG_PCI;
    }
    return 0; // Success
}

bx_usb_uhci_c::bx_usb_uhci_c()
{
    //put("usb_uhci", "UHCI");
    device_change = 0;
    rt_conf_id = -1;
}

bx_usb_uhci_c::~bx_usb_uhci_c()
{
    char pname[32];

    //SIM->unregister_runtime_config_handler(rt_conf_id);

    for (int i = 0; i < USB_UHCI_PORTS; i++) {
        //sprintf(pname, "port%d.device", i + 1);
        //SIM->get_param_enum(pname, SIM->get_param(BXPN_USB_UHCI))->set_handler(NULL);
        //sprintf(pname, "port%d.options", i + 1);
        //SIM->get_param_string(pname, SIM->get_param(BXPN_USB_UHCI))->set_enable_handler(NULL);
        //sprintf(pname, "port%d.over_current", i + 1);
        //SIM->get_param_bool(pname, SIM->get_param(BXPN_USB_UHCI))->set_handler(NULL);
        remove_device(i);
    }

    //SIM->get_bochs_root()->remove("usb_uhci");
    //bx_list_c* usb_rt = (bx_list_c*)SIM->get_param(BXPN_MENU_RUNTIME_USB);
    //usb_rt->remove("uhci");

    //BX_DEBUG(("Exit"));
}

void bx_usb_uhci_c::init(void)
{
    unsigned i;
    char pname[6];
    bx_list_c* uhci, * port;
    bx_param_enum_c* device;
    bx_param_string_c* options;
    bx_param_bool_c* over_current;
    Bit8u devfunc;
    Bit16u devid;

    /*  If you wish to set DEBUG=report in the code, instead of
     *  in the configuration, simply uncomment this line.  I use
     *  it when I am working on this emulation.
     */
     //LOG_THIS setonoff(LOGLEV_DEBUG, ACT_REPORT);

     // Read in values from config interface
    uhci = NULL;
    // Check if the device is disabled or not configured
    if (0) {
        //BX_INFO(("USB UHCI disabled"));
        // mark unused plugin for removal
        //((bx_param_bool_c*)((bx_list_c*)SIM->get_param(BXPN_PLUGIN_CTRL))->get_by_name("usb_uhci"))->set(0);
        return;
    }

    if (BX_PCI_CHIPSET_I440FX == BX_PCI_CHIPSET_I440FX) {
        devfunc = BX_PCI_DEVICE(1, 2);
        devid = 0x7020;
    }
    else if (BX_PCI_CHIPSET_I440FX == BX_PCI_CHIPSET_I440BX) {
        devfunc = BX_PCI_DEVICE(7, 2);
        devid = 0x7112;
    }
    else {
        devfunc = 0x00;
        devid = 0x7020;
    }
    BX_UHCI_THIS init_uhci(devfunc, 0x8086, devid, 0x01, 0x00, BX_PCI_INTD);
#if 0
    bx_list_c* usb_rt = (bx_list_c*)SIM->get_param(BXPN_MENU_RUNTIME_USB);
    bx_list_c* uhci_rt = new bx_list_c(usb_rt, "uhci", "UHCI Runtime Options");
    uhci_rt->set_options(uhci_rt->SHOW_PARENT);
    for (i = 0; i < USB_UHCI_PORTS; i++) {
        sprintf(pname, "port%d", i + 1);
        port = (bx_list_c*)SIM->get_param(pname, uhci);
        uhci_rt->add(port);
        device = (bx_param_enum_c*)port->get_by_name("device");
        device->set_handler(usb_param_handler);
        options = (bx_param_string_c*)port->get_by_name("options");
        options->set_enable_handler(usb_param_enable_handler);
        over_current = (bx_param_bool_c*)port->get_by_name("over_current");
        over_current->set_handler(usb_param_oc_handler);
    }
#endif
    // register handler for correct device connect handling after runtime config
    BX_UHCI_THIS rt_conf_id = -1;
    BX_UHCI_THIS device_change = 0;

#if BX_USB_DEBUGGER
    //if (SIM->get_param_enum(BXPN_USB_DEBUG_TYPE)->get() == USB_DEBUG_UHCI) {
        //SIM->register_usb_debug_type(USB_DEBUG_UHCI);
    //}
#endif

    //BX_INFO(("USB UHCI initialized"));
}

void bx_usb_uhci_c::reset(unsigned type)
{
    unsigned i;
    char pname[6];

    BX_UHCI_THIS reset_uhci(type);
    for (i = 0; i < USB_UHCI_PORTS; i++) {
        if (BX_UHCI_THIS hub.usb_port[i].device == NULL) {
            sprintf(pname, "port%d", i + 1);
            //init_device(i, (bx_list_c*)SIM->get_param(pname, SIM->get_param(BXPN_USB_UHCI)));
        }
    }
}

void bx_usb_uhci_c::register_state()
{
    BX_UHCI_THIS uhci_register_state(NULL);
}

void bx_usb_uhci_c::after_restore_state()
{
    bx_uhci_core_c::after_restore_state();
}

int uhci_event_handler(int event, void* ptr, void* dev, int port);

void bx_usb_uhci_c::init_device(Bit8u port, bx_list_c* portconf)
{
    char pname[BX_PATHNAME_LEN];

    if (DEV_usb_init_device(portconf, BX_UHCI_THIS_PTR, &BX_UHCI_THIS hub.usb_port[port].device, uhci_event_handler, port)) {
        if (set_connect_status(port, 1)) {
            portconf->get_by_name("options")->set_enabled(0);
            sprintf(pname, "usb_uhci.hub.port%d.device", port + 1);
            //bx_list_c* sr_list = (bx_list_c*)SIM->get_param(pname, SIM->get_bochs_root());
            //BX_UHCI_THIS hub.usb_port[port].device->register_state(sr_list);
        }
        else {
            ((bx_param_enum_c*)portconf->get_by_name("device"))->set_by_name("none");
            ((bx_param_string_c*)portconf->get_by_name("options"))->set("none");
            ((bx_param_bool_c*)portconf->get_by_name("over_current"))->set(0);
            set_connect_status(port, 0);
        }
    }
}

void bx_usb_uhci_c::remove_device(Bit8u port)
{
    if (BX_UHCI_THIS hub.usb_port[port].device != NULL) {
        delete BX_UHCI_THIS hub.usb_port[port].device;
        BX_UHCI_THIS hub.usb_port[port].device = NULL;
    }
}

void bx_usb_uhci_c::runtime_config_handler(void* this_ptr)
{
    bx_usb_uhci_c* class_ptr = (bx_usb_uhci_c*)this_ptr;
    class_ptr->runtime_config();
}

void bx_usb_uhci_c::runtime_config(void)
{
    char pname[8];

    for (int i = 0; i < USB_UHCI_PORTS; i++) {
        // device change support
        if ((BX_UHCI_THIS device_change & (1 << i)) != 0) {
            if (!BX_UHCI_THIS hub.usb_port[i].status) {
                sprintf(pname, "port%d", i + 1);
                //dinit_device(i, (bx_list_c*)SIM->get_param(pname, SIM->get_param(BXPN_USB_UHCI)));
            }
            else {
                set_connect_status(i, 0);
                remove_device(i);
            }
            BX_UHCI_THIS device_change &= ~(1 << i);
        }
        // forward to connected device
        if (BX_UHCI_THIS hub.usb_port[i].device != NULL) {
            BX_UHCI_THIS hub.usb_port[i].device->runtime_config();
        }
    }
}

// USB runtime parameter handler
Bit64s bx_usb_uhci_c::usb_param_handler(bx_param_c* param, bool set, Bit64s val)
{
    if (set) {
        int portnum = atoi((param->get_parent())->get_name() + 4) - 1;
        bool empty = (val == 0);
        if ((portnum >= 0) && (portnum < USB_UHCI_PORTS)) {
            if (empty && BX_UHCI_THIS hub.usb_port[portnum].status) {
                BX_UHCI_THIS device_change |= (1 << portnum);
            }
            else if (!empty && !BX_UHCI_THIS hub.usb_port[portnum].status) {
                BX_UHCI_THIS device_change |= (1 << portnum);
            }
            else if (val != ((bx_param_enum_c*)param)->get()) {
                //BX_ERROR(("usb_param_handler(): port #%d already in use", portnum + 1));
                val = ((bx_param_enum_c*)param)->get();
            }
        }
        else {
            //BX_PANIC(("usb_param_handler called with unexpected parameter '%s'", param->get_name()));
        }
    }
    return val;
}

// USB runtime parameter handler: over-current
Bit64s bx_usb_uhci_c::usb_param_oc_handler(bx_param_c* param, bool set, Bit64s val)
{
    if (set && val) {
        int portnum = atoi((param->get_parent())->get_name() + 4) - 1;
        if ((portnum >= 0) && (portnum < USB_UHCI_PORTS)) {
            if (BX_UHCI_THIS hub.usb_port[portnum].status) {
                // The UHCI specification does not specify what happens when an over-current
                //  condition exists. Therefore, we will set the condition and then envoke
                //  an interrupt. Hopefully the guest will check the port change.
                BX_UHCI_THIS hub.usb_port[portnum].over_current_change = 1;
                BX_UHCI_THIS hub.usb_port[portnum].over_current = 1;
                //BX_DEBUG(("Over-current signaled on port #%d.", portnum + 1));
                BX_UHCI_THIS update_irq();
            }
        }
        else {
            //BX_ERROR(("Over-current: Bad portnum given: %d", portnum + 1));
        }
    }

    return 0; // clear the indicator for next time
}

// USB runtime parameter enable handler
bool bx_usb_uhci_c::usb_param_enable_handler(bx_param_c* param, bool en)
{
    int portnum = atoi((param->get_parent())->get_name() + 4) - 1;
    if (en && (BX_UHCI_THIS hub.usb_port[portnum].device != NULL)) {
        en = 0;
    }
    return en;
}