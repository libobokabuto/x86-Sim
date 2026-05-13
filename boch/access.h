#pragma once


BX_CPP_INLINE void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_word_32(unsigned s, Bit32u offset, Bit16u data)
{  //34
	Bit32u laddr = agen_write32(s, offset, 2);
	write_linear_word(s, laddr, data);
}

BX_CPP_INLINE void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_dword_32(unsigned s, Bit32u offset, Bit32u data)
{
	Bit32u laddr = agen_write32(s, offset, 4);
	write_linear_dword(s, laddr, data);
}

BX_CPP_INLINE Bit8u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_virtual_byte(unsigned s, bx_address offset)
{
	bx_address laddr = agen_read(s, offset, 1);
	return read_linear_byte(s, laddr);
}

 BX_CPP_INLINE Bit16u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_virtual_word(unsigned s, bx_address offset)
{
	bx_address laddr = agen_read(s, offset, 2);
	return read_linear_word(s, laddr);
}

 BX_CPP_INLINE void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_byte(unsigned s, bx_address offset, Bit8u data)
 {
	 bx_address laddr = agen_write(s, offset, 1);
	 write_linear_byte(s, laddr, data);
 }

 BX_CPP_INLINE void BX_CPP_AttrRegparmN(3)
	 BX_CPU_C::write_virtual_word(unsigned s, bx_address offset, Bit16u data)
 { //189
	 bx_address laddr = agen_write(s, offset, 2);
	 write_linear_word(s, laddr, data);
 }

 BX_CPP_INLINE void BX_CPP_AttrRegparmN(3)
	 BX_CPU_C::write_virtual_dword(unsigned s, bx_address offset, Bit32u data)
 {
	 bx_address laddr = agen_write(s, offset, 4);
	 write_linear_dword(s, laddr, data);
 }
