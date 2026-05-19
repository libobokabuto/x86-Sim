#pragma once

class bx_object_c;
class bx_param_c;
class bx_param_string_c;
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

class BOCHSAPI bx_param_string_c : public bx_param_c {
protected:
	int maxsize;
	char* val, * initial_val;
public:
	enum {
		IS_FILENAME = 1,       // 1=yes it's a filename, 0=not a filename.
		// Some guis have a file browser. This
		// bit suggests that they use it.
		SAVE_FILE_DIALOG = 2,  // Use save dialog opposed to open file dialog
		SELECT_FOLDER_DLG = 4  // Use folder selection dialog
	} bx_string_opt_bits;
	char* getptr() { return val; }//396
	const char* getptr() const { return val; }//397
	bool isempty() const;//402

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

