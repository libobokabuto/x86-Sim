#pragma once

#define BX_IODEV_PARPORT_H

#if BX_USE_PAR_SMF
#  define BX_PAR_SMF  static
#  define BX_PAR_THIS theParallelDevice->
#else
#  define BX_PAR_SMF
#  define BX_PAR_THIS this->
#endif

#define BX_PARPORT_MAXDEV   2


enum {
	BX_PAR_DATA = 0,
	BX_PAR_STAT = 1,
	BX_PAR_CTRL = 2
};

typedef struct {
    Bit8u data;
    struct {
        bool error;
        bool slct;
        bool pe;
        bool ack;
        bool busy;
    } STATUS;
    struct {
        bool strobe;
        bool autofeed;
        bool init;
        bool slct_in;
        bool irq;
        bool input;
    } CONTROL;
    Bit8u IRQ;
    bx_param_string_c* file;
    FILE* output;
    bool file_changed;
    bool initmode;
} bx_par_t;


class bx_parallel_c : public bx_devmodel_c {
public:
    bx_parallel_c();
    virtual ~bx_parallel_c();
    virtual void init(void);
    virtual void reset(unsigned type);

private:
    bx_par_t s[BX_PARPORT_MAXDEV];
    static void   virtual_printer(Bit8u port);
    static Bit32u read_handler(void* this_ptr, Bit32u address, unsigned io_len);
    static void   write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);

};