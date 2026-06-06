#include "param_names.h"
#include "iodev.h"
#include "debug.h"
#include "virt_timer.h"

bx_simulator_interface_c* SIM = NULL;
bx_list_c* root_param = NULL;
#define LOG_THIS siminterface_log->

static int rt_conf_id = 0;

typedef struct _rt_conf_entry_t {
	int id;
	void* device;
	rt_conf_handler_t handler;
	struct _rt_conf_entry_t* next;
} rt_conf_entry_t;

typedef struct _addon_option_t {
	const char* name;
	addon_option_parser_t parser;
	addon_option_save_t savefn;
	struct _addon_option_t* next;
} addon_option_t;

class bx_real_sim_c : public bx_simulator_interface_c {
	bxevent_handler bxevent_callback;
	void* bxevent_callback_data;
	const char* registered_ci_name;
	config_interface_callback_t ci_callback;
	void* ci_callback_data;
	rt_conf_entry_t* rt_conf_entries;
	addon_option_t* addon_options;
	bool init_done;
	bool ci_started;
	bool enabled;
	// save context to jump to if we must quit unexpectedly
	jmp_buf* quit_context;
	int exit_code;
	unsigned param_id;
	bool bx_debug_gui;
	bool bx_log_viewer;
	bool wxsel;
public:
};

const char* floppy_devtype_names[] = { "none", "5.25\" 360K", "5.25\" 1.2M", "3.5\" 720K", "3.5\" 1.44M", "3.5\" 2.88M", NULL };
const char* floppy_type_names[] = { "none", "1.2M", "1.44M", "2.88M", "720K", "360K", "160K", "180K", "320K", "auto", NULL };
int floppy_type_n_sectors[] = { -1, 80 * 2 * 15, 80 * 2 * 18, 80 * 2 * 36, 80 * 2 * 9, 40 * 2 * 9, 40 * 1 * 8, 40 * 1 * 9, 40 * 2 * 8, -1 };
const char* media_status_names[] = { "ejected", "inserted", NULL };

const char* bochs_bootdisk_names[] = { "none", "floppy", "disk", "cdrom", "", "usb", "network", NULL };



