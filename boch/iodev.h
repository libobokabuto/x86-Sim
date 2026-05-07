#pragma once
#include "bochs.h"
#include "plugin.h"
#include "param_names.h"
#include "pc_system.h"
#include "memory-bochs.h"
#include "siminterface.h"
typedef Bit32u(*bx_read_handler_t)(void*, Bit32u, unsigned);//59
typedef void   (*bx_write_handler_t)(void*, Bit32u, Bit32u, unsigned); //60

class BOCHSAPI bx_devmodel_c {
public:
    virtual void init(void) {}
};

#if BX_SUPPORT_PCI
//#define BX_DEBUG_PCI_READ(addr, value, io_len) \
  if (io_len == 1) \
    BX_DEBUG(("read  PCI register 0x%02X value 0x%02X (len=1)", address, value)); \
  else if (io_len == 2) \
    BX_DEBUG(("read  PCI register 0x%02X value 0x%04X (len=2)", address, value)); \
  else if (io_len == 4) \
    BX_DEBUG(("read  PCI register 0x%02X value 0x%08X (len=4)", address, value));

//#define BX_DEBUG_PCI_WRITE(addr, value, io_len) \
  if (io_len == 1) \
    BX_DEBUG(("write PCI register 0x%02X value 0x%02X (len=1)", addr, value)); \
  else if (io_len == 2) \
    BX_DEBUG(("write PCI register 0x%02X value 0x%04X (len=2)", addr, value)); \
  else if (io_len == 4) \
    BX_DEBUG(("write PCI register 0x%02X value 0x%08X (len=4)", addr, value));

enum {
    BX_PCI_BAR_TYPE_NONE = 0,
    BX_PCI_BAR_TYPE_MEM = 1,
    BX_PCI_BAR_TYPE_IO = 2
};

#define BX_PCI_ADVOPT_NOACPI 0x01
#define BX_PCI_ADVOPT_NOHPET 0x02
#define BX_PCI_ADVOPT_NOAGP  0x04

typedef struct {
    //132
    Bit8u  type;
    Bit32u size;
    Bit32u addr;
    union {
        struct {
            memory_handler_t rh;
            memory_handler_t wh;
            const Bit8u* dummy;
        } mem;
        struct {
            bx_read_handler_t rh;
            bx_write_handler_t wh;
            const Bit8u* mask;
        } io;
    };
} bx_pci_bar_t;

class BOCHSAPI bx_pci_device_c : public bx_devmodel_c {
    //150
public:
    bx_pci_device_c() : pci_rom(NULL), pci_rom_size(0) {
        for (int i = 0; i < 6; i++) memset(&pci_bar[i], 0, sizeof(bx_pci_bar_t));
    }
    virtual ~bx_pci_device_c() {
        if (pci_rom != NULL) delete[] pci_rom;
    }
protected:
    const char* pci_name;
    Bit8u pci_conf[256];
    bx_pci_bar_t pci_bar[6];
    Bit8u* pci_rom;
    Bit32u pci_rom_address;
    Bit32u pci_rom_size;
    memory_handler_t pci_rom_read_handler;


};
#endif
class BOCHSAPI bx_devices_c {
    //374-614
public:
	bx_devices_c();
	~bx_devices_c();
    void init(BX_MEM_C*);//383
    BX_MEM_C* mem;//392
    bool register_default_io_read_handler(void* this_ptr, bx_read_handler_t f, const char* name, Bit8u mask);//411
    bool register_default_io_write_handler(void* this_ptr, bx_write_handler_t f, const char* name, Bit8u mask);//412
    void   outp(Bit16u addr, Bit32u value, unsigned io_len) BX_CPP_AttrRegparmN(3); //416
private:

    struct io_handler_struct { //511
        struct io_handler_struct* next;
        struct io_handler_struct* prev;
        void* funct; // C++ type checking is great, but annoying
        void* this_ptr;
        char* handler_name;  // name of device
        int usage_count;
        Bit8u mask;          // io_len mask
    };
    struct io_handler_struct io_read_handlers;//520
    struct io_handler_struct io_write_handlers;//521
#define PORTS 0x10000
    struct io_handler_struct** read_port_to_handler;
    struct io_handler_struct** write_port_to_handler; //524
    static Bit32u default_read_handler(void* this_ptr, Bit32u address, unsigned io_len);//535
    static void   default_write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);//536

};


BOCHSAPI extern bx_devices_c bx_devices;