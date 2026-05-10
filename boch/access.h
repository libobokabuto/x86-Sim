#pragma once

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