
#include "iodev.h"
#include "biosdev.h"

bx_biosdev_c* theBiosDevice = NULL;

#define bioslog theBiosDevice
//logfunctions* vgabioslog;
PLUGIN_ENTRY_FOR_MODULE(biosdev)
{
    if (mode == PLUGIN_INIT) {
        theBiosDevice = new bx_biosdev_c();
        BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theBiosDevice, BX_PLUGIN_BIOSDEV);
    }
    else if (mode == PLUGIN_FINI) {
        delete theBiosDevice;
    }
    else if (mode == PLUGIN_PROBE) {
        return (int)PLUGTYPE_OPTIONAL;
    }
    return(0); // Success
}

bx_biosdev_c::bx_biosdev_c(void)
{
    memset(&s, 0, sizeof(s));

    //put("biosdev", "BIOS");

    //vgabioslog = new logfunctions();
    //vgabioslog->put("vgabios", "VBIOS");
}

bx_biosdev_c::~bx_biosdev_c(void)
{
    //bioslog->ldebug("Exit");
    //if (vgabioslog != NULL) {
    //    delete vgabioslog;
   //     vgabioslog = NULL;
   // }
}

void bx_biosdev_c::init(void)
{
    DEV_register_iowrite_handler(this, write_handler, 0x0400, "Bios Panic Port 1", 3);
    DEV_register_iowrite_handler(this, write_handler, 0x0401, "Bios Panic Port 2", 3);
    DEV_register_iowrite_handler(this, write_handler, 0x0402, "Bios Info Port", 1);
    DEV_register_iowrite_handler(this, write_handler, 0x0403, "Bios Debug Port", 1);

    DEV_register_iowrite_handler(this, write_handler, 0x0500, "VGABios Info Port", 1);
    DEV_register_iowrite_handler(this, write_handler, 0x0501, "VGABios Panic Port 1", 3);
    DEV_register_iowrite_handler(this, write_handler, 0x0502, "VGABios Panic Port 2", 3);
    DEV_register_iowrite_handler(this, write_handler, 0x0503, "VGABios Debug Port", 1);
}

void bx_biosdev_c::write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_BIOS_SMF
    bx_biosdev_c* class_ptr = (bx_biosdev_c*)this_ptr;

    class_ptr->write(address, value, io_len);
}

void bx_biosdev_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
    UNUSED(this_ptr);
#endif  // !BX_USE_BIOS_SMF
    UNUSED(io_len);


    switch (address) {
        // 0x400-0x401 are used as panic ports for the rombios
    case 0x0401:
        if (value == 0) {
            // The next message sent to the info port will cause a panic
            BX_BIOS_THIS s.bios_panic_flag = true;
            break;
        }
        else if (BX_BIOS_THIS s.bios_message_i > 0) {
            // if there are bits of message in the buffer, print them as the
            // panic message.  Otherwise fall into the next case.
            if (BX_BIOS_THIS s.bios_message_i >= BX_BIOS_MESSAGE_SIZE)
                BX_BIOS_THIS s.bios_message_i = BX_BIOS_MESSAGE_SIZE - 1;
            BX_BIOS_THIS s.bios_message[BX_BIOS_THIS s.bios_message_i] = 0;
            BX_BIOS_THIS s.bios_message_i = 0;
            //bioslog->panic("%s", BX_BIOS_THIS s.bios_message);
            break;
        }
    case 0x0400:
        if (value > 0) {
            //bioslog->panic("BIOS panic at rombios.c, line %d", value);
        }
        break;

        // 0x0402 is used as the info port for the rombios
        // 0x0403 is used as the debug port for the rombios
    case 0x0402:
    case 0x0403:
        BX_BIOS_THIS s.bios_message[BX_BIOS_THIS s.bios_message_i] =
            (Bit8u)value;
        BX_BIOS_THIS s.bios_message_i++;
        if (BX_BIOS_THIS s.bios_message_i >= BX_BIOS_MESSAGE_SIZE) {
            BX_BIOS_THIS s.bios_message[BX_BIOS_MESSAGE_SIZE - 1] = 0;
            BX_BIOS_THIS s.bios_message_i = 0;
            if (address == 0x403){}
                //bioslog->ldebug("%s", BX_BIOS_THIS s.bios_message);
            else{}
                //bioslog->info("%s", BX_BIOS_THIS s.bios_message);
        }
        else if ((value & 0xff) == '\n') {
            BX_BIOS_THIS s.bios_message[BX_BIOS_THIS s.bios_message_i - 1] = 0;
            BX_BIOS_THIS s.bios_message_i = 0;
            if (BX_BIOS_THIS s.bios_panic_flag){}
                //bioslog->panic("%s", BX_BIOS_THIS s.bios_message);
            else if (address == 0x403){}
               // bioslog->ldebug("%s", BX_BIOS_THIS s.bios_message);
            else{}
                //bioslog->info("%s", BX_BIOS_THIS s.bios_message);
            BX_BIOS_THIS s.bios_panic_flag = false;
        }
        break;

        // 0x501-0x502 are used as panic ports for the vgabios
    case 0x0502:
        if (value == 0) {
            BX_BIOS_THIS s.vgabios_panic_flag = true;
            break;
        }
        else if (BX_BIOS_THIS s.vgabios_message_i > 0) {
            // if there are bits of message in the buffer, print them as the
            // panic message.  Otherwise fall into the next case.
            if (BX_BIOS_THIS s.vgabios_message_i >= BX_BIOS_MESSAGE_SIZE)
                BX_BIOS_THIS s.vgabios_message_i = BX_BIOS_MESSAGE_SIZE - 1;
            BX_BIOS_THIS s.vgabios_message[BX_BIOS_THIS s.vgabios_message_i] = 0;
            BX_BIOS_THIS s.vgabios_message_i = 0;
            //vgabioslog->panic("%s", BX_BIOS_THIS s.vgabios_message);
            break;
        }
    case 0x0501:
        if (value > 0) {
            //vgabioslog->panic("VGABIOS panic at vgabios.c, line %d", value);
        }
        break;

        // 0x0500 is used as the message port for the vgabios
    case 0x0500:
    case 0x0503:
        BX_BIOS_THIS s.vgabios_message[BX_BIOS_THIS s.vgabios_message_i] =
            (Bit8u)value;
        BX_BIOS_THIS s.vgabios_message_i++;
        if (BX_BIOS_THIS s.vgabios_message_i >= BX_BIOS_MESSAGE_SIZE) {
            BX_BIOS_THIS s.vgabios_message[BX_BIOS_MESSAGE_SIZE - 1] = 0;
            BX_BIOS_THIS s.vgabios_message_i = 0;
            if (address == 0x503){}
                //vgabioslog->ldebug("%s", BX_BIOS_THIS s.vgabios_message);
            else{}
                //vgabioslog->info("%s", BX_BIOS_THIS s.vgabios_message);
        }
        else if ((value & 0xff) == '\n') {
            BX_BIOS_THIS s.vgabios_message[BX_BIOS_THIS s.vgabios_message_i - 1] = 0;
            BX_BIOS_THIS s.vgabios_message_i = 0;
            if (BX_BIOS_THIS s.vgabios_panic_flag){}
                //vgabioslog->panic("%s", BX_BIOS_THIS s.vgabios_message);
            else if (address == 0x503){}
                //vgabioslog->ldebug("%s", BX_BIOS_THIS s.vgabios_message);
            else{}
                //vgabioslog->info("%s", BX_BIOS_THIS s.vgabios_message);
            BX_BIOS_THIS s.vgabios_panic_flag = false;
        }
        break;

    default:
        break;
    }
}
