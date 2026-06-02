#define BX_PLUGGABLE

#include "iodev.h"
#include "floppy.h"

bx_floppy_ctrl_c *theFloppyController = NULL;

PLUGIN_ENTRY_FOR_MODULE(floppy)
{
  if (mode == PLUGIN_INIT) {
    theFloppyController = new bx_floppy_ctrl_c();
    BX_REGISTER_DEVICE_DEVMODEL(plugin, type, theFloppyController, BX_PLUGIN_FLOPPY);
  } else if (mode == PLUGIN_FINI) {
    delete theFloppyController;
    theFloppyController = NULL;
  } else if (mode == PLUGIN_PROBE) {
    return (int)PLUGTYPE_CORE;
  }
  return 0;
}

bx_floppy_ctrl_c::bx_floppy_ctrl_c()
{
  memset(&s, 0, sizeof(s));
  s.DSR = 2;
  s.DOR = 0x0c;
  enter_idle_phase();
}

void bx_floppy_ctrl_c::init(void)
{
  DEV_register_irq(6, "Floppy Drive");
  for (Bit32u addr = 0x03f0; addr <= 0x03f7; addr++) {
    DEV_register_ioread_handler(this, read_handler, addr, "Floppy Drive", 1);
    DEV_register_iowrite_handler(this, write_handler, addr, "Floppy Drive", 1);
  }
  DEV_dma_register_8bit_channel(FLOPPY_DMA_CHAN, dma_write, dma_read, "Floppy Drive");

  DEV_cmos_set_reg(0x10, 0x00);
  DEV_cmos_set_reg(0x14, DEV_cmos_get_reg(0x14) & 0xfe);
  DEV_cmos_checksum();

  reset(BX_RESET_HARDWARE);
}

void bx_floppy_ctrl_c::reset(unsigned type)
{
  UNUSED(type);
  lower_interrupt();
  s.DSR = 2;
  s.DOR = 0x0c;
  s.status_reg0 = 0;
  s.cylinder[0] = 0;
  s.cylinder[1] = 0;
  s.command_index = 0;
  s.command_size = 0;
  s.result_index = 0;
  s.result_size = 0;
  s.reset_sensei = 0;
  enter_idle_phase();
}

Bit32u bx_floppy_ctrl_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
  bx_floppy_ctrl_c *fdc = (bx_floppy_ctrl_c *)this_ptr;
  Bit32u value = 0xff;

  for (unsigned i = 0; i < io_len; i++) {
    value &= ~(0xffu << (i * 8));
    value |= ((Bit32u)fdc->read(address + i)) << (i * 8);
  }
  return value;
}

void bx_floppy_ctrl_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
  bx_floppy_ctrl_c *fdc = (bx_floppy_ctrl_c *)this_ptr;

  for (unsigned i = 0; i < io_len; i++) {
    fdc->write(address + i, (Bit8u)(value & 0xff));
    value >>= 8;
  }
}

Bit16u bx_floppy_ctrl_c::dma_write(Bit8u *buffer, Bit16u maxlen)
{
  UNUSED(buffer);
  UNUSED(maxlen);
  return 0;
}

Bit16u bx_floppy_ctrl_c::dma_read(Bit8u *buffer, Bit16u maxlen)
{
  UNUSED(buffer);
  UNUSED(maxlen);
  return 0;
}

Bit8u bx_floppy_ctrl_c::read(Bit32u address)
{
  switch (address & 0x07) {
    case 0x00:
    case 0x01:
      return 0;
    case 0x02:
      return s.DOR;
    case 0x03:
      return 0;
    case 0x04:
      return s.main_status_reg;
    case 0x05:
      if (s.result_index < s.result_size) {
        Bit8u value = s.result[s.result_index++];
        if (s.result_index >= s.result_size) {
          enter_idle_phase();
          lower_interrupt();
        }
        return value;
      }
      return 0;
    case 0x07:
      return 0x80;
    default:
      return 0xff;
  }
}

void bx_floppy_ctrl_c::write(Bit32u address, Bit8u value)
{
  switch (address & 0x07) {
    case 0x02: {
      bool was_reset = (s.DOR & 0x04) != 0;
      bool now_reset = (value & 0x04) != 0;
      s.DOR = value;
      if (!now_reset) {
        lower_interrupt();
        enter_idle_phase();
      } else if (!was_reset) {
        s.status_reg0 = 0xc0;
        s.reset_sensei = 4;
        enter_idle_phase();
        raise_interrupt();
      }
      break;
    }
    case 0x05:
      write_data(value);
      break;
    case 0x07:
      s.DSR = value & 0x83;
      if (value & 0x80) {
        s.status_reg0 = 0xc0;
        s.reset_sensei = 4;
        enter_idle_phase();
        raise_interrupt();
      }
      break;
    default:
      break;
  }
}

void bx_floppy_ctrl_c::write_data(Bit8u value)
{
  if (s.command_index == 0) {
    s.command_size = command_size(value);
    s.main_status_reg = FD_MS_RQM | FD_MS_BUSY;
  }

  if (s.command_index < sizeof(s.command)) {
    s.command[s.command_index++] = value;
  }

  if (s.command_index >= s.command_size) {
    process_command();
  }
}

Bit8u bx_floppy_ctrl_c::command_size(Bit8u command) const
{
  switch (command & 0x1f) {
    case FD_CMD_SPECIFY:
      return 3;
    case FD_CMD_SENSE_DRV:
    case FD_CMD_RECALIBRATE:
    case FD_CMD_READ_ID:
    case FD_CMD_PERP_MODE:
      return 2;
    case FD_CMD_SEEK:
      return 3;
    case FD_CMD_CONFIGURE:
      return 4;
    case FD_CMD_READ_DATA:
    case FD_CMD_WRITE_DATA:
      return 9;
    case FD_CMD_FORMAT_TRACK:
      return 6;
    default:
      return 1;
  }
}

void bx_floppy_ctrl_c::process_command(void)
{
  Bit8u cmd = s.command[0];
  Bit8u base = cmd & 0x1f;
  Bit8u drive = (s.command_size > 1) ? (s.command[1] & FDC_DRV_MASK) : (s.DOR & FDC_DRV_MASK);
  Bit8u result[16];

  memset(result, 0, sizeof(result));
  switch (base) {
    case FD_CMD_SPECIFY:
    case FD_CMD_CONFIGURE:
    case FD_CMD_PERP_MODE:
      enter_idle_phase();
      break;

    case FD_CMD_SENSE_DRV:
      result[0] = drive;
      enter_result_phase(result, 1);
      break;

    case FD_CMD_SENSE_INTERRUPT:
      if (s.reset_sensei > 0) {
        drive = (Bit8u)(4 - s.reset_sensei);
        s.reset_sensei--;
        result[0] = (Bit8u)(0xc0 | (drive & FDC_DRV_MASK));
        result[1] = s.cylinder[drive & FDC_DRV_MASK];
      } else if (!s.pending_irq) {
        result[0] = 0x80;
        result[1] = 0;
      } else {
        result[0] = s.status_reg0;
        result[1] = s.cylinder[drive];
      }
      enter_result_phase(result, 2);
      lower_interrupt();
      break;

    case FD_CMD_RECALIBRATE:
      s.cylinder[drive] = 0;
      s.status_reg0 = (Bit8u)(0x20 | drive);
      enter_idle_phase();
      raise_interrupt();
      break;

    case FD_CMD_SEEK:
      s.cylinder[drive] = s.command[2];
      s.status_reg0 = (Bit8u)(0x20 | drive);
      enter_idle_phase();
      raise_interrupt();
      break;

    case FD_CMD_DUMPREG:
      result[0] = s.cylinder[0];
      result[1] = s.cylinder[1];
      result[6] = 0;
      result[7] = 0;
      enter_result_phase(result, 10);
      break;

    case FD_CMD_VERSION:
      result[0] = 0x90;
      enter_result_phase(result, 1);
      break;

    case FD_CMD_LOCK_UNLOCK:
      result[0] = (cmd & 0x80) ? 0x10 : 0x00;
      enter_result_phase(result, 1);
      break;

    case FD_CMD_READ_DATA:
    case FD_CMD_WRITE_DATA:
    case FD_CMD_READ_ID:
    case FD_CMD_FORMAT_TRACK:
      result[0] = (Bit8u)(0x40 | drive);
      result[1] = 0x04;
      result[2] = 0x00;
      result[3] = s.cylinder[drive];
      result[4] = (s.command_size > 3) ? s.command[3] : 0;
      result[5] = (s.command_size > 4) ? s.command[4] : 1;
      result[6] = 2;
      enter_result_phase(result, 7);
      raise_interrupt();
      break;

    default:
      result[0] = 0x80;
      enter_result_phase(result, 1);
      break;
  }
}

void bx_floppy_ctrl_c::enter_idle_phase(void)
{
  s.command_index = 0;
  s.command_size = 0;
  s.result_index = 0;
  s.result_size = 0;
  s.main_status_reg = FD_MS_RQM;
}

void bx_floppy_ctrl_c::enter_result_phase(const Bit8u *data, Bit8u len)
{
  if (len > sizeof(s.result)) {
    len = sizeof(s.result);
  }
  memcpy(s.result, data, len);
  s.result_index = 0;
  s.result_size = len;
  s.command_index = 0;
  s.command_size = 0;
  s.main_status_reg = FD_MS_RQM | FD_MS_DIO | FD_MS_BUSY;
}

void bx_floppy_ctrl_c::raise_interrupt(void)
{
  s.pending_irq = true;
  DEV_pic_raise_irq(6);
}

void bx_floppy_ctrl_c::lower_interrupt(void)
{
  s.pending_irq = false;
  DEV_pic_lower_irq(6);
  DEV_dma_set_drq(FLOPPY_DMA_CHAN, 0);
}
