#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include "bochs.h"
#include "cpu.h"
#include "param_names.h"
#include "iodev.h"

#include <io.h> //自己加的
#include <fcntl.h> //自己加的
#include <sys/stat.h> //自己加的

//31
#define BX_MEM_HANDLERS   ((BX_CONST64(1) << BX_PHY_ADDRESS_WIDTH) >> 20) /* one per megabyte */

#define FLASH_READ_ARRAY  0xff
#define FLASH_INT_ID      0x90
#define FLASH_READ_STATUS 0x70
#define FLASH_CLR_STATUS  0x50
#define FLASH_ERASE_SETUP 0x20
#define FLASH_ERASE_SUSP  0xb0
#define FLASH_PROG_SETUP  0x40
#define FLASH_ERASE       0xd0

BX_MEM_C::BX_MEM_C() : BX_MEMORY_STUB_C()
{
	//42
	memory_handlers = NULL;
}
BX_MEM_C::~BX_MEM_C()
{
	//47
	cleanup_memory();
}
void BX_MEM_C::init_memory(Bit64u guest, Bit64u host, Bit32u block_size)
{
	//52
	unsigned idx, i;

	BX_MEMORY_STUB_C::init_memory(guest, host, block_size);

	BX_MEM_THIS smram_available = false;
	BX_MEM_THIS smram_enable = false;
	BX_MEM_THIS smram_restricted = false;

	BX_MEM_THIS memory_handlers = new struct memory_handler_struct* [BX_MEM_HANDLERS];
	for (idx = 0; idx < BX_MEM_HANDLERS; idx++)
		BX_MEM_THIS memory_handlers[idx] = NULL;

	BX_MEM_THIS pci_enabled = 1;
	BX_MEM_THIS bios_write_enabled = false;
	BX_MEM_THIS bios_rom_addr = 0xffff0000;
	BX_MEM_THIS flash_type = 0;
	BX_MEM_THIS flash_status = 0x80;
	BX_MEM_THIS flash_wsm_state = FLASH_READ_ARRAY;
	BX_MEM_THIS flash_modified = false;

	for (i = 0; i < 65; i++)
		BX_MEM_THIS rom_present[i] = false;
	for (i = 0; i <= BX_MEM_AREA_F0000; i++) {
		BX_MEM_THIS memory_type[i][0] = false;
		BX_MEM_THIS memory_type[i][1] = false;
	}

	BX_MEM_THIS register_state();
}


void BX_MEM_C::register_state()
{
	//176
	char param_name[15];

	// bx_list_c* list = new bx_list_c(SIM->get_bochs_root(), "memory", "Memory State");
	Bit32u num_blocks = (Bit32u)(BX_MEM_THIS len / BX_MEM_THIS block_size);
#if BX_LARGE_RAMFILE
	//bx_shadow_filedata_c* ramfile = new bx_shadow_filedata_c(list, "ram", &(BX_MEM_THIS overflow_file));
	//ramfile->set_sr_handlers(this, ramfile_save_handler, (filedata_restore_handler)NULL);
	// BXRS_DEC_PARAM_FIELD(list, next_swapout_idx, BX_MEM_THIS next_swapout_idx);
#else
	// new bx_shadow_data_c(list, "ram", BX_MEM_THIS vector, BX_MEM_THIS allocated);
#endif
	// BXRS_DEC_PARAM_FIELD(list, used_blocks, BX_MEM_THIS used_blocks);
	//bx_list_c* mapping = new bx_list_c(list, "mapping");
	for (Bit32u blk = 0; blk < num_blocks; blk++) {
		sprintf(param_name, "blk%d", blk);
		//bx_param_num_c* param = new bx_param_num_c(mapping, param_name, "", "", -2, BX_MAX_BIT32U, 0);
		//param->set_base(BASE_DEC);
		//param->set_sr_handlers(this, memory_param_save_handler, memory_param_restore_handler);
	}
	//bx_list_c* memtype = new bx_list_c(list, "memtype");
	for (int i = 0; i <= BX_MEM_AREA_F0000; i++) {
		sprintf(param_name, "%d_r", i);
		//new bx_shadow_bool_c(memtype, param_name, &BX_MEM_THIS memory_type[i][0]);
		sprintf(param_name, "%d_w", i);
		//new bx_shadow_bool_c(memtype, param_name, &BX_MEM_THIS memory_type[i][1]);
	}
	//BXRS_HEX_PARAM_FIELD(list, flash_status, BX_MEM_THIS flash_status);
	//BXRS_DEC_PARAM_FIELD(list, flash_wsm_state, BX_MEM_THIS flash_wsm_state);
	//BXRS_PARAM_BOOL(list, flash_modified, BX_MEM_THIS flash_modified);
	//bx_param_bool_c* flash_data = new bx_param_bool_c(list, "flash_data", "", "", false);
	//flash_data->set_sr_handlers(this, memory_param_save_handler, memory_param_restore_handler);
}
void BX_MEM_C::cleanup_memory()
{
	//212
}
void BX_MEM_C::load_ROM(const char* path, bx_phy_address romaddress, Bit8u type)
{
	struct _stat stat_buf;

	int fd, ret, i, start_idx, end_idx;
	unsigned long size, max_size, offset;
	bool is_bochs_bios = false;

	if (*path == '\0') {
		return;
	}

	fd = _open(path, O_RDONLY

#ifdef O_BINARY
		| O_BINARY
#endif
	);
	if (fd < 0) {
		return;
	}

	ret = _fstat(fd, &stat_buf);
	if (ret) {
		_close(fd);
		return;
	}

	size = (unsigned long)stat_buf.st_size;

	if (type > 0) {
		max_size = 0x20000;
	}
	else {
		max_size = BIOSROMSZ;
	}

	if (size > max_size) {
		_close(fd);
		return;
	}

	if (type == 0) {
		if (romaddress > 0) {
			if ((romaddress + size) != 0x100000 && (romaddress + size)) {
				_close(fd);
				return;
			}
		}
		else {
			romaddress = ~(size - 1);
		}

		offset = (unsigned long)(romaddress & BIOS_MASK);

		if ((romaddress & 0xf0000) < 0xf0000) {
			BX_MEM_THIS rom_present[64] = true;
		}

		BX_MEM_THIS bios_rom_addr = (Bit32u)romaddress;

		is_bochs_bios =
			(strstr(path, "BIOS-bochs-latest") != NULL) ||
			(strstr(path, "BIOS-bochs-legacy") != NULL);

		if (size == 0x40000) {
			BX_MEM_THIS flash_type = 2;
		}
		else if (size == 0x20000) {
			BX_MEM_THIS flash_type = 1;
		}
	}
	else {
		if ((size % 512) != 0) {
			_close(fd);
			return;
		}

		if ((romaddress % 2048) != 0) {
			_close(fd);
			return;
		}

		if ((romaddress < 0xc0000) ||
			(((romaddress + size - 1) > 0xdffff) && (romaddress < 0xe0000))) {
			_close(fd);
			return;
		}

		if (romaddress < 0xe0000) {
			offset = (unsigned long)((romaddress & EXROM_MASK) + BIOSROMSZ);
			start_idx = (((Bit32u)romaddress - 0xc0000) >> 11);
			end_idx = start_idx + (size >> 11) + (((size % 2048) > 0) ? 1 : 0);
		}
		else {
			offset = (unsigned long)(romaddress & BIOS_MASK);
			start_idx = 64;
			end_idx = 64;
		}

		for (i = start_idx; i < end_idx; i++) {
			if (BX_MEM_THIS rom_present[i]) {
				_close(fd);
				return;
			}
			else {
				BX_MEM_THIS rom_present[i] = true;
			}
		}
	}

	while (size > 0) {
		ret = _read(fd, (bx_ptr_t)&BX_MEM_THIS rom[offset], (unsigned)size);
		if (ret <= 0) {
			_close(fd);
			return;
		}
		size -= ret;
		offset += ret;
	}

	_close(fd);

	offset -= (unsigned long)stat_buf.st_size;
	size = (unsigned long)stat_buf.st_size;

	if (is_bochs_bios ||
		((BX_MEM_THIS rom[offset] == 0x55) && (BX_MEM_THIS rom[offset + 1] == 0xaa))) {
		if ((type == 0) && ((romaddress & 0xfffff) == 0xe0000)) {
			offset += 0x10000;
			size = 0x10000;
		}

		Bit8u checksum = 0;
		for (i = 0; i < (int)size; i++) {
			checksum += BX_MEM_THIS rom[offset + i];
		}

		// 当前阶段只先完成 ROM 装载，不处理中断式报错和 flash 持久化
		UNUSED(checksum);
	}
}


Bit8u* BX_MEM_C::getHostMemAddr(BX_CPU_C* cpu, bx_phy_address addr, unsigned rw)
{
	bx_phy_address a20addr = A20ADDR(addr);

	bool is_bios = (a20addr >= (bx_phy_address)BX_MEM_THIS bios_rom_addr);
#if BX_PHY_ADDRESS_LONG
	if (a20addr > BX_CONST64(0xffffffff)) is_bios = false;
#endif

	bool write = rw & 1;

	if ((cpu != NULL) && (rw == BX_EXECUTE)) {
		// reading from SMRAM memory space
		if ((a20addr >= 0x000a0000 && a20addr < 0x000c0000) && (BX_MEM_THIS smram_available))
		{
			//if (BX_MEM_THIS smram_enable || cpu->smm_mode())
			//	return BX_MEM_THIS get_vector(a20addr);
		}
	}

#if BX_SUPPORT_MONITOR_MWAIT
	if (write && BX_MEM_THIS is_monitor(a20addr & ~((bx_phy_address)(0xfff)), 0xfff)) {
		// Vetoed! Write monitored page !
		return(NULL);
	}
#endif
	/*
	while (memory_handler) {
    if (memory_handler->begin <= a20addr &&
        memory_handler->end >= a20addr) {
      if (memory_handler->da_handler)
        return memory_handler->da_handler(a20addr, rw, memory_handler->param);
      else
        return(NULL); // Vetoed! memory handler for i/o apic, vram, mmio and PCI PnP
    }
    memory_handler = memory_handler->next;
  }
	*/
	if (!write) {
		if ((a20addr >= 0x000a0000 && a20addr < 0x000c0000))
			return(NULL); // Vetoed!  Mem mapped IO (VGA)
#if BX_SUPPORT_PCI
		else if (BX_MEM_THIS pci_enabled && (a20addr >= 0x000c0000 && a20addr < 0x00100000)) {
			unsigned area = (unsigned)(a20addr >> 14) & 0x0f;
			if (area > BX_MEM_AREA_F0000) area = BX_MEM_AREA_F0000;
			if (BX_MEM_THIS memory_type[area][0] == false) {
				// Read from ROM
				if ((a20addr & 0xfffe0000) == 0x000e0000) {
					// last 128K of BIOS ROM mapped to 0xE0000-0xFFFFF
					return (Bit8u*)&BX_MEM_THIS rom[BIOS_MAP_LAST128K(a20addr)];
				}
				else {
					return (Bit8u*)&BX_MEM_THIS rom[(a20addr & EXROM_MASK) + BIOSROMSZ];
				}
			}
			else {
				// Read from ShadowRAM
				return BX_MEM_THIS get_vector(a20addr);
			}
		}
#endif
		else if ((a20addr < BX_MEM_THIS len) && !is_bios)
		{
			if (a20addr < 0x000c0000 || a20addr >= 0x00100000) {
				return BX_MEM_THIS get_vector(a20addr);
			}
			// must be in C0000 - FFFFF range
			else if ((a20addr & 0xfffe0000) == 0x000e0000) {
				// last 128K of BIOS ROM mapped to 0xE0000-0xFFFFF
				return (Bit8u*)&BX_MEM_THIS rom[BIOS_MAP_LAST128K(a20addr)];
			}
			else {
				return((Bit8u*)&BX_MEM_THIS rom[(a20addr & EXROM_MASK) + BIOSROMSZ]);
			}
		}

#if BX_PHY_ADDRESS_LONG
		else if (a20addr > BX_CONST64(0xffffffff)) {
			// Error, requested addr is out of bounds.
			return (Bit8u*)&BX_MEM_THIS bogus[a20addr & 0xfff];
		}
#endif
		else if (is_bios)
		{
			return (Bit8u*)&BX_MEM_THIS rom[a20addr & BIOS_MASK];
		}
		else
		{
			// Error, requested addr is out of bounds.
			return (Bit8u*)&BX_MEM_THIS bogus[a20addr & 0xfff];
		}
	}

	else
	{ // op == {BX_WRITE, BX_RW}
		if ((a20addr >= BX_MEM_THIS len) || is_bios)
			return(NULL); // Error, requested addr is out of bounds.
		else if (a20addr >= 0x000a0000 && a20addr < 0x000c0000)
			return(NULL); // Vetoed!  Mem mapped IO (VGA)
#if BX_SUPPORT_PCI
		else if (BX_MEM_THIS pci_enabled && (a20addr >= 0x000c0000 && a20addr < 0x00100000))
		{
			// Veto direct writes to this area. Otherwise, there is a chance
			// for Guest2HostTLB and memory consistency problems, for example
			// when some 16K block marked as write-only using PAM registers.
			return(NULL);
		}
#endif
		else
		{
			if (a20addr < 0x000c0000 || a20addr >= 0x00100000) {
				return BX_MEM_THIS get_vector(a20addr);
			}
			else {
				return(NULL);  // Vetoed!  ROMs
			}
		}
	}
}

void BX_MEM_C::disable_smram(void)
{
	BX_MEM_THIS smram_available = false;
	BX_MEM_THIS smram_enable = false;
	BX_MEM_THIS smram_restricted = false;
}

Bit8u BX_MEM_C::flash_read(Bit32u addr)
{  //909
	Bit8u ret = 0;

	switch (BX_MEM_THIS flash_wsm_state) {
	case FLASH_INT_ID:
		if (addr & 1) {
			ret = (BX_MEM_THIS flash_type == 2) ? 0x7c : 0x94;
		}
		else {
			ret = 0x89;
		}
		//BX_DEBUG(("flash read ID (address = 0x%08x value = 0x%02x)", addr, ret));
		break;
	case FLASH_READ_ARRAY:
		//BX_DEBUG(("flash read from ROM (address = 0x%08x)", addr));
		ret = BX_MEM_THIS rom[addr];
		break;
	default:
		ret = BX_MEM_THIS flash_status;
		if (BX_MEM_THIS flash_wsm_state == FLASH_ERASE) {
			BX_MEM_THIS flash_status |= 0x80;
		}
		//BX_DEBUG(("flash read status (address = 0x%08x value = 0x%02x)", addr, ret));
	}
	return ret;
}

void BX_MEM_C::flash_write(Bit32u addr, Bit8u data)
{  //936
	Bit32u flash_addr;
	int i;

	if (BX_MEM_THIS flash_type == 2) {
		flash_addr = addr & 0x3ffff;
	}
	else {
		flash_addr = addr & 0x1ffff;
	}
	if (BX_MEM_THIS flash_wsm_state == FLASH_PROG_SETUP) {
		//BX_DEBUG(("flash write to ROM (address = 0x%08x, data = 0x%02x)", flash_addr, data));
		BX_MEM_THIS rom[addr] &= data;
		BX_MEM_THIS flash_wsm_state = FLASH_READ_STATUS;
		BX_MEM_THIS flash_modified = true;
	}
	else {
		//BX_DEBUG(("flash write command (address = 0x%08x, code = 0x%02x)", flash_addr, data));
		switch (data) {
		case FLASH_INT_ID:
		case FLASH_READ_ARRAY:
		case FLASH_ERASE_SETUP:
		case FLASH_ERASE_SUSP:
		case FLASH_PROG_SETUP:
			BX_MEM_THIS flash_wsm_state = data;
			break;
		case FLASH_READ_STATUS:
			if (BX_MEM_THIS flash_wsm_state != FLASH_ERASE) {
				BX_MEM_THIS flash_wsm_state = data;
			}
			break;
		case FLASH_CLR_STATUS:
			BX_MEM_THIS flash_status &= ~0x38;
			BX_MEM_THIS flash_wsm_state = FLASH_READ_ARRAY;
			break;
		case FLASH_ERASE:
			if (BX_MEM_THIS flash_wsm_state == FLASH_ERASE_SETUP) {
				BX_MEM_THIS flash_status &= ~0xc0;
				BX_MEM_THIS flash_wsm_state = FLASH_ERASE;
				if ((BX_MEM_THIS flash_type == 1) &&
					((flash_addr == 0x1c000) || (flash_addr == 0x1d000))) {
					for (i = 0; i < 0x1000; i++) {
						BX_MEM_THIS rom[addr + i] = 0xff;
					}
					BX_MEM_THIS flash_modified = true;
				}
				else if ((BX_MEM_THIS flash_type == 2) &&
					((flash_addr == 0x38000) || (flash_addr == 0x3a000))) {
					for (i = 0; i < 0x2000; i++) {
						BX_MEM_THIS rom[addr + i] = 0xff;
					}
					BX_MEM_THIS flash_modified = true;
				}
			}
			else if (BX_MEM_THIS flash_wsm_state == FLASH_ERASE_SUSP) {
				BX_MEM_THIS flash_status &= ~0x40;
				BX_MEM_THIS flash_wsm_state = FLASH_ERASE;
			}
			else {
				//BX_DEBUG(("flash_write(): unexpected ERASE CONFIRM / ERASE RESUME"));
			}
			break;
		default:
			//BX_DEBUG(("flash_write(): unsupported code 0x%02x", data));
			break;
		}
	}
}