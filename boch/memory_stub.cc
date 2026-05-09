#include "bochs.h"
#include "pc_system.h"
#include "cpu.h"
#include "memory-bochs.h"
#define LOG_THIS BX_MEM(0)->

BX_CPP_INLINE bool is_power_of_2(Bit64u x)
{
	//31
	return (x & (x - 1)) == 0;
}

#define BX_MEM_VECTOR_ALIGN 4096 //37
Bit8u* const BX_MEMORY_STUB_C::swapped_out = ((Bit8u*)NULL - sizeof(Bit8u)); //44

BX_MEMORY_STUB_C::BX_MEMORY_STUB_C()
{
}

BX_MEMORY_STUB_C::~BX_MEMORY_STUB_C()
{
}

Bit8u* BX_MEMORY_STUB_C::alloc_vector_aligned(Bit64u bytes, Bit64u alignment)
{
	//80
	Bit64u test_mask = alignment - 1;
	BX_MEM_THIS actual_vector = new Bit8u[(Bit32u)(bytes + test_mask)];
	if (BX_MEM_THIS actual_vector == 0) {
		//BX_PANIC(("alloc_vector_aligned: unable to allocate host RAM !"));
		return 0;
	}
	// round address forward to nearest multiple of alignment.  Alignment
	// MUST BE a power of two for this to work.
	Bit64u masked = ((Bit64u)(BX_MEM_THIS actual_vector + test_mask)) & ~test_mask;
	Bit8u* vector = (Bit8u*)masked;
	// sanity check: no lost bits during pointer conversion
	assert(sizeof(masked) >= sizeof(vector));
	// sanity check: after realignment, everything fits in allocated space
	assert(vector + bytes <= BX_MEM_THIS actual_vector + bytes + test_mask);
	return vector;
}

void BX_MEMORY_STUB_C::init_memory(Bit64u guest, Bit64u host, Bit32u block_size)
{
	//99行
	// accept only memory size which is multiply of 1M
	//BX_ASSERT((host & 0xfffff) == 0); 这行是login.h
	//BX_ASSERT((guest & 0xfffff) == 0);

	if (!is_power_of_2(block_size)) {

		//BX_PANIC(("Block size %d is not power of two !", block_size));
	}

	if (BX_MEM_THIS actual_vector != NULL) {
		//BX_INFO(("freeing existing memory vector"));
		delete[] BX_MEM_THIS actual_vector;
		BX_MEM_THIS actual_vector = NULL;
		BX_MEM_THIS vector = NULL;
		BX_MEM_THIS blocks = NULL;
	}

	BX_MEM_THIS vector = alloc_vector_aligned(host + BIOSROMSZ + EXROMSIZE + 4096, BX_MEM_VECTOR_ALIGN);
	//BX_INFO(("allocated memory at %p. after alignment, vector=%p, block_size = %dK",BX_MEM_THIS actual_vector, BX_MEM_THIS vector, block_size / 1024));

	BX_MEM_THIS len = guest;
	BX_MEM_THIS allocated = host;
	BX_MEM_THIS rom = &BX_MEM_THIS vector[host];
	BX_MEM_THIS bogus = &BX_MEM_THIS vector[host + BIOSROMSZ + EXROMSIZE];
	memset(BX_MEM_THIS rom, 0xff, BIOSROMSZ + EXROMSIZE + 4096);

	BX_MEM_THIS block_size = block_size;
	// block must be large enough to fit num_blocks in 32-bit
	// BX_ASSERT((BX_MEM_THIS len / BX_MEM_THIS block_size) <= 0xffffffff);
	Bit32u num_blocks = (Bit32u)(BX_MEM_THIS len / BX_MEM_THIS block_size);
	//BX_INFO(("%.2fMB", (float)(BX_MEM_THIS len / (1024.0 * 1024.0))));
	//BX_INFO(("mem block size = 0x%08x, blocks=%u", BX_MEM_THIS block_size, num_blocks));
	BX_MEM_THIS blocks = new Bit8u * [num_blocks];

	if (0) {
		// all guest memory is allocated, just map it
		for (unsigned idx = 0; idx < num_blocks; idx++) {
			BX_MEM_THIS blocks[idx] = BX_MEM_THIS vector + (idx * BX_MEM_THIS block_size);
		}
		BX_MEM_THIS used_blocks = num_blocks;
	}
	else {
		// host cannot allocate all requested guest memory
		for (unsigned idx = 0; idx < num_blocks; idx++) {
			BX_MEM_THIS blocks[idx] = NULL;
		}
		BX_MEM_THIS used_blocks = 0;
	}
}
void BX_MEMORY_STUB_C::allocate_block(Bit32u block)
{
	//178
}
Bit8u* BX_MEMORY_STUB_C::get_vector(bx_phy_address addr)
{
	Bit32u block = (Bit32u)(addr / BX_MEM_THIS block_size);
#if (BX_LARGE_RAMFILE)
	if (!BX_MEM_THIS blocks[block] || (BX_MEM_THIS blocks[block] == BX_MEM_THIS swapped_out))
#else
	if (!BX_MEM_THIS blocks[block])
#endif
		allocate_block(block);

	return BX_MEM_THIS blocks[block] + (Bit32u)(addr & (BX_MEM_THIS block_size - 1));
}

bool BX_MEMORY_STUB_C::is_monitor(bx_phy_address begin_addr, unsigned len)
{
	//525
	
	for (int i = 0; i < BX_SMP_PROCESSORS; i++) {
		if (BX_CPU(i)->is_monitor(begin_addr, len))
			return true;
	}
	
	return false; // this is NOT monitored page
}

void BX_MEMORY_STUB_C::readPhysicalPage(BX_CPU_C* cpu, bx_phy_address addr, unsigned len, void* data)
{
	Bit8u* data_ptr;
	bx_phy_address a20addr = A20ADDR(addr);

	// Note: accesses should always be contained within a single page
	if ((addr >> 12) != ((addr + len - 1) >> 12)) {
		//BX_PANIC(("readPhysicalPage: cross page access at address 0x" FMT_PHY_ADDRX ", len=%d", addr, len));
	}

	if (a20addr < BX_MEM_THIS len) {
		if (len == 8) {
			*(Bit64u*)data = ReadHostQWordFromLittleEndian((Bit64u*)BX_MEM_THIS get_vector(a20addr));
			return;
		}
		if (len == 4) {
			*(Bit32u*)data = ReadHostDWordFromLittleEndian((Bit32u*)BX_MEM_THIS get_vector(a20addr));
			return;
		}
		if (len == 2) {
			*(Bit16u*)data = ReadHostWordFromLittleEndian((Bit16u*)BX_MEM_THIS get_vector(a20addr));
			return;
		}
		if (len == 1) {
			*(Bit8u*)data = *(BX_MEM_THIS get_vector(a20addr));
			return;
		}
		// len == other case can just fall thru to special cases handling

#ifdef BX_LITTLE_ENDIAN
		data_ptr = (Bit8u*)data;
#else // BX_BIG_ENDIAN
		data_ptr = (Bit8u*)data + (len - 1);
#endif

		// addr *not* in range 000A0000 .. 000FFFFF
		while (1) {
			// Read in chunks of 8 bytes if we can
			if ((len & 7) == 0) {
				*((Bit64u*)data_ptr) = ReadHostQWordFromLittleEndian((Bit64u*)BX_MEM_THIS get_vector(a20addr));
				len -= 8;
				a20addr += 8;
#ifdef BX_LITTLE_ENDIAN
				data_ptr += 8;
#else
				data_ptr -= 8;
#endif

				if (len == 0) return;
			}
			else {
				*data_ptr = *(BX_MEM_THIS get_vector(a20addr));
				if (len == 1) return;
				len--;
				a20addr++;
#ifdef BX_LITTLE_ENDIAN
				data_ptr++;
#else // BX_BIG_ENDIAN
				data_ptr--;
#endif
			}
		}
	}
	else  // access outside limits of physical memory
	{
		// bogus memory
		memset(data, 0xFF, len);
	}
}
