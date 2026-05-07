#pragma once

#if BX_USE_UM_SMF
#  define BX_UM_SMF  static
#  define BX_UM_THIS theUnmappedDevice->
#else
#  define BX_UM_SMF
#  define BX_UM_THIS this->
#endif

class bx_unmapped_c  {
public:

    virtual void init(void);
private:

    static Bit32u read_handler(void* this_ptr, Bit32u address, unsigned io_len);
    static void   write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);
    struct {
        Bit8u port80;
        Bit8u port8e;
        Bit8u shutdown;
        bool port_e9_hack;
    } s;
};
