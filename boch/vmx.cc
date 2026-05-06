#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "iodev.h"

extern VMCS_Mapping vmcs_map;

extern int get_exception_class(unsigned vector);
extern int get_exception_type(unsigned vector);
extern bool exception_push_error(unsigned vector);
