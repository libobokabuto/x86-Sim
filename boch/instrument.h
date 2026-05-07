#pragma once
#define BX_INSTR_FAR_BRANCH(cpu_id, what, prev_cs, prev_eip, new_cs, new_eip) //159
/* decoding completed 161ÐÐ*/
#define BX_INSTR_OPCODE(cpu_id, i, opcode, len, is32, is64)

#define BX_INSTR_EXCEPTION(cpu_id, vector, error_code) //165

#define BX_INSTR_BEFORE_EXECUTION(cpu_id, i) //176
#define BX_INSTR_AFTER_EXECUTION(cpu_id, i)


#define BX_INSTR_OUTP(addr, len, val) //189