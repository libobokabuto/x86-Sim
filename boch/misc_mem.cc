#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
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
}

void BX_MEM_C::cleanup_memory()
{
	//212
}
void BX_MEM_C::load_ROM(const char* path, bx_phy_address romaddress, Bit8u type)
{
	//296
}