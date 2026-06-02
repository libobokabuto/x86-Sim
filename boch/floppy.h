#ifndef BX_IODEV_FLOPPY_H
#define BX_IODEV_FLOPPY_H

#define FD_MS_RQM  0x80
#define FD_MS_DIO  0x40
#define FD_MS_NDMA 0x20
#define FD_MS_BUSY 0x10

#define FLOPPY_DMA_CHAN 2
#define FDC_DRV_MASK 0x01

enum {
  FD_CMD_SPECIFY          = 0x03,
  FD_CMD_SENSE_DRV       = 0x04,
  FD_CMD_WRITE_DATA       = 0x05,
  FD_CMD_READ_DATA        = 0x06,
  FD_CMD_RECALIBRATE      = 0x07,
  FD_CMD_SENSE_INTERRUPT  = 0x08,
  FD_CMD_READ_ID          = 0x0a,
  FD_CMD_FORMAT_TRACK     = 0x0d,
  FD_CMD_DUMPREG          = 0x0e,
  FD_CMD_SEEK             = 0x0f,
  FD_CMD_VERSION          = 0x10,
  FD_CMD_PERP_MODE        = 0x12,
  FD_CMD_CONFIGURE        = 0x13,
  FD_CMD_LOCK_UNLOCK      = 0x14
};

class bx_floppy_ctrl_c : public bx_devmodel_c {
public:
  bx_floppy_ctrl_c();
  virtual ~bx_floppy_ctrl_c() {}

  virtual void init(void);
  virtual void reset(unsigned type);
  virtual void register_state(void) {}

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
  static Bit16u dma_write(Bit8u *buffer, Bit16u maxlen);
  static Bit16u dma_read(Bit8u *buffer, Bit16u maxlen);

private:
  void write(Bit32u address, Bit8u value);
  Bit8u read(Bit32u address);
  void write_data(Bit8u value);
  void process_command(void);
  void enter_idle_phase(void);
  void enter_result_phase(const Bit8u *data, Bit8u len);
  void raise_interrupt(void);
  void lower_interrupt(void);
  Bit8u command_size(Bit8u command) const;

  struct {
    Bit8u DOR;
    Bit8u DSR;
    Bit8u main_status_reg;
    Bit8u status_reg0;
    Bit8u cylinder[2];
    Bit8u command[16];
    Bit8u command_index;
    Bit8u command_size;
    Bit8u result[16];
    Bit8u result_index;
    Bit8u result_size;
    Bit8u reset_sensei;
    bool pending_irq;
  } s;
};

#endif
