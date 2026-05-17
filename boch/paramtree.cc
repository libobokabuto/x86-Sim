#include "bochs.h"
#include "siminterface.h"
#include "paramtree.h"

void bx_shadow_filedata_c::set_sr_handlers(void* devptr, filedata_save_handler save, filedata_restore_handler restore)
{
	//1130
	this->sr_devptr = devptr;
	this->save_handler = save;
	this->restore_handler = restore;
}


