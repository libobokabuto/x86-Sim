#define BX_PLUGGABLE

#include "iodev.h"
#include "harddrv.h"

bx_hard_drive_c *theHardDrive = NULL;

PLUGIN_ENTRY_FOR_MODULE(harddrv)
{
  if (mode == PLUGIN_INIT) {
    theHardDrive = new bx_hard_drive_c();
    bx_devices.pluginHardDrive = theHardDrive;
    BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theHardDrive, BX_PLUGIN_HARDDRV);
  } else if (mode == PLUGIN_FINI) {
    bx_devices.pluginHardDrive = &bx_devices.stubHardDrive;
    delete theHardDrive;
    theHardDrive = NULL;
  } else if (mode == PLUGIN_PROBE) {
    return (int)PLUGTYPE_STANDARD;
  }
  return 0;
}

bx_hard_drive_c::bx_hard_drive_c()
{
  memset(channels, 0, sizeof(channels));
  channels[0].io_base = 0x01f0;
  channels[0].ctrl_base = 0x03f6;
  channels[0].irq = 14;
  channels[1].io_base = 0x0170;
  channels[1].ctrl_base = 0x0376;
  channels[1].irq = 15;
}

void bx_hard_drive_c::init(void)
{
  DEV_register_irq(14, "Primary IDE");
  DEV_register_irq(15, "Secondary IDE");

  for (Bit32u addr = 0x01f0; addr <= 0x01f7; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "Primary IDE", 7);
    DEV_register_iowrite_handler(this, write_handler, addr, "Primary IDE", 7);
  }
  for (Bit32u addr = 0x0170; addr <= 0x0177; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "Secondary IDE", 7);
    DEV_register_iowrite_handler(this, write_handler, addr, "Secondary IDE", 7);
  }
  DEV_register_ioread_handler(this, read_handler, 0x0376, "Secondary IDE", 1);
  DEV_register_iowrite_handler(this, write_handler, 0x0376, "Secondary IDE", 1);

  DEV_cmos_set_reg(0x12, 0x00);
  DEV_cmos_set_reg(0x39, 0x00);
  DEV_cmos_set_reg(0x3a, 0x00);
  DEV_cmos_checksum();

  reset(BX_RESET_HARDWARE);
}

void bx_hard_drive_c::reset(unsigned type)
{
  UNUSED(type);
  reset_channel(channels[0]);
  reset_channel(channels[1]);
}

Bit32u bx_hard_drive_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
  return ((bx_hard_drive_c *)this_ptr)->virt_read_handler(address, io_len);
}

void bx_hard_drive_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
  ((bx_hard_drive_c *)this_ptr)->virt_write_handler(address, value, io_len);
}

Bit32u bx_hard_drive_c::virt_read_handler(Bit32u address, unsigned io_len)
{
  Bit32u value = 0;

  for (unsigned i = 0; i < io_len; i++) {
    Bit16u offset = 0;
    bool alternate = false;
    channel_t *channel = channel_from_address(address + i, &offset, &alternate);
    Bit8u byte = (channel != NULL) ? read(*channel, offset, alternate) : 0xff;
    value |= ((Bit32u)byte) << (i * 8);
  }
  return value;
}

void bx_hard_drive_c::virt_write_handler(Bit32u address, Bit32u value, unsigned io_len)
{
  for (unsigned i = 0; i < io_len; i++) {
    Bit16u offset = 0;
    bool alternate = false;
    channel_t *channel = channel_from_address(address + i, &offset, &alternate);
    if (channel != NULL) {
      if (alternate) {
        channel->control = (Bit8u)value;
        if (value & 0x04) {
          reset_channel(*channel);
        }
      } else {
        write(*channel, offset, (Bit8u)value);
      }
    }
    value >>= 8;
  }
}

bool bx_hard_drive_c::bmdma_read_sector(Bit8u channel, Bit8u *buffer, Bit32u *sector_size)
{
  UNUSED(channel);
  UNUSED(buffer);
  if (sector_size != NULL) {
    *sector_size = 512;
  }
  return false;
}

bool bx_hard_drive_c::bmdma_write_sector(Bit8u channel, Bit8u *buffer)
{
  UNUSED(channel);
  UNUSED(buffer);
  return false;
}

void bx_hard_drive_c::bmdma_complete(Bit8u channel)
{
  if (channel < 2) {
    raise_interrupt(channels[channel]);
  }
}

Bit8u bx_hard_drive_c::read(channel_t &channel, Bit16u offset, bool alternate)
{
  switch (offset) {
    case 0:
      return 0;
    case 1:
      return channel.error;
    case 2:
      return channel.sector_count;
    case 3:
      return channel.sector_no;
    case 4:
      return channel.cylinder_low;
    case 5:
      return channel.cylinder_high;
    case 6:
      return channel.drive_head;
    case 7:
      if (!alternate) {
        lower_interrupt(channel);
      }
      return channel.status;
    default:
      return 0xff;
  }
}

void bx_hard_drive_c::write(channel_t &channel, Bit16u offset, Bit8u value)
{
  switch (offset) {
    case 1:
      channel.feature = value;
      break;
    case 2:
      channel.sector_count = value;
      break;
    case 3:
      channel.sector_no = value;
      break;
    case 4:
      channel.cylinder_low = value;
      break;
    case 5:
      channel.cylinder_high = value;
      break;
    case 6:
      channel.drive_head = value;
      channel.selected = (value >> 4) & 1;
      break;
    case 7:
      write_command(channel, value);
      break;
    default:
      break;
  }
}

void bx_hard_drive_c::write_command(channel_t &channel, Bit8u command)
{
  switch (command) {
    case 0x08:
    case 0x10:
    case 0x20:
    case 0x21:
    case 0x24:
    case 0x30:
    case 0x34:
    case 0x90:
    case 0xa0:
    case 0xa1:
    case 0xec:
      abort_command(channel);
      break;
    case 0xef:
      channel.error = 0x01;
      channel.status = ATA_SR_DRDY | ATA_SR_DSC;
      raise_interrupt(channel);
      break;
    default:
      abort_command(channel);
      break;
  }
}

void bx_hard_drive_c::reset_channel(channel_t &channel)
{
  lower_interrupt(channel);
  channel.selected = 0;
  channel.feature = 0;
  channel.error = 0x01;
  channel.sector_count = 0x01;
  channel.sector_no = 0x01;
  channel.cylinder_low = 0x00;
  channel.cylinder_high = 0x00;
  channel.drive_head = 0xa0;
  channel.status = ATA_SR_DRDY | ATA_SR_DSC;
  channel.control = 0x00;
}

void bx_hard_drive_c::abort_command(channel_t &channel)
{
  channel.error = ATA_ER_ABRT;
  channel.status = ATA_SR_DRDY | ATA_SR_DSC | ATA_SR_ERR;
  raise_interrupt(channel);
}

void bx_hard_drive_c::raise_interrupt(channel_t &channel)
{
  if ((channel.control & 0x02) == 0) {
    DEV_pic_raise_irq(channel.irq);
  }
}

void bx_hard_drive_c::lower_interrupt(channel_t &channel)
{
  DEV_pic_lower_irq(channel.irq);
}

bx_hard_drive_c::channel_t *bx_hard_drive_c::channel_from_address(Bit32u address,
  Bit16u *offset, bool *alternate)
{
  for (unsigned channel = 0; channel < 2; channel++) {
    Bit32u io_base = channels[channel].io_base;
    if ((address >= io_base) && (address <= io_base + 7)) {
      *offset = (Bit16u)(address - io_base);
      *alternate = false;
      return &channels[channel];
    }
    if (address == channels[channel].ctrl_base) {
      *offset = 7;
      *alternate = true;
      return &channels[channel];
    }
  }
  return NULL;
}
