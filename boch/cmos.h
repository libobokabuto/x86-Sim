#ifndef BX_IODEV_CMOS_H
#if BX_USE_CMOS_SMF
#  define BX_CMOS_SMF  static
#  define BX_CMOS_THIS theCmosDevice->
#else
#  define BX_CMOS_SMF
#  define BX_CMOS_THIS this->
#endif
class bx_cmos_c : public bx_cmos_stub_c {
public:
	bx_cmos_c();
    virtual void init(void);
    virtual void checksum_cmos(void);
    virtual Bit32u get_reg(Bit8u reg) {
        return s.reg[reg];
    }
    virtual void set_reg(Bit8u reg, Bit32u val) {
        s.reg[reg] = val;
    }
    struct {
        int     periodic_timer_index;
        Bit32u  periodic_interval_usec;
        int     one_second_timer_index;
        int     uip_timer_index;
        Bit64s  timeval;                //Changed this from time_t to Bit64s - this struct seems to not be referenced ouside of this class despite being public
        Bit8u   cmos_mem_address;
        Bit8u   cmos_ext_mem_addr;
        bool    timeval_change;
        bool    rtc_mode_12hour;
        bool    rtc_mode_binary;
        bool    rtc_sync;
        bool    irq_enabled;

        Bit8u   reg[256];
        Bit8u   max_reg;

        bool    use_image;
    } s;

private:

    static Bit32u read_handler(void* this_ptr, Bit32u address, unsigned io_len);
    static void   write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);

private:
    BX_CMOS_SMF void update_clock(void);
    BX_CMOS_SMF void update_timeval(void);
    BX_CMOS_SMF void CRA_change(void);
};

#endif