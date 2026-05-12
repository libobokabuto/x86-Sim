#pragma once

#if BX_SUPPORT_X86_64
#define BX_LF_SIGN_BIT  63
#else
#define BX_LF_SIGN_BIT  31
#endif

enum {
    LF_BIT_SD = 0,         /* lazy Sign Flag Delta            */
    LF_BIT_AF = 3,         /* lazy Adjust flag                */
    LF_BIT_PDB = 8,         /* lazy Parity Delta Byte (8 bits) */
    LF_BIT_CF = 31,        /* lazy Carry Flag                 */
    LF_BIT_PO = 30         /* lazy Partial Overflow = CF ^ OF */
};

const Bit32u LF_MASK_SD = (0x01 << LF_BIT_SD);
const Bit32u LF_MASK_AF = (0x01 << LF_BIT_AF);
const Bit32u LF_MASK_PDB = (0xFF << LF_BIT_PDB);
const Bit32u LF_MASK_CF = (0x01 << LF_BIT_CF);
const Bit32u LF_MASK_PO = (0x01 << LF_BIT_PO);

#define ADD_COUT_VEC(op1, op2, result) \
  (((op1) & (op2)) | (((op1) | (op2)) & (~(result))))  //49

#define SUB_COUT_VEC(op1, op2, result) \
  (((~(op1)) & (op2)) | (((~(op1)) ^ (op2)) & (result)))

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

#define SET_FLAGS_OSZAPC_8(carries, result) \
  SET_FLAGS_OSZAPC_SIZE(8, carries, result)
#define SET_FLAGS_OSZAPC_16(carries, result) \
  SET_FLAGS_OSZAPC_SIZE(16, carries, result) //76
#define SET_FLAGS_OSZAPC_32(carries, result) \
  SET_FLAGS_OSZAPC_SIZE(32, carries, result)


#define SET_FLAGS_OSZAPC_LOGIC_16(result_16) \
   SET_FLAGS_OSZAPC_16(0, (result_16)) //88
#define SET_FLAGS_OSZAPC_LOGIC_32(result_32) \
   SET_FLAGS_OSZAPC_32(0, (result_32))

#define SET_FLAGS_OSZAPC_SUB_8(op1_8, op2_8, diff_8) \
  SET_FLAGS_OSZAPC_8(SUB_COUT_VEC((op1_8), (op2_8), (diff_8)), (diff_8))
#define SET_FLAGS_OSZAPC_ADD_16(op1_16, op2_16, sum_16) \
  SET_FLAGS_OSZAPC_16(ADD_COUT_VEC((op1_16), (op2_16), (sum_16)), (sum_16))

struct bx_lazyflags_entry { //193
    bx_address result;
    bx_address auxbits;
    BX_CPP_INLINE unsigned getB_OF(void) const;
    BX_CPP_INLINE unsigned get_OF(void) const;
    BX_CPP_INLINE void set_OF(bool val);
    BX_CPP_INLINE void clear_OF(void);
    BX_CPP_INLINE void assert_OF(void);

    BX_CPP_INLINE unsigned getB_SF(void) const;
    BX_CPP_INLINE unsigned get_SF(void) const;
    BX_CPP_INLINE void set_SF(bool val);
    BX_CPP_INLINE void clear_SF(void);
    BX_CPP_INLINE void assert_SF(void);

    BX_CPP_INLINE unsigned getB_ZF(void) const;
    BX_CPP_INLINE unsigned get_ZF(void) const;
    BX_CPP_INLINE void set_ZF(bool val);
    BX_CPP_INLINE void clear_ZF(void);
    BX_CPP_INLINE void assert_ZF(void);

    BX_CPP_INLINE unsigned getB_AF(void) const;
    BX_CPP_INLINE unsigned get_AF(void) const;
    BX_CPP_INLINE void set_AF(bool val);
    BX_CPP_INLINE void clear_AF(void);
    BX_CPP_INLINE void assert_AF(void);

    BX_CPP_INLINE unsigned getB_PF(void) const;
    BX_CPP_INLINE unsigned get_PF(void) const;
    BX_CPP_INLINE void set_PF(bool val);
    BX_CPP_INLINE void clear_PF(void);
    BX_CPP_INLINE void assert_PF(void);

    BX_CPP_INLINE unsigned getB_CF(void) const;
    BX_CPP_INLINE unsigned get_CF(void) const;
    BX_CPP_INLINE void set_CF(bool val);
    BX_CPP_INLINE void clear_CF(void);
    BX_CPP_INLINE void assert_CF(void);

    BX_CPP_INLINE void set_flags_OxxxxC(Bit32u new_of, Bit32u new_cf)
    { //233
        Bit32u temp_po = new_of ^ new_cf;
        auxbits &= ~(LF_MASK_PO | LF_MASK_CF);
        auxbits |= (temp_po << LF_BIT_PO) | (new_cf << LF_BIT_CF);
    }

    BX_CPP_INLINE void assert_flags_OxxxxC() { set_flags_OxxxxC(1, 1); }
};

BX_CPP_INLINE unsigned bx_lazyflags_entry::getB_OF(void) const
{
    return ((auxbits + (1U << LF_BIT_PO)) >> LF_BIT_CF) & 1;
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::get_OF(void) const
{
    return (auxbits + (1U << LF_BIT_PO)) & (1U << LF_BIT_CF);
}

BX_CPP_INLINE void bx_lazyflags_entry::set_OF(bool val)
{  //254
    bool temp_cf = getB_CF();
    set_flags_OxxxxC(val, temp_cf);
}

BX_CPP_INLINE void bx_lazyflags_entry::clear_OF(void)
{
    bool temp_cf = getB_CF();
    set_flags_OxxxxC(0, temp_cf);
}

BX_CPP_INLINE void bx_lazyflags_entry::assert_OF(void)
{
    unsigned temp_cf = getB_CF();
    set_flags_OxxxxC(1, temp_cf);
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::getB_SF(void) const
{//273
    return ((result >> BX_LF_SIGN_BIT) ^ (auxbits >> LF_BIT_SD)) & 1;
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::get_SF(void) const { return getB_SF(); }

BX_CPP_INLINE void bx_lazyflags_entry::set_SF(bool val)
{
    bool temp_sf = getB_SF();
    auxbits ^= (temp_sf ^ val) << LF_BIT_SD;
}

BX_CPP_INLINE void bx_lazyflags_entry::clear_SF(void) { set_SF(0); }
BX_CPP_INLINE void bx_lazyflags_entry::assert_SF(void) { set_SF(1); }

BX_CPP_INLINE unsigned bx_lazyflags_entry::getB_ZF(void) const
{ //290
    return (0 == result);
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::get_ZF(void) const { return getB_ZF(); }

BX_CPP_INLINE void bx_lazyflags_entry::set_ZF(bool val)
{
    if (val) assert_ZF();
    else clear_ZF();
}

BX_CPP_INLINE void bx_lazyflags_entry::clear_ZF(void)
{
    result |= (1 << 8);
}

BX_CPP_INLINE void bx_lazyflags_entry::assert_ZF(void)
{
    // merge the sign bit into the Sign Delta
    auxbits ^= (((result >> BX_LF_SIGN_BIT) & 1) << LF_BIT_SD);

    // merge the parity bits into the Parity Delta Byte
    Bit32u temp_pdb = (255 & result);
    auxbits ^= (temp_pdb << LF_BIT_PDB);

    // now zero the .result value
    result = 0;
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::getB_AF(void) const
{
    return (auxbits >> LF_BIT_AF) & 1;
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::get_AF(void) const
{
    return (auxbits & LF_MASK_AF);
}

BX_CPP_INLINE void bx_lazyflags_entry::set_AF(bool val)
{
    auxbits &= ~(LF_MASK_AF);
    auxbits |= (val) << LF_BIT_AF;
}

BX_CPP_INLINE void bx_lazyflags_entry::clear_AF(void)
{
    auxbits &= ~(LF_MASK_AF);
}

BX_CPP_INLINE void bx_lazyflags_entry::assert_AF(void)
{
    auxbits |= (LF_MASK_AF);
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::getB_PF(void) const
{
    Bit32u temp = (255 & result);
    temp = temp ^ (255 & (auxbits >> LF_BIT_PDB));
    temp = (temp ^ (temp >> 4)) & 0x0F;
    return (0x9669U >> temp) & 1;
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::get_PF(void) const { return getB_PF(); }

BX_CPP_INLINE void bx_lazyflags_entry::set_PF(bool val)
{
    Bit32u temp_pdb = (255 & result) ^ (!val);
    auxbits &= ~(LF_MASK_PDB);
    auxbits |= (temp_pdb << LF_BIT_PDB);
}

BX_CPP_INLINE void bx_lazyflags_entry::clear_PF(void) { set_PF(0); }
BX_CPP_INLINE void bx_lazyflags_entry::assert_PF(void) { set_PF(1); }

BX_CPP_INLINE unsigned bx_lazyflags_entry::getB_CF(void) const
{ //375
    return (auxbits >> LF_BIT_CF) & 1;
}

BX_CPP_INLINE unsigned bx_lazyflags_entry::get_CF(void) const
{
    return (auxbits & LF_MASK_CF);
}

BX_CPP_INLINE void bx_lazyflags_entry::set_CF(bool val)
{
    bool temp_of = getB_OF();
    set_flags_OxxxxC(temp_of, val);
}

BX_CPP_INLINE void bx_lazyflags_entry::clear_CF(void)
{
    bool temp_of = getB_OF();
    set_flags_OxxxxC(temp_of, 0);
}

BX_CPP_INLINE void bx_lazyflags_entry::assert_CF(void)
{
    bool temp_of = getB_OF();
    set_flags_OxxxxC(temp_of, 1);
}