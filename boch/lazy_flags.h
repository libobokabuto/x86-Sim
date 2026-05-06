#pragma once

enum {
    LF_BIT_SD = 0,         /* lazy Sign Flag Delta            */
    LF_BIT_AF = 3,         /* lazy Adjust flag                */
    LF_BIT_PDB = 8,         /* lazy Parity Delta Byte (8 bits) */
    LF_BIT_CF = 31,        /* lazy Carry Flag                 */
    LF_BIT_PO = 30         /* lazy Partial Overflow = CF ^ OF */
};

const Bit32u LF_MASK_SD = (0x01 << LF_BIT_SD);
const Bit32u LF_MASK_AF = (0x01 << LF_BIT_AF);//44
const Bit32u LF_MASK_PDB = (0xFF << LF_BIT_PDB);//45


//63
#define SET_FLAGS_OSZAPC_SIZE(size, lf_carries, lf_result) { \
  bx_address temp = ((lf_carries) & (LF_MASK_AF)) | \
        (((lf_carries) >> (size - 2)) << LF_BIT_PO); \
  BX_CPU_THIS_PTR oszapc.result = (bx_address)(Bit##size##s)(lf_result); \
  if ((size) == 32) temp = ((lf_carries) & ~(LF_MASK_PDB | LF_MASK_SD)); \
  if ((size) == 16) temp = ((lf_carries) & (LF_MASK_AF)) | ((lf_carries) << 16); \
  if ((size) == 8)  temp = ((lf_carries) & (LF_MASK_AF)) | ((lf_carries) << 24); \
  BX_CPU_THIS_PTR oszapc.auxbits = (bx_address)(Bit32u)temp; \
}

#define SET_FLAGS_OSZAPC_16(carries, result) \
  SET_FLAGS_OSZAPC_SIZE(16, carries, result) //76
#define SET_FLAGS_OSZAPC_32(carries, result) \
  SET_FLAGS_OSZAPC_SIZE(32, carries, result)


#define SET_FLAGS_OSZAPC_LOGIC_16(result_16) \
   SET_FLAGS_OSZAPC_16(0, (result_16)) //88
#define SET_FLAGS_OSZAPC_LOGIC_32(result_32) \
   SET_FLAGS_OSZAPC_32(0, (result_32))

struct bx_lazyflags_entry { //193
    bx_address result;
    bx_address auxbits;
};