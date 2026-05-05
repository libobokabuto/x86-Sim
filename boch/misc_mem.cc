#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
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
	//296
}