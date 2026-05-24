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
typedef Bit64s(*param_event_handler)(class bx_param_c*, bool set, Bit64s val);
typedef Bit64s(*param_save_handler)(void* devptr, class bx_param_c*);
typedef void (*param_restore_handler)(void* devptr, class bx_param_c*, Bit64s val);
typedef bool (*param_enable_handler)(class bx_param_c*, bool en);
class BOCHSAPI bx_param_num_c : public bx_param_c {
protected:
    Bit64s min, max, initial_val;
    union _uval_ {
        Bit64s number;   // used by bx_param_num_c
        Bit64s* p64bit;  // used by bx_shadow_num_c
        Bit32s* p32bit;  // used by bx_shadow_num_c
        Bit16s* p16bit;  // used by bx_shadow_num_c
        Bit8s* p8bit;   // used by bx_shadow_num_c
        float* pfloat;  // used by bx_shadow_num_c
        double* pdouble; // used by bx_shadow_num_c
        bool* pbool;   // used by bx_shadow_bool_c
    } val;
    param_event_handler handler;
    void* sr_devptr;
    param_save_handler save_handler;
    param_restore_handler restore_handler;
    param_enable_handler enable_handler;
    int base;
    bool is_shadow;

public:
    enum {
        // When a bx_param_num_c is displayed in dialog, USE_SPIN_CONTROL controls
        // whether a spin control should be used instead of a simple text control.
        USE_SPIN_CONTROL = (1 << 0)
    } bx_numopt_bits;

};
class BOCHSAPI bx_param_bool_c : public bx_param_num_c {

public:
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

typedef struct _bx_listitem_t {
	bx_param_c* param;
	struct _bx_listitem_t* next;
} bx_listitem_t;

typedef void (*list_restore_handler)(void* devptr, class bx_list_c*);

class BOCHSAPI bx_list_c : public bx_param_c {
protected:
    // chained list of bx_listitem_t
    bx_listitem_t* list;
    int size;
    // for a menu, the value of choice before the call to "ask" is default.
    // After ask, choice holds the value that the user chose. Choice defaults
    // to 1 in the constructor.
    Bit32u choice; // type Bit32u is compatible with ask_uint
    // title of the menu or series
    char* title;
    void init(const char* list_title);
    // save / restore support
    void* sr_devptr;
    list_restore_handler restore_handler;
public:
    enum {
        // When a bx_list_c is displayed as a menu, SHOW_PARENT controls whether or
        // not the menu shows a "Return to parent menu" choice or not.
        SHOW_PARENT = (1 << 0),
        // Some lists are best displayed shown as menus, others as a series of
        // related questions.  This bit suggests to the CI that the series of
        // questions format is preferred.
        SERIES_ASK = (1 << 1),
        // When a bx_list_c is displayed in a dialog, USE_TAB_WINDOW suggests
        // to the CI that each item in the list should be shown as a separate
        // tab.  This would be most appropriate when each item is another list
        // of parameters.
        USE_TAB_WINDOW = (1 << 2),
        // When a bx_list_c is displayed in a dialog, the list name is used as the
        // label of the group box if USE_BOX_TITLE is set. This is only necessary if
        // more than one list appears in a dialog box.
        USE_BOX_TITLE = (1 << 3),
        // When a bx_list_c is displayed as a menu, SHOW_GROUP_NAME controls whether
        // or not the name of group the item belongs to is added to the name of the
        // item (used in the runtime menu).
        SHOW_GROUP_NAME = (1 << 4),
        // When a bx_list_c is displayed in a dialog, USE_SCROLL_WINDOW suggests
        // to the CI that the list items should be displayed in a scrollable dialog
        // window. Large lists can make the dialog unusable and using this flag
        // can force the CI to limit the dialog height with all items accessible.
        USE_SCROLL_WINDOW = (1 << 5)
    } bx_listopt_bits;

};

