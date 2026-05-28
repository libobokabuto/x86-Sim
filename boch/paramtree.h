#pragma once

typedef enum {
    BXT_OBJECT = 201,
    BXT_PARAM,
    BXT_PARAM_NUM,
    BXT_PARAM_BOOL,
    BXT_PARAM_ENUM,
    BXT_PARAM_STRING,
    BXT_PARAM_BYTESTRING,
    BXT_PARAM_DATA,
    BXT_PARAM_FILEDATA,
    BXT_LIST
} bx_objtype;

class bx_object_c;
class bx_param_c;
class bx_param_string_c;
class bx_param_filename_c;
class BOCHSAPI bx_object_c {
	//78
private:
    Bit32u id;
    bx_objtype type;
protected:
    void set_type(bx_objtype _type) { type = _type; }
public:
    bx_object_c(Bit32u _id) : id(_id), type(BXT_OBJECT) {}
    virtual ~bx_object_c() {}
    Bit32u get_id() const { return id; }
    Bit8u get_type() const { return type; }
};

#define BASE_DEC 10
#define BASE_HEX 16
#define BASE_FLOAT 32
#define BASE_DOUBLE 64

class BOCHSAPI bx_param_c : public bx_object_c {
	//96
	BOCHSAPI_CYGONLY static const char* default_text_format;
protected:
    bx_list_c* parent;
    char* name;
    char* description;
    char* label;            // label string for text menus and gui dialogs
    char* text_format;      // printf format string. %d for ints, %s for strings, etc.
    char* long_text_format; // printf format string. %d for ints, %s for strings, etc.
    char* ask_format;       // format string for asking for a new value
    char* group_name;       // name of the group the param belongs to
    bool runtime_param;
    bool enabled;
    Bit32u options;
    // The dependent_list is initialized to NULL.  If dependent_list is modified
    // to point to a bx_list_c of other parameters, the set() method of the
    // parameter type will enable those parameters when the enable condition is
    // true, and disable them it is false.
    bx_list_c* dependent_list;
    void* device;
public:
    enum {
        // If set, this parameter is available in CI only. In bochsrc, it is set
        // indirectly from one or more other options (e.g. cpu count)
        CI_ONLY = (1 << 31)
    } bx_param_opt_bits;

    bx_param_c(Bit32u id, const char* name, const char* description);
    virtual ~bx_param_c();

    virtual void reset() {}

    const char* get_name() const { return name; }
    bx_param_c* get_parent() { return (bx_param_c*)parent; }

    int get_param_path(char* path_out, int maxlen);

    void set_format(const char* format);
    const char* get_format() const { return text_format; }

    void set_long_format(const char* format);
    const char* get_long_format() const { return long_text_format; }

    void set_ask_format(const char* format);
    const char* get_ask_format() const { return ask_format; }

    void set_label(const char* text);
    const char* get_label() const { return label; }

    void set_description(const char* text);
    const char* get_description() const { return description; }

    virtual void set_runtime_param(bool val) { runtime_param = val; }
    bool get_runtime_param() const { return runtime_param; }

    void set_group(const char* group);
    const char* get_group() const { return group_name; }

    bool get_enabled() const { return enabled; }
    virtual void set_enabled(bool _enabled) { enabled = _enabled; }

    static const char* set_default_format(const char* f);
    static const char* get_default_format() { return default_text_format; }

    bx_list_c* get_dependent_list() { return dependent_list; }

    void set_options(Bit32u _options) { options = _options; }
    Bit32u get_options() const { return options; }

    void set_device_param(void* dev) { device = dev; }
    void* get_device_param() { return device; }

    virtual int parse_param(const char* value) { return -1; }

    virtual void dump_param(FILE* fp) {}
    virtual int dump_param(char* buf, int buflen, bool dquotes = false) { return 0; }

};


typedef Bit64s(*param_event_handler)(class bx_param_c*, bool set, Bit64s val);
typedef Bit64s(*param_save_handler)(void* devptr, class bx_param_c*);
typedef void (*param_restore_handler)(void* devptr, class bx_param_c*, Bit64s val);
typedef bool (*param_enable_handler)(class bx_param_c*, bool en);

class BOCHSAPI bx_param_num_c : public bx_param_c {
    BOCHSAPI_CYGONLY static Bit32u default_base;
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

class BOCHSAPI bx_shadow_num_c : public bx_param_num_c {
    Bit8u varsize;   // must be 64, 32, 16, or 8
    Bit8u lowbit;   // range of bits associated with this param
    Bit64u mask;     // mask is ANDed with value before it is returned from get
public:
};

class BOCHSAPI bx_param_bool_c : public bx_param_num_c {

public:
};

class BOCHSAPI bx_shadow_bool_c : public bx_param_bool_c {
public:
};

class BOCHSAPI bx_param_enum_c : public bx_param_num_c {
    const char** choices;
    Bit64u* deps_bitmap;
};

typedef const char* (*param_string_event_handler)(class bx_param_string_c*,
    bool set, const char* oldval, const char* newval, int maxlen);

class BOCHSAPI bx_param_string_c : public bx_param_c {
protected:
	int maxsize;
	char* val, * initial_val;
    param_string_event_handler handler;
    param_enable_handler enable_handler;
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

class BOCHSAPI bx_param_bytestring_c : public bx_param_string_c {
    char separator;
};

class BOCHSAPI bx_param_filename_c : public bx_param_string_c {
    const char* ext;
};

class BOCHSAPI bx_shadow_data_c : public bx_param_c {
    Bit32u data_size;
    Bit8u* data_ptr;
    bool is_text;
public:
    bx_shadow_data_c(bx_param_c* parent,
        const char* name,
        Bit8u* ptr_to_data,
        Bit32u data_size, bool is_text = 0);
    Bit8u* getptr() { return data_ptr; }
    const Bit8u* getptr() const { return data_ptr; }
    Bit32u get_size() const { return data_size; }
    bool is_text_format() const { return is_text; }
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

    void add(bx_param_c* param);
    bx_param_c* get_by_name(const char* name);

};

