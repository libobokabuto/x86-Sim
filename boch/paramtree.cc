#include "bochs.h"
#include "siminterface.h"
#include "paramtree.h"

bool bx_param_string_c::isempty() const
{//946
	return (strlen(val) == 0) || !strcmp(val, "none");
}

void bx_shadow_filedata_c::set_sr_handlers(void* devptr, filedata_save_handler save, filedata_restore_handler restore)
{
	//1130
	this->sr_devptr = devptr;
	this->save_handler = save;
	this->restore_handler = restore;
}

