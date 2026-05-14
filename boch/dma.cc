#define BX_PLUGGABLE

#include "iodev.h"
#include "dma.h"

#include "debug.h"

#define LOG_THIS theDmaDevice->
bx_dma_c* theDmaDevice = NULL;
PLUGIN_ENTRY_FOR_MODULE(dma)
{
    if (mode == PLUGIN_INIT) {
        theDmaDevice = new bx_dma_c();
        bx_devices.pluginDmaDevice = theDmaDevice;
        BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theDmaDevice, BX_PLUGIN_DMA);
    }
    else if (mode == PLUGIN_FINI) {
        delete theDmaDevice;
    }
    else if (mode == PLUGIN_PROBE) {
        return (int)PLUGTYPE_CORE;
    }
    return 0; // Success
}
bx_dma_c::bx_dma_c()
{
    //put("DMA");
    memset(&s, 0, sizeof(s));
}

bx_dma_c::~bx_dma_c()
{
    //SIM->get_bochs_root()->remove("dma");
    //BX_DEBUG(("Exit"));
}
