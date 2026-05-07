#include "iodev.h"
#include "pci.h"
#include "debug.h"

#define LOG_THIS thePciBridge->

bx_pci_bridge_c* thePciBridge = NULL;


PLUGIN_ENTRY_FOR_MODULE(pci)
{
    if (mode == PLUGIN_INIT) {
        thePciBridge = new bx_pci_bridge_c();
        BX_REGISTER_DEVICE_DEVMODEL(plugin, type, thePciBridge, BX_PLUGIN_PCI);
    }
    else if (mode == PLUGIN_FINI) {
        delete thePciBridge;
    }
    else if (mode == PLUGIN_PROBE) {
        return (int)PLUGTYPE_CORE;
    }
    return 0; // Success
}