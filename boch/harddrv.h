#ifndef BX_IODEV_HARDDRV_H
#define BX_IODEV_HARDDRV_H

#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF   0x20
#define ATA_SR_DSC  0x10
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

#define ATA_ER_ABRT 0x04

class bx_hard_drive_c : public bx_hard_drive_stub_c {
public:
  bx_hard_drive_c();
  virtual ~bx_hard_drive_c() {}

  virtual void init(void);
  virtual void reset(unsigned type);
  virtual void register_state(void) {}
  virtual Bit32u virt_read_handler(Bit32u address, unsigned io_len);
  virtual void virt_write_handler(Bit32u address, Bit32u value, unsigned io_len);
  virtual bool bmdma_read_sector(Bit8u channel, Bit8u *buffer, Bit32u *sector_size);
  virtual bool bmdma_write_sector(Bit8u channel, Bit8u *buffer);
  virtual void bmdma_complete(Bit8u channel);

  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);

private:
  struct channel_t {
    Bit16u io_base;
    Bit16u ctrl_base;
    Bit8u irq;
    Bit8u selected;
    Bit8u feature;
    Bit8u error;
    Bit8u sector_count;
    Bit8u sector_no;
    Bit8u cylinder_low;
    Bit8u cylinder_high;
    Bit8u drive_head;
    Bit8u status;
    Bit8u control;
  };

  Bit8u read(channel_t &channel, Bit16u offset, bool alternate);
  void write(channel_t &channel, Bit16u offset, Bit8u value);
  void write_command(channel_t &channel, Bit8u command);
  void reset_channel(channel_t &channel);
  void abort_command(channel_t &channel);
  void raise_interrupt(channel_t &channel);
  void lower_interrupt(channel_t &channel);
  channel_t *channel_from_address(Bit32u address, Bit16u *offset, bool *alternate);

  channel_t channels[2];
};

#endif
