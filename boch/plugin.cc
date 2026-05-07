#include "bochs.h"
#include "iodev.h"
#include "plugin.h"
//logfunctions* pluginlog; //66
device_t* devices = NULL;//101
device_t* core_devices = NULL;//102

void pluginRegisterDeviceDevmodel(plugin_t* plugin, Bit16u type, bx_devmodel_c* devmodel, const char* name)
{
    //686
    device_t** devlist;

    device_t* device = new device_t;

    device->name = name;
    //BX_ASSERT(devmodel != NULL);
    device->devmodel = devmodel;
    device->plugin = plugin;  // this can be NULL
    device->next = NULL;
    device->plugtype = type;

    switch (type) {
    case PLUGTYPE_CORE:
    case PLUGTYPE_VGA:
        devlist = &core_devices;
        break;
    case PLUGTYPE_STANDARD:
    case PLUGTYPE_OPTIONAL:
    default:
        devlist = &devices;
        break;
    }

    if (!*devlist) {
        /* Empty list, this become the first entry. */
        *devlist = device;
    }
    else {
        /* Non-empty list.  Add to end. */
        device_t* temp = *devlist;

        while (temp->next)
            temp = temp->next;

        temp->next = device;
    }
}


void bx_init_plugins()
{
    //833
    device_t* device;

    for (device = core_devices; device; device = device->next) {
        //pluginlog->info("init_dev of '%s' plugin device by virtual method", device->name);
        device->devmodel->init();
    }
    for (device = devices; device; device = device->next) {
        if (device->plugtype == PLUGTYPE_STANDARD) {
            //pluginlog->info("init_dev of '%s' plugin device by virtual method", device->name);
            device->devmodel->init();
        }
    }
    for (device = devices; device; device = device->next) {
        if (device->plugtype == PLUGTYPE_OPTIONAL) {
            //pluginlog->info("init_dev of '%s' plugin device by virtual method", device->name);
            device->devmodel->init();
        }
    }
}