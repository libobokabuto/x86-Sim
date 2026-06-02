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

bool pluginDevicePresent(const char* name)
{
    device_t* device;

    for (device = devices; device; device = device->next)
    {
        if (!strcmp(name, device->name)) return 1;
    }

    return 0;
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

void bx_reset_plugins(unsigned signal)
{
    device_t* device;

    for (device = core_devices; device; device = device->next) {
        //pluginlog->info("reset of '%s' plugin device by virtual method", device->name);
        device->devmodel->reset(signal);
    }
    for (device = devices; device; device = device->next) {
        if (device->plugtype == PLUGTYPE_STANDARD) {
            //pluginlog->info("reset of '%s' plugin device by virtual method", device->name);
            device->devmodel->reset(signal);
        }
    }
    for (device = devices; device; device = device->next) {
        if (device->plugtype == PLUGTYPE_OPTIONAL) {
            //pluginlog->info("reset of '%s' plugin device by virtual method", device->name);
            device->devmodel->reset(signal);
        }
    }
}

void bx_plugins_register_state(void)
{
}

void bx_plugins_after_restore_state(void)
{
}

#define BUILTIN_OPT_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_OPTIONAL, 0, lib##mod##_plugin_entry, 0}
#define BUILTIN_OPTPCI_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_OPTIONAL, PLUGFLAG_PCI, lib##mod##_plugin_entry, 0}
#define BUILTIN_VGA_PLUGIN_ENTRY(mod, t, f) {#mod, PLUGTYPE_VGA | t, f, lib##mod##_plugin_entry, 0}
#define BUILTIN_CI_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_CI, 0, lib##mod##_plugin_entry, 0} //978
#define BUILTIN_GUI_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_GUI, 0, lib##mod##_gui_plugin_entry, 0}//979
#define BUILTIN_GUICI_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_GUI | PLUGTYPE_CI, 0, lib##mod##_gui_plugin_entry, 0}
#define BUILTIN_IMG_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_IMG, 0, lib##mod##_img_plugin_entry, 0}
#define BUILTIN_NET_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_NET, 0, libeth_##mod##_plugin_entry, 0}
#define BUILTIN_SND_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_SND, 0, libsound##mod##_plugin_entry, 0}
#define BUILTIN_USB_PLUGIN_ENTRY(mod) {#mod, PLUGTYPE_USB, 0, lib##mod##_plugin_entry, 0}


plugin_t bx_builtin_plugins[] = {
  BUILTIN_OPT_PLUGIN_ENTRY(unmapped),
  BUILTIN_OPT_PLUGIN_ENTRY(biosdev),
  BUILTIN_OPT_PLUGIN_ENTRY(speaker),
  BUILTIN_OPT_PLUGIN_ENTRY(parallel),
  BUILTIN_OPT_PLUGIN_ENTRY(serial),
  BUILTIN_OPTPCI_PLUGIN_ENTRY(usb_uhci),
  /*
#if BX_SUPPORT_BUSMOUSE
  BUILTIN_OPT_PLUGIN_ENTRY(busmouse),
#endif
#if BX_SUPPORT_E1000
  BUILTIN_OPTPCI_PLUGIN_ENTRY(e1000),
#endif
#if BX_SUPPORT_ES1370
  BUILTIN_OPTPCI_PLUGIN_ENTRY(es1370),
#endif
#if BX_SUPPORT_GAMEPORT
  BUILTIN_OPT_PLUGIN_ENTRY(gameport),
#endif
#if BX_SUPPORT_IODEBUG
  BUILTIN_OPT_PLUGIN_ENTRY(iodebug),
#endif
#if BX_SUPPORT_NE2K
  BUILTIN_OPTPCI_PLUGIN_ENTRY(ne2k),
#endif
#if BX_SUPPORT_PCIDEV
  BUILTIN_OPTPCI_PLUGIN_ENTRY(pcidev),
#endif
#if BX_SUPPORT_PCIPNIC
  BUILTIN_OPTPCI_PLUGIN_ENTRY(pcipnic),
#endif
#if BX_SUPPORT_SB16
  BUILTIN_OPT_PLUGIN_ENTRY(sb16),
#endif
#if BX_SUPPORT_USB_UHCI
  BUILTIN_OPTPCI_PLUGIN_ENTRY(usb_uhci),
#endif
#if BX_SUPPORT_USB_OHCI
  BUILTIN_OPTPCI_PLUGIN_ENTRY(usb_ohci),
#endif
#if BX_SUPPORT_USB_EHCI
  BUILTIN_OPTPCI_PLUGIN_ENTRY(usb_ehci),
#endif
#if BX_SUPPORT_USB_XHCI
  BUILTIN_OPTPCI_PLUGIN_ENTRY(usb_xhci),
#endif
#if BX_SUPPORT_SOUNDLOW
  BUILTIN_SND_PLUGIN_ENTRY(dummy),
  BUILTIN_SND_PLUGIN_ENTRY(file),
#if BX_HAVE_SOUND_ALSA
  BUILTIN_SND_PLUGIN_ENTRY(alsa),
#endif
#if BX_HAVE_SOUND_OSS
  BUILTIN_SND_PLUGIN_ENTRY(oss),
#endif
#if BX_HAVE_SOUND_OSX
  BUILTIN_SND_PLUGIN_ENTRY(osx),
#endif
#if BX_HAVE_SOUND_PULSE
  BUILTIN_SND_PLUGIN_ENTRY(pulse),
#endif
#if BX_HAVE_SOUND_SDL
  BUILTIN_SND_PLUGIN_ENTRY(sdl),
#endif
#if BX_HAVE_SOUND_WIN
  BUILTIN_SND_PLUGIN_ENTRY(win),
#endif
#endif
#if BX_NETWORKING
  BUILTIN_NET_PLUGIN_ENTRY(null),
  BUILTIN_NET_PLUGIN_ENTRY(vnet),
#if BX_NETMOD_FBSD
  BUILTIN_NET_PLUGIN_ENTRY(fbsd),
#endif
#if BX_NETMOD_LINUX
  BUILTIN_NET_PLUGIN_ENTRY(linux),
#endif
#if BX_NETMOD_SLIRP
  BUILTIN_NET_PLUGIN_ENTRY(slirp),
#endif
#if BX_NETMOD_SOCKET
  BUILTIN_NET_PLUGIN_ENTRY(socket),
#endif
#if BX_NETMOD_TAP
  BUILTIN_NET_PLUGIN_ENTRY(tap),
#endif
#if BX_NETMOD_TUNTAP
  BUILTIN_NET_PLUGIN_ENTRY(tuntap),
#endif
#if BX_NETMOD_VDE
  BUILTIN_NET_PLUGIN_ENTRY(vde),
#endif
#if BX_NETMOD_WIN32
  BUILTIN_NET_PLUGIN_ENTRY(win32),
#endif
#endif
#if BX_SUPPORT_PCIUSB
  BUILTIN_USB_PLUGIN_ENTRY(usb_floppy),
  BUILTIN_USB_PLUGIN_ENTRY(usb_hid),
  BUILTIN_USB_PLUGIN_ENTRY(usb_hub),
  BUILTIN_USB_PLUGIN_ENTRY(usb_msd),
  BUILTIN_USB_PLUGIN_ENTRY(usb_printer),
#endif
  BUILTIN_IMG_PLUGIN_ENTRY(vmware3),
  BUILTIN_IMG_PLUGIN_ENTRY(vmware4),
  BUILTIN_IMG_PLUGIN_ENTRY(vbox),
  BUILTIN_IMG_PLUGIN_ENTRY(vpc),
  BUILTIN_IMG_PLUGIN_ENTRY(vvfat),
  */
  {"NULL", PLUGTYPE_NULL, 0, NULL, 0}
};

Bit8u bx_get_plugins_count_np(Bit16u type)
{ //1146
    int i = 0;
    Bit8u count = 0;

    while (strcmp(bx_builtin_plugins[i].name, "NULL")) {
        if ((type & bx_builtin_plugins[i].type) != 0)
            count++;
        i++;
    }
    return count;
}

const char* bx_get_plugin_name_np(Bit16u type, Bit8u index)
{//1159
    int i = 0;
    Bit8u count = 0;

    while (strcmp(bx_builtin_plugins[i].name, "NULL")) {
        if ((type & bx_builtin_plugins[i].type) != 0) {
            if (count == index)
                return bx_builtin_plugins[i].name;
            count++;
        }
        i++;
    }
    return NULL;
}

Bit8u bx_get_plugin_flags_np(Bit16u type, Bit8u index)
{
    int i = 0;
    Bit8u count = 0;

    while (strcmp(bx_builtin_plugins[i].name, "NULL")) {
        if ((type & bx_builtin_plugins[i].type) != 0) {
            if (count == index)
                return bx_builtin_plugins[i].flags;
            count++;
        }
        i++;
    }
    return 0;
}


int bx_load_plugin_np(const char* name, Bit16u type)
{
    //1191
    int i = 0;
    while (strcmp(bx_builtin_plugins[i].name, "NULL")) {
        if ((!strcmp(name, bx_builtin_plugins[i].name)) &&
            ((type & bx_builtin_plugins[i].type) != 0)) {
            if (bx_builtin_plugins[i].initialized == 0) {
                bx_builtin_plugins[i].loadtype = type;
                bx_builtin_plugins[i].plugin_entry(NULL, type, PLUGIN_INIT);
                bx_builtin_plugins[i].initialized = 1;
            }
            else {
                //BX_PANIC(("plugin '%s' already loaded", name));
            }
            return 1;
        }
        i++;
    }
    return 0;
}

int bx_unload_opt_plugin(const char* name, bool devflag)
{
    UNUSED(devflag);

    int i = 0;
    while (strcmp(bx_builtin_plugins[i].name, "NULL")) {
        if ((!strcmp(name, bx_builtin_plugins[i].name)) &&
            (((bx_builtin_plugins[i].type & PLUGTYPE_OPTIONAL) != 0) ||
             ((bx_builtin_plugins[i].type & PLUGTYPE_VGA) != 0))) {
            if (bx_builtin_plugins[i].initialized != 0) {
                bx_builtin_plugins[i].plugin_entry(NULL, bx_builtin_plugins[i].loadtype, PLUGIN_FINI);
                bx_builtin_plugins[i].loadtype = PLUGTYPE_NULL;
                bx_builtin_plugins[i].initialized = 0;
                return 1;
            }
            return 0;
        }
        i++;
    }
    return 0;
}
