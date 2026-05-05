#pragma once

class bx_object_c;
class bx_param_c;
class bx_param_filename_c;
class BOCHSAPI bx_object_c {
	//78
private:
};

class BOCHSAPI bx_param_c : public bx_object_c {
	//96
	BOCHSAPI_CYGONLY static const char* default_text_format;
protected:
};

typedef void (*filedata_save_handler)(void* devptr, FILE* save_fp);//467
typedef void (*filedata_restore_handler)(void* devptr, FILE* save_fp);//468
class BOCHSAPI bx_shadow_filedata_c : public bx_param_c {
	//470
protected:
	void* sr_devptr;
	filedata_save_handler    save_handler;
	filedata_restore_handler restore_handler;
public:
	bx_shadow_filedata_c(bx_param_c* parent,
		const char* name, FILE** scratch_file_ptr_ptr);
	void set_sr_handlers(void* devptr, filedata_save_handler save, filedata_restore_handler restore);
};