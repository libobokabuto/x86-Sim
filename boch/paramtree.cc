#define  _CRT_SECURE_NO_WARNINGS
#include "bochs.h"
#include "siminterface.h"
#include "paramtree.h"

extern bx_simulator_interface_c* SIM;
//extern logfunctions* siminterface_log;
extern bx_list_c* root_param;

#define LOG_THIS siminterface_log->


const char* bx_param_c::default_text_format = NULL;

static Bit32u bx_local_gen_param_id()
{ //自己加的
    static Bit32u id = BXP_NEW_PARAM_ID;
    return id++;
}

bx_param_c::bx_param_c(Bit32u id, const char* param_name, const char* param_desc)
    : bx_object_c(id),
    parent(NULL),
    description(NULL),
    label(NULL),
    text_format(NULL),
    long_text_format(NULL),
    ask_format(NULL),
    group_name(NULL)
{
    set_type(BXT_PARAM);
    this->name = new char[strlen(param_name) + 1];
    strcpy(this->name, param_name);
    set_description(param_desc);
    set_format(default_text_format);
    set_long_format(default_text_format);
    this->runtime_param = 0;
    this->enabled = 1;
    this->options = 0;
    // dependent_list must be initialized before the set(),
    // because set calls update_dependents().
    dependent_list = NULL;
}

bx_param_c::~bx_param_c()
{
    delete[] name;
    delete[] label;
    delete[] description;
    delete[] text_format;
    delete[] long_text_format;
    delete[] ask_format;
    delete[] group_name;
    delete dependent_list;
}

void bx_param_c::set_description(const char* text)
{
    delete[] this->description;
    if (text) {
        this->description = new char[strlen(text) + 1];
        strcpy(this->description, text);
    }
    else {
        this->description = NULL;
    }
}

void bx_param_c::set_label(const char* text)
{
    delete[] label;
    if (text) {
        label = new char[strlen(text) + 1];
        strcpy(label, text);
    }
    else {
        label = NULL;
    }
}

void bx_param_c::set_format(const char* format)
{
    delete[] text_format;
    if (format) {
        text_format = new char[strlen(format) + 1];
        strcpy(text_format, format);
    }
    else {
        text_format = NULL;
    }
}

void bx_param_c::set_long_format(const char* format)
{
    delete[] long_text_format;
    if (format) {
        long_text_format = new char[strlen(format) + 1];
        strcpy(long_text_format, format);
    }
    else {
        long_text_format = NULL;
    }
}

void bx_param_c::set_ask_format(const char* format)
{
    delete[] ask_format;
    if (format) {
        ask_format = new char[strlen(format) + 1];
        strcpy(ask_format, format);
    }
    else {
        ask_format = NULL;
    }
}

void bx_param_c::set_group(const char* group)
{
    delete[] group_name;
    if (group) {
        group_name = new char[strlen(group) + 1];
        strcpy(group_name, group);
    }
    else {
        group_name = NULL;
    }
}

int bx_param_c::get_param_path(char* path_out, int maxlen)
{
    if ((get_parent() == NULL) || (get_parent() == root_param)) {
        // Start with an empty string.
        // Never print the name of the root param.
        path_out[0] = 0;
    }
    else {
        // build path of the parent, add a period, add path of this node
        if (get_parent()->get_param_path(path_out, maxlen) > 0) {
            strncat(path_out, ".", maxlen);
        }
    }
    strncat(path_out, name, maxlen);
    return strlen(path_out);
}

const char* bx_param_c::set_default_format(const char* f)
{
    const char* old = default_text_format;
    default_text_format = f;
    return old;
}


bool bx_param_string_c::isempty() const
{//946
	return (strlen(val) == 0) || !strcmp(val, "none");
}

bx_shadow_data_c::bx_shadow_data_c(bx_param_c* parent,
    const char* name,
    Bit8u* ptr_to_data,
    Bit32u data_size,
    bool is_text)
    : bx_param_c(bx_local_gen_param_id(), name, "")
{//1080
    set_type(BXT_PARAM_DATA);
    this->data_ptr = ptr_to_data;
    this->data_size = data_size;
    this->is_text = is_text;
    if (parent) {
        //BX_ASSERT(parent->get_type() == BXT_LIST);
        this->parent = (bx_list_c*)parent;
        this->parent->add(this);
    }
}

void bx_shadow_filedata_c::set_sr_handlers(void* devptr, filedata_save_handler save, filedata_restore_handler restore)
{
	//1130
	this->sr_devptr = devptr;
	this->save_handler = save;
	this->restore_handler = restore;
}

void bx_list_c::add(bx_param_c* param)
{//1260
    if ((get_by_name(param->get_name()) != NULL) && (param->get_parent() == this)) {
        //BX_PANIC(("parameter '%s' already exists in list '%s'", param->get_name(), this->get_name()));
        return;
    }
    bx_listitem_t* item = new bx_listitem_t;
    item->param = param;
    item->next = NULL;
    if (list == NULL) {
        list = item;
    }
    else {
        bx_listitem_t* temp = list;
        while (temp->next)
            temp = temp->next;
        temp->next = item;
    }
    if (runtime_param) {
        param->set_runtime_param(1);
    }
    size++;
}

bx_param_c* bx_list_c::get_by_name(const char* name)
{ //1298
    bx_listitem_t* temp = list;
    while (temp != NULL) {
        bx_param_c* p = temp->param;
        if (!_stricmp(name, p->get_name())) {
            return p;
        }
        temp = temp->next;
    }
    return NULL;
}