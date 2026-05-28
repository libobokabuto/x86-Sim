#pragma once
#include "paramtree.h"

#define BXP_NEW_PARAM_ID 1001

typedef enum {
	BX_TOOLBAR_UNDEFINED,
	BX_TOOLBAR_FLOPPYA,
	BX_TOOLBAR_FLOPPYB,
	BX_TOOLBAR_CDROM1,
	BX_TOOLBAR_RESET,
	BX_TOOLBAR_POWER,
	BX_TOOLBAR_SAVE_RESTORE,
	BX_TOOLBAR_COPY,
	BX_TOOLBAR_PASTE,
	BX_TOOLBAR_SNAPSHOT,
	BX_TOOLBAR_CONFIG,
	BX_TOOLBAR_MOUSE_EN,
	BX_TOOLBAR_USER,
	BX_TOOLBAR_USB_DEBUG
} bx_toolbar_buttons;

//#define BX_LOG_OPTS_EXCLUDE(type, choice)  (             \
   /* can't die, ask or warn, on debug or info events */ \
   (type <= LOGLEV_INFO && (choice >= ACT_WARN))         \
   /* can't ignore panics */                             \
   || (type == LOGLEV_PANIC && choice == ACT_IGNORE)     \
   )

// floppy / cdrom media status
enum { BX_EJECTED = 0, BX_INSERTED = 1 };

enum {
	BX_BOOT_NONE,
	BX_BOOT_FLOPPYA,
	BX_BOOT_DISKC,
	BX_BOOT_CDROM,
	BX_BOOT_PCMCIA,
	BX_BOOT_USB,
	BX_BOOT_NETWORK
};

#define BX_EVT_IS_ASYNC(type) ((type) > __ALL_EVENTS_BELOW_ARE_ASYNC__)

typedef enum {
	__ALL_EVENTS_BELOW_ARE_SYNCHRONOUS__ = 2000,
	BX_SYNC_EVT_GET_PARAM,          // CI -> simulator -> CI
	BX_SYNC_EVT_ASK_PARAM,          // simulator -> CI -> simulator
	BX_SYNC_EVT_TICK, // simulator -> CI, wait for response.
	BX_SYNC_EVT_LOG_DLG,            // simulator -> CI, wait for response.
	BX_SYNC_EVT_GET_DBG_COMMAND,    // simulator -> CI, wait for response.
	BX_SYNC_EVT_MSG_BOX,            // simulator -> CI, wait for response.
	BX_SYNC_EVT_ML_MSG_BOX,         // simulator -> CI, do not wait for response.
	BX_SYNC_EVT_ML_MSG_BOX_KILL,    // simulator -> CI, do not wait for response.
	__ALL_EVENTS_BELOW_ARE_ASYNC__,
	BX_ASYNC_EVT_KEY,               // vga window -> simulator
	BX_ASYNC_EVT_MOUSE,             // vga window -> simulator
	BX_ASYNC_EVT_SET_PARAM,         // CI -> simulator
	BX_ASYNC_EVT_LOG_MSG,           // simulator -> CI
	BX_ASYNC_EVT_DBG_MSG,           // simulator -> CI
	BX_ASYNC_EVT_VALUE_CHANGED,     // simulator -> CI
	BX_ASYNC_EVT_TOOLBAR,           // CI -> simulator
	BX_ASYNC_EVT_STATUSBAR,         // simulator -> CI
	BX_ASYNC_EVT_REFRESH,           // simulator -> CI
	BX_ASYNC_EVT_QUIT_SIM           // simulator -> CI
} BxEventType;

typedef union {
	Bit32s s32;
	char* charptr;
} AnyParamVal;

typedef struct {
	// what was pressed?  This is a BX_KEY_* value.  For key releases,
	// BX_KEY_RELEASED is ORed with the base BX_KEY_*.
	Bit32u bx_key;
	bool raw_scancode;
} BxKeyEvent;

typedef struct {
	// type is BX_EVT_MOUSE
	Bit16s dx, dy, dz;       // mouse motion delta
	Bit8u buttons;           // which buttons are pressed.
	// bit 0: 1=left button down, 0=up
	// bit 1: 1=right button down, 0=up
	// bit 2: 1=middle button down, 0=up
} BxMouseEvent;

typedef struct {
	// type is BX_EVT_GET_PARAM, BX_EVT_SET_PARAM
	class bx_param_c* param;         // pointer to param structure
	AnyParamVal val;
} BxParamEvent;

typedef struct {
	Bit8u level;
	Bit8u mode;
	const char* prefix;
	const char* msg;
} BxLogMsgEvent;

enum {
	BX_LOG_ASK_CHOICE_CONTINUE,
	BX_LOG_ASK_CHOICE_CONTINUE_ALWAYS,
	BX_LOG_ASK_CHOICE_DIE,
	BX_LOG_ASK_CHOICE_DUMP_CORE,
	BX_LOG_ASK_CHOICE_ENTER_DEBUG,
	BX_LOG_ASK_N_CHOICES,
	BX_LOG_NOTIFY_FAILED
};

enum {
	BX_LOG_DLG_ASK,
	BX_LOG_DLG_WARN,
	BX_LOG_DLG_QUIT
};

typedef struct {
	char* command;   // null terminated string. allocated by debugger interface
	// with new operator, freed by simulator with delete.
} BxDebugCommand;

typedef struct {
	bx_toolbar_buttons button;
	bool on; // for toggling buttons, on=true means the toolbar button is
	// pressed. on=false means it is not pressed.
} BxToolbarEvent;

// Event type: BX_ASYNC_EVT_STATUSAR
typedef struct {
	int element;
	char* text;
	bool active;
	bool w;
} BxStatusbarEvent;

typedef struct {
	BxEventType type; // what kind is this?
	Bit32s retcode;   // success or failure. only used for synchronous events.
	void* param0;     // misc parameter
	union {
		BxKeyEvent key;
		BxMouseEvent mouse;
		BxParamEvent param;
		BxLogMsgEvent logmsg;
		BxToolbarEvent toolbar;
		BxStatusbarEvent statbar;
		BxDebugCommand debugcmd;
	} u;
} BxEvent;

enum {
	// Just start the simulation without running the configuration interface
	// at all, unless something goes wrong.
	BX_QUICK_START = 200,
	// Run the configuration interface.  The default action will be to load a
	// configuration file.  This makes sense if a config file could not be
	// loaded, either because it wasn't found or because it had errors.
	BX_LOAD_START,
	// Run the configuration interface.  The default action will be to
	// edit the configuration.
	BX_EDIT_START,
	// Run the configuration interface, but make the default action be to
	// start the simulation.
	BX_RUN_START
};

enum {
	BX_VGA_EXTENSION_NONE,
	BX_VGA_EXTENSION_VBE
};

enum {
	BX_DDC_MODE_DISABLED,
	BX_DDC_MODE_BUILTIN,
	BX_DDC_MODE_FILE
};

enum {
	BX_VBE_MEMSIZE_4MB,
	BX_VBE_MEMSIZE_8MB,
	BX_VBE_MEMSIZE_16MB,
	BX_VBE_MEMSIZE_32MB
};

enum {  //462
	BX_MOUSE_TYPE_NONE,
	BX_MOUSE_TYPE_PS2,
	BX_MOUSE_TYPE_IMPS2,
#if BX_SUPPORT_BUSMOUSE
	BX_MOUSE_TYPE_INPORT,
	BX_MOUSE_TYPE_BUS,
#endif
	BX_MOUSE_TYPE_SERIAL,
	BX_MOUSE_TYPE_SERIAL_WHEEL,
	BX_MOUSE_TYPE_SERIAL_MSYS
};

enum {
	BX_MOUSE_TOGGLE_CTRL_MB,
	BX_MOUSE_TOGGLE_CTRL_F10,
	BX_MOUSE_TOGGLE_CTRL_ALT,
	BX_MOUSE_TOGGLE_CTRL_ALT_G,
	BX_MOUSE_TOGGLE_F12
};

#define BX_FDD_NONE  0 // floppy not present
#define BX_FDD_525DD 1 // 360K  5.25"
#define BX_FDD_525HD 2 // 1.2M  5.25"
#define BX_FDD_350DD 3 // 720K  3.5"
#define BX_FDD_350HD 4 // 1.44M 3.5"
#define BX_FDD_350ED 5 // 2.88M 3.5"

#define BX_FLOPPY_NONE   10 // media not present
#define BX_FLOPPY_1_2    11 // 1.2M  5.25"
#define BX_FLOPPY_1_44   12 // 1.44M 3.5"
#define BX_FLOPPY_2_88   13 // 2.88M 3.5"
#define BX_FLOPPY_720K   14 // 720K  3.5"
#define BX_FLOPPY_360K   15 // 360K  5.25"
#define BX_FLOPPY_160K   16 // 160K  5.25"
#define BX_FLOPPY_180K   17 // 180K  5.25"
#define BX_FLOPPY_320K   18 // 320K  5.25"
#define BX_FLOPPY_LAST   18 // last legal value of floppy type

#define BX_FLOPPY_AUTO     19 // autodetect image size
#define BX_FLOPPY_UNKNOWN  20 // image size doesn't match one of the types above

#define BX_ATA_DEVICE_NONE       0
#define BX_ATA_DEVICE_DISK       1
#define BX_ATA_DEVICE_CDROM      2

#define BX_ATA_BIOSDETECT_AUTO   0
#define BX_ATA_BIOSDETECT_CMOS   1
#define BX_ATA_BIOSDETECT_NONE   2

enum {
	BX_SECT_SIZE_512,
	BX_SECT_SIZE_1024,
	BX_SECT_SIZE_4096
};

enum {
	BX_ATA_TRANSLATION_NONE,
	BX_ATA_TRANSLATION_LBA,
	BX_ATA_TRANSLATION_LARGE,
	BX_ATA_TRANSLATION_RECHS,
	BX_ATA_TRANSLATION_AUTO
};
#define BX_ATA_TRANSLATION_LAST  BX_ATA_TRANSLATION_AUTO

enum {
	BX_CLOCK_SYNC_NONE,
	BX_CLOCK_SYNC_REALTIME,
	BX_CLOCK_SYNC_SLOWDOWN,
	BX_CLOCK_SYNC_BOTH
};
#define BX_CLOCK_SYNC_LAST       BX_CLOCK_SYNC_BOTH

enum {
	BX_PCI_CHIPSET_I430FX,
	BX_PCI_CHIPSET_I440FX,
	BX_PCI_CHIPSET_I440BX
};

#define BX_CLOCK_TIME0_LOCAL     1
#define BX_CLOCK_TIME0_UTC       2

BOCHSAPI extern const char* floppy_devtype_names[];
BOCHSAPI extern const char* floppy_type_names[];
BOCHSAPI extern int floppy_type_n_sectors[];
BOCHSAPI extern const char* media_status_names[];
BOCHSAPI extern const char* bochs_bootdisk_names[];

#if BX_USB_DEBUGGER

enum {
	USB_DEBUG_NONE,
	USB_DEBUG_UHCI,
	USB_DEBUG_OHCI,
	USB_DEBUG_EHCI,
	USB_DEBUG_XHCI
};

// USB debug break_type
#define USB_DEBUG_FRAME    1
#define USB_DEBUG_COMMAND  2
#define USB_DEBUG_EVENT    3
#define USB_DEBUG_NONEXIST 4
#define USB_DEBUG_RESET    5
#define USB_DEBUG_ENABLE   6
#define USB_DEBUG_DATA     7

#define BX_USB_DEBUG_SOF_NONE      0
#define BX_USB_DEBUG_SOF_SET       1
#define BX_USB_DEBUG_SOF_TRIGGER   2

// lParam flags
#define USB_LPARAM_FLAG_BEFORE  0x00000001
#define USB_LPARAM_FLAG_AFTER   0x00000002

#endif

#include <setjmp.h>

enum ci_command_t { CI_START, CI_RUNTIME_CONFIG, CI_SHUTDOWN };
enum ci_return_t {
	CI_OK,                  // normal return value
	CI_ERR_NO_TEXT_CONSOLE  // err: can't work because there's no text console
};
typedef int (*config_interface_callback_t)(void* userdata, ci_command_t command);
typedef BxEvent* (*bxevent_handler)(void* theclass, BxEvent* event);
typedef void (*rt_conf_handler_t)(void* this_ptr);
typedef Bit32s(*addon_option_parser_t)(const char* context, int num_params, char* params[]);
typedef Bit32s(*addon_option_save_t)(FILE* fp);



enum disp_mode_t { DISP_MODE_CONFIG = 100, DISP_MODE_SIM }; //606

class BOCHSAPI bx_simulator_interface_c {
public:
	bx_simulator_interface_c() {}
	virtual ~bx_simulator_interface_c() {}
	virtual void set_quit_context(jmp_buf* context) {}
	virtual bool get_init_done() { return 0; }
	virtual int set_init_done(bool n) { return 0; }
	virtual bool get_ci_started() { return 0; }
	virtual void reset_all_param() {}
	// new param methods
	virtual bx_param_c* get_param(const char* pname, bx_param_c* base = NULL) { return NULL; }
	virtual bx_param_num_c* get_param_num(const char* pname, bx_param_c* base = NULL) { return NULL; }
	virtual bx_param_string_c* get_param_string(const char* pname, bx_param_c* base = NULL) { return NULL; }
	virtual bx_param_bool_c* get_param_bool(const char* pname, bx_param_c* base = NULL) { return NULL; }
	virtual bx_param_enum_c* get_param_enum(const char* pname, bx_param_c* base = NULL) { return NULL; }
	virtual unsigned gen_param_id() { return 0; }
	virtual int get_n_log_modules() { return -1; }
	virtual const char* get_logfn_name(int mod) { return NULL; }
	virtual int get_logfn_id(const char* name) { return -1; }
	virtual const char* get_prefix(int mod) { return NULL; }
	virtual int get_log_action(int mod, int level) { return -1; }
	virtual void set_log_action(int mod, int level, int action) {}
	virtual int get_default_log_action(int level) { return -1; }
	virtual void set_default_log_action(int level, int action) {}
	virtual const char* get_action_name(int action) { return NULL; }
	virtual int is_action_name(const char* val) { return -1; }
	virtual const char* get_log_level_name(int level) { return NULL; }
	virtual int get_max_log_level() { return -1; }

	// exiting is somewhat complicated!  The preferred way to exit bochs is
	// to call BX_EXIT(exitcode).  That is defined to call
	// SIM->quit_sim(exitcode).  The quit_sim function first calls
	// the cleanup functions in bochs so that it can destroy windows
	// and free up memory, then sends a notify message to the CI
	// telling it that bochs has stopped.
	virtual void quit_sim(int code) {}

	virtual int get_exit_code() { return 0; }

	virtual int get_default_rc(char* path, int len) { return -1; }
	virtual int read_rc(const char* path) { return -1; }
	virtual int write_rc(const char* rc, int overwrite) { return -1; }
	virtual int get_log_file(char* path, int len) { return -1; }
	virtual int set_log_file(const char* path) { return -1; }
	virtual int get_log_prefix(char* prefix, int len) { return -1; }
	virtual int set_log_prefix(const char* prefix) { return -1; }
	virtual int get_debugger_log_file(char* path, int len) { return -1; }
	virtual int set_debugger_log_file(const char* path) { return -1; }

	virtual void set_notify_callback(bxevent_handler func, void* arg) {}
	virtual void get_notify_callback(bxevent_handler* func, void** arg) {}

	// send an event from the simulator to the CI.
	virtual BxEvent* sim_to_ci_event(BxEvent* event) { return NULL; }

	// called from simulator to display a gui dialog in particular situations.
	// 1. When it hits serious errors, to ask if the user wants to continue or not.
	// 2. When it hits errors, to warn the user before continuing simulation
	// 3. When it hits critical errors, inform the user before terminating simulation.
	virtual int log_dlg(const char* prefix, int level, const char* msg, int mode) { return -1; }
	// called from simulator when writing a message to log file
	virtual void log_msg(const char* prefix, int level, const char* msg) {}
	// set this to 1 if the gui has a log viewer
	virtual void set_log_viewer(bool val) {}
	virtual bool has_log_viewer() const { return 0; }

	// tell the CI to ask the user for the value of a parameter.
	virtual int ask_param(bx_param_c* param) { return -1; }
	virtual int ask_param(const char* pname) { return -1; }

	// ask the user for a pathname
	virtual int ask_filename(const char* filename, int maxlen, const char* prompt, const char* the_default, int flags) { return -1; }
	// yes/no dialog
	virtual int ask_yes_no(const char* title, const char* prompt, bool the_default) { return -1; }
	// simple message box
	virtual void message_box(const char* title, const char* message) {}
	// modeless message box
	virtual void* ml_message_box(const char* title, const char* message) { return NULL; }
	// kill modeless message box
	virtual void ml_message_box_kill(void* ptr) {}
	// called at a regular interval, currently by the bx_devices_c::timer()
	virtual void periodic() {}
	virtual int create_disk_image(const char* filename, int sectors, bool overwrite) { return -3; }
	// Tell the configuration interface (CI) that some parameter values have
	// changed.  The CI will reread the parameters and change its display if it's
	// appropriate.  Maybe later: mention which params have changed to save time.
	virtual void refresh_ci() {}
	// forces a vga update.  This was added so that a debugger can force
	// a vga update when single stepping, without having to wait thousands
	// of cycles for the normal vga refresh triggered by the vga timer handler..
	virtual void refresh_vga() {}
	// forces a call to bx_gui.handle_events.  This was added so that a debugger
	// can force the gui events to be handled, so that interactive things such
	// as a toolbar click will be processed.
	virtual void handle_events() {}
	// return first hard disk in ATA interface
	virtual bx_param_c* get_first_hd() { return NULL; }
	// return first cdrom in ATA interface
	virtual bx_param_c* get_first_cdrom() { return NULL; }
	// return 1 if device is connected to a PCI slot
	virtual bool is_pci_device(const char* name) { return 0; }
	// return 1 if device is connected to the AGP slot
	virtual bool is_agp_device(const char* name) { return 0; }
	virtual bool debugger_active() { return false; }
#if BX_DEBUGGER
	// for debugger: same behavior as pressing control-C
	virtual void debug_break() {}
	virtual void debug_interpret_cmd(char* cmd) {}
	virtual char* debug_get_next_command() { return NULL; }
	virtual void debug_puts(const char* text) {}
#endif
	virtual void register_configuration_interface(
		const char* name,
		config_interface_callback_t callback,
		void* userdata) {
	}
	virtual int configuration_interface(const char* name, ci_command_t command) { return -1; }
#if BX_USB_DEBUGGER
	virtual void register_usb_debug_type(int type) {}
	virtual void usb_debug_trigger(int type, int trigger, Bit64u param0, int param1, int param2) {}
	virtual int usb_debug_interface(int type, Bit64u param0, int param1, int param2) { return -1; }
#endif
	virtual int begin_simulation(int argc, char* argv[]) { return -1; }
	virtual int register_runtime_config_handler(void* dev, rt_conf_handler_t handler) { return 0; }
	virtual void unregister_runtime_config_handler(int id) {}
	virtual void update_runtime_options() {}
	typedef bool (*is_sim_thread_func_t)();
	is_sim_thread_func_t is_sim_thread_func;
	virtual void set_sim_thread_func(is_sim_thread_func_t func) {
		is_sim_thread_func = func;
	}
	virtual bool is_sim_thread() { return 1; }
	virtual bool is_wx_selected() const { return 0; }
	virtual void set_debug_gui(bool val) {}
	virtual bool has_debug_gui() const { return 0; }
	// provide interface to bx_gui->set_display_mode() method for config
	// interfaces to use.
	virtual void set_display_mode(disp_mode_t newmode) {}
	virtual bool test_for_text_console() { return 1; }

	// add-on config option support
	virtual bool register_addon_option(const char* keyword, addon_option_parser_t parser, addon_option_save_t save_func) { return 0; }
	virtual bool unregister_addon_option(const char* keyword) { return 0; }
	virtual bool is_addon_option(const char* keyword) { return 0; }
	virtual Bit32s parse_addon_option(const char* context, int num_params, char* params[]) { return -1; }
	virtual Bit32s save_addon_options(FILE* fp) { return -1; }

	// statistics
	virtual void init_statistics() {}
	virtual void cleanup_statistics() {}
	virtual bx_list_c* get_statistics_root() { return NULL; }

	// save/restore support
	virtual void init_save_restore() {}
	virtual void cleanup_save_restore() {}
	virtual bool save_state(const char* checkpoint_path) { return 0; }
	virtual bool restore_config() { return 0; }
	virtual bool restore_logopts() { return 0; }
	virtual bool restore_hardware() { return 0; }
	virtual bx_list_c* get_bochs_root() { return NULL; }
	virtual bool restore_bochs_param(bx_list_c* root, const char* sr_path, const char* restore_name) { return 0; }

	// special config parameter and options functions for plugins
	virtual bool opt_plugin_ctrl(const char* plugname, bool load) { return 0; }
	virtual void init_std_nic_options(const char* name, bx_list_c* menu) {}
	virtual void init_usb_options(const char* usb_name, const char* pname, int maxports, int param0) {}
	virtual int  parse_param_from_list(const char* context, const char* param, bx_list_c* base) { return 0; }
	virtual int  parse_nic_params(const char* context, const char* param, bx_list_c* base) { return 0; }
	virtual int  parse_usb_port_params(const char* context, const char* param,
		int maxports, bx_list_c* base) {
		return -1;
	}
	virtual int  split_option_list(const char* msg, const char* rawopt, char** argv, int max_argv) { return 0; }
	virtual int  write_param_list(FILE* fp, bx_list_c* base, const char* optname, bool multiline) { return 0; }
	virtual int  write_usb_options(FILE* fp, int maxports, bx_list_c* base) { return 0; }

#if BX_USE_GUI_CONSOLE
	virtual int  bx_printf(const char* fmt, ...) { return 0; }
	virtual char* bx_gets(char* s, int size, FILE* stream) { return NULL; }
#endif

};
#if defined(__WXMSW__) || defined(WIN32)
// Just to provide HINSTANCE, etc. in files that have not included bochs.h.
// I don't like this at all, but I don't see a way around it.
#include <windows.h>
#endif

typedef struct BOCHSAPI {  //809
	// standard argc,argv
	//809
	int argc;
	char** argv;
#ifdef WIN32
	char initial_dir[MAX_PATH];
#endif
#ifdef __WXMSW__
	// these are only used when compiling with wxWidgets.  This gives us a
	// place to store the data that was passed to WinMain.
	HINSTANCE hInstance;
	HINSTANCE hPrevInstance;
	LPSTR m_lpCmdLine;
	int nCmdShow;
#endif
} bx_startup_flags_t;

BOCHSAPI extern bx_startup_flags_t bx_startup_flags;
BOCHSAPI extern bool bx_user_quit;