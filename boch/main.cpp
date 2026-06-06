#include "bochs.h"
#include "memory-bochs.h"
#include "cpu.h"
#include "iodev.h"
//自己添加的头文件，与源码不相同
#include "siminterface.h"

#if BX_SUPPORT_SMP
// multiprocessor simulation, we need an array of cpus
BOCHSAPI BX_CPU_C_PTR* bx_cpu_array = NULL;
#else
// single processor simulation, so there's one of everything
BOCHSAPI BX_CPU_C bx_cpu;
#endif
BOCHSAPI BX_MEM_C bx_mem;

bx_debug_t bx_dbg;


bx_startup_flags_t bx_startup_flags;//78行
bool bx_user_quit;
void bx_init_hardware(void);//70行
int bx_init_main(int argc, char* argv[]);

Bit32u apic_id_mask;  //82
bool simulate_xapic;


bx_pc_system_c bx_pc_system;

int bx_begin_simulation(int argc, char* argv[])
{

	const char* gui_name = "win32";

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--nogui")) {
			gui_name = "nogui";
		}
		else if (!strcmp(argv[i], "--win32")) {
			gui_name = "win32";
		}
	}

	if (bx_gui == NULL) {
		PLUG_load_plugin_var(gui_name, PLUGTYPE_GUI);
	}
	bx_init_hardware();

	BX_CPU(0)->cpu_loop();

	return 0;
}


void bx_init_hardware() {
	//1170
	int i;
	char pname[16];
	bx_list_c* base;
	char buffer[128];
	//内存初始化
	//memSize = 33554432, hostMemSize = 33554432, memBlockSize = 131072
	/*替换的是
	bx_param_num_c *bxp_memsize = SIM->get_param_num(BXPN_MEM_SIZE);
	Bit64u memSize = bxp_memsize->get64() * BX_CONST64(1024*1024);

	bx_param_num_c *bxp_host_memsize = SIM->get_param_num(BXPN_HOST_MEM_SIZE);
	Bit64u hostMemSize = bxp_host_memsize->get64() * BX_CONST64(1024*1024);

	// do not allocate more host memory than needed for emulation of guest RAM
	if (memSize < hostMemSize) hostMemSize = memSize;

	bx_param_num_c *bxp_memblock_size = SIM->get_param_num(BXPN_MEM_BLOCK_SIZE);
	Bit32u memBlockSize = (Bit32u)(bxp_memblock_size->get64() * 1024);
	*/
	Bit64u memSize = BX_CONST64(32 * 1024 * 1024);
	Bit64u hostMemSize = BX_CONST64(32 * 1024 * 1024);
	Bit32u memBlockSize = 131072;

	BX_MEM(0)->init_memory(memSize, hostMemSize, memBlockSize);			//1290



	//加载BIOS
	BX_MEM(0)->load_ROM("E:/Study/codes/bochs/boch/boch/BIOS-bochs-latest", 0, 0);
	//初始化所有
	BX_CPU(0)->initialize();
	bx_pc_system.initialize(15000000);//自己加的

	DEV_init_devices();  //1331
	bx_pc_system.Reset(BX_RESET_HARDWARE);//1341

}

int main_proc(int argc, char* argv[])
{
	int init_ret = 0;
	int cpu_loop_ret = 0;

	//初始化函数调用

	//cpu_loop调用

	bx_begin_simulation(argc, argv);

	return 0;
}
//309行main.cc
int bxmain(void)
{
	//315行
	//bx_init_siminterface();

	bx_init_main(bx_startup_flags.argc, bx_startup_flags.argv);

	
	return 0;
}
//530行main.cc
#if !defined(__WXMSW__)
// normal main function, presently in for all cases except for
// wxWidgets under win32.
int CDECL main(int argc, char* argv[])
{
	bx_startup_flags.argc = argc;
	bx_startup_flags.argv = argv;
#ifdef WIN32
	int arg = 1;
	bool bx_noconsole = 0;
	while (arg < argc) {
		if (!strcmp("-noconsole", argv[arg])) {
			bx_noconsole = 1;
			break;
		}
		arg++;
	}

	if (bx_noconsole) {
		FreeConsole();
	}
	else {
#if BX_WITH_SDL || BX_WITH_SDL2
		// if SDL/win32, try to create a console window.
		if (!RedirectIOToConsole()) {
			return 1;
		}
#endif
		SetConsoleTitle("Bochs for Windows - Console");
	}
#endif
	return bxmain();
}
#endif
//595行main.cc
int bx_init_main(int argc, char* argv[])
{
	//bx_init_bx_dbg();

	//plugin_startup();//暂时不需要

	//bx_init_options();

	//bx_print_header();

	//SIM->get_param_enum(BXPN_BOCHS_START)->set(BX_RUN_START);

	int arg = 1, load_rcfile = 1;

	//SIM->get_param_enum(BXPN_BOCHS_START)->set(BX_QUICK_START);

	int inet = 0;
	inet = main_proc(argc, argv);


	return 0;
}

#if BX_SHOW_IPS
void bx_show_ips_handler(void)
{//1443
	static Bit64u ticks_count = 0;
	static Bit64u counts = 0;

	// amount of system ticks passed from last time the handler was called
	Bit64u ips_count = bx_pc_system.time_ticks() - ticks_count;
	if (ips_count) {
		bx_gui->show_ips((Bit32u)ips_count);
		ticks_count = bx_pc_system.time_ticks();
		counts++;
		if (bx_dbg.print_timestamps) {
			printf("IPS: %u\taverage = %u\t\t(%us)\n",
				(unsigned)ips_count, (unsigned)(ticks_count / counts), (unsigned)counts);
			fflush(stdout);
		}
	}
	return;
}
#endif