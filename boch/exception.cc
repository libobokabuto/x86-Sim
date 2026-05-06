#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#include "param_names.h"
#include "iodev.h"


enum ExceptionType { //844
    BX_ET_BENIGN = 0,
    BX_ET_CONTRIBUTORY = 1,
    BX_ET_PAGE_FAULT = 2,
    BX_ET_DOUBLE_FAULT = 10
};
/*
int get_exception_class(unsigned vector)
{
    //899
    if (vector < BX_CPU_HANDLED_EXCEPTIONS)
        return exceptions_info[vector].exception_class;
    else
        return BX_EXCEPTION_CLASS_FAULT;
}

int BX_CPU_C::get_exception_type(unsigned vector)
{
    //907
    if (vector < BX_CPU_HANDLED_EXCEPTIONS) {
        if (vector == BX_CP_EXCEPTION)
            if (!BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_CET))
                return BX_ET_BENIGN;
        if (vector == BX_SX_EXCEPTION)
            if (!BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_SVM))
                return BX_ET_BENIGN;
        return exceptions_info[vector].exception_type;
    }
    else
        return BX_ET_BENIGN;
}

bool BX_CPU_C::exception_push_error(unsigned vector)
{
    //922
    if (vector < BX_CPU_HANDLED_EXCEPTIONS) {
        if (vector == BX_CP_EXCEPTION)
            if (!BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_CET)) return false;
        if (vector == BX_SX_EXCEPTION)
            if (!BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_SVM)) return false;
        return exceptions_info[vector].push_error;
    }
    else
        return false;
}
*/
void BX_CPU_C::exception(unsigned vector, Bit16u error_code)
{
    
    //937
    /*
    unsigned exception_type = BX_ET_BENIGN;
    unsigned exception_class = BX_EXCEPTION_CLASS_FAULT;
    bool push_error = false;

    if (vector < BX_CPU_HANDLED_EXCEPTIONS) {
        push_error = exception_push_error(vector);
        exception_class = get_exception_class(vector);
        exception_type = get_exception_type(vector);
    }
    else {
        //BX_PANIC(("exception(%u): bad vector", vector));
    }
    */
    /* Excluding page faults and double faults, error_code may not have the
     * least significant bit set correctly. This correction is applied first
     * to make the change transparent to any instrumentation.
     */
    /*
    if (push_error) {
        if (vector != BX_PF_EXCEPTION && vector != BX_DF_EXCEPTION && vector != BX_CP_EXCEPTION && vector != BX_SX_EXCEPTION) {
            error_code = (error_code & 0xfffe) | (Bit16u)(BX_CPU_THIS_PTR EXT);
        }
    }

    //BX_DEBUG(("exception(0x%02x): error_code=%04x", vector, error_code));

    if (real_mode()) {
        push_error = false; // not INT, no error code pushed
        error_code = 0;
    }

    BX_INSTR_EXCEPTION(BX_CPU_ID, vector, error_code);

#if BX_DEBUGGER
    if (bx_dbg.debugger_active)
        bx_dbg_exception(BX_CPU_ID, vector, error_code);
#endif

#if BX_SUPPORT_VMX
    VMexit_Event(BX_HARDWARE_EXCEPTION, vector, error_code, push_error);
#endif

#if BX_SUPPORT_SVM
    SvmInterceptException(BX_HARDWARE_EXCEPTION, vector, error_code, push_error);
#endif

    if (exception_class == BX_EXCEPTION_CLASS_FAULT)
    {
        // restore RIP/RSP to value before error occurred
        RIP = BX_CPU_THIS_PTR prev_rip;
        if (BX_CPU_THIS_PTR speculative_rsp) {
            RSP = BX_CPU_THIS_PTR prev_rsp;
#if BX_SUPPORT_CET
            SSP = BX_CPU_THIS_PTR prev_ssp;
#endif
        }
        BX_CPU_THIS_PTR speculative_rsp = false;

        if (vector != BX_DB_EXCEPTION) BX_CPU_THIS_PTR assert_RF();

        if (BX_CPU_THIS_PTR last_exception_type == BX_ET_DOUBLE_FAULT)
        {
            debug(BX_CPU_THIS_PTR prev_rip); // print debug information to the log
#if BX_SUPPORT_VMX
            VMexit_TripleFault();
#endif
#if BX_SUPPORT_SVM
            if (BX_CPU_THIS_PTR in_svm_guest) {
                if (SVM_INTERCEPT(SVM_INTERCEPT0_SHUTDOWN)) Svm_Vmexit(SVM_VMEXIT_SHUTDOWN);
            }
#endif
#if BX_DEBUGGER
            // trap into debugger (the same as when a PANIC occurs)
            if (bx_dbg.debugger_active) bx_debug_break();
#endif
            if (SIM->get_param_bool(BXPN_RESET_ON_TRIPLE_FAULT)->get()) {
                //BX_ERROR(("exception(): 3rd (%d) exception with no resolution, shutdown status is %02xh, resetting", vector, DEV_cmos_get_reg(0x0f)));
                bx_pc_system.Reset(BX_RESET_HARDWARE);
            }
            else {
               // BX_PANIC(("exception(): 3rd (%d) exception with no resolution", vector));
               // BX_ERROR(("WARNING: Any simulation after this point is completely bogus !"));
                shutdown();
            }
            longjmp(BX_CPU_THIS_PTR jmp_buf_env, 1); // go back to main decode loop
        }
    }

    if (vector == BX_DB_EXCEPTION) {
        // Commit debug events to DR6: preserve DR5.BS and DR6.BD values,
        // only software can clear them
        BX_CPU_THIS_PTR dr6.val32 = (BX_CPU_THIS_PTR dr6.val32 & 0xffff6ff0) |
            (BX_CPU_THIS_PTR debug_trap & 0x0000e00f);

        // clear GD flag in the DR7 prior entering debug exception handler
        BX_CPU_THIS_PTR dr7.set_GD(0);
    }

    BX_CPU_THIS_PTR EXT = 1;
    */
    /* if we've already had 1st exception, see if 2nd causes a
     * Double Fault instead. Otherwise, just record 1st exception.
     */
     /*
    if (exception_type != BX_ET_DOUBLE_FAULT) {
        if (!is_exception_OK[BX_CPU_THIS_PTR last_exception_type][exception_type]) {
            exception(BX_DF_EXCEPTION, 0);
        }
    }

    BX_CPU_THIS_PTR last_exception_type = exception_type;

    interrupt(vector, BX_HARDWARE_EXCEPTION, push_error, error_code);

    BX_CPU_THIS_PTR last_exception_type = 0; // error resolved

    longjmp(BX_CPU_THIS_PTR jmp_buf_env, 1); // go back to main decode loop
    */
}