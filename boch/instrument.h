#pragma once
#define BX_INSTR_RESET(cpu_id, type)
#define BX_INSTR_HLT(cpu_id) //148

#define BX_INSTR_CNEAR_BRANCH_TAKEN(cpu_id, branch_eip, new_eip) //156
#define BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(cpu_id, branch_eip) //157
#define BX_INSTR_UCNEAR_BRANCH(cpu_id, what, branch_eip, new_eip)
#define BX_INSTR_FAR_BRANCH(cpu_id, what, prev_cs, prev_eip, new_cs, new_eip) //159
/* decoding completed 161ÐÐ*/
#define BX_INSTR_OPCODE(cpu_id, i, opcode, len, is32, is64)

#define BX_INSTR_EXCEPTION(cpu_id, vector, error_code) //165
#define BX_INSTR_INTERRUPT(cpu_id, vector)//166
#define BX_INSTR_HWINTERRUPT(cpu_id, vector, cs, eip) //167
#define BX_INSTR_TLB_CNTRL(cpu_id, what, new_cr3) //172
#define BX_INSTR_BEFORE_EXECUTION(cpu_id, i) //176
#define BX_INSTR_AFTER_EXECUTION(cpu_id, i)
#define BX_INSTR_REPEAT_ITERATION(cpu_id, i) //178
#define BX_INSTR_LIN_ACCESS(cpu_id, lin, phy, len, memtype, rw) //181
#define BX_INSTR_PHY_ACCESS(cpu_id, phy, len, memtype, rw)//184
#define BX_INSTR_INP(addr, len)//187
#define BX_INSTR_INP2(addr, len, val)//188
#define BX_INSTR_OUTP(addr, len, val) //189
#define BX_INSTR_WRMSR(cpu_id, addr, value) //192
#define BX_INSTR_VMEXIT(cpu_id, reason, qualification) //195