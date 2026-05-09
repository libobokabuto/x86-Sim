#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
#define LOG_THIS BX_MEM_THIS

void BX_MEM_C::readPhysicalPage(BX_CPU_C* cpu, bx_phy_address addr, unsigned len, void* data)
{
    Bit8u* data_ptr;
    bx_phy_address a20addr = A20ADDR(addr);
    struct memory_handler_struct* memory_handler = NULL;

    // Note: accesses should always be contained within a single page
    if ((addr >> 12) != ((addr + len - 1) >> 12)) {
        //BX_PANIC(("readPhysicalPage: cross page access at address 0x" FMT_PHY_ADDRX ", len=%d", addr, len));
    }

    bool is_bios = (a20addr >= (bx_phy_address)BX_MEM_THIS bios_rom_addr);
#if BX_PHY_ADDRESS_LONG
    if (a20addr > BX_CONST64(0xffffffff)) is_bios = false;
#endif

    if (cpu != NULL) {
#if BX_SUPPORT_IODEBUG
        bx_devices.pluginIODebug->mem_read(cpu, a20addr, len, data);
#endif

        if ((a20addr >= 0x000a0000 && a20addr < 0x000c0000) && BX_MEM_THIS smram_available)
        {
            // SMRAM memory space
            if (BX_MEM_THIS smram_enable || (cpu->smm_mode() && !BX_MEM_THIS smram_restricted))
                goto mem_read;
        }
    }

    memory_handler = BX_MEM_THIS memory_handlers[a20addr >> 20];
    while (memory_handler) {
        if (memory_handler->begin <= a20addr &&
            memory_handler->end >= a20addr &&
            memory_handler->read_handler(a20addr, len, data, memory_handler->param))
        {
            return;
        }
        memory_handler = memory_handler->next;
    }

mem_read:

    if ((a20addr < BX_MEM_THIS len) && !is_bios) {
        // all of data is within limits of physical memory
        if (a20addr < 0x000a0000 || a20addr >= 0x00100000)
        {
            BX_MEMORY_STUB_C::readPhysicalPage(cpu, addr, len, data);
            return;
        }

#ifdef BX_LITTLE_ENDIAN
        data_ptr = (Bit8u*)data;
#else // BX_BIG_ENDIAN
        data_ptr = (Bit8u*)data + (len - 1);
#endif

        // addr must be in range 000A0000 .. 000FFFFF
        for (unsigned i = 0; i < len; i++) {

            // SMMRAM
            if (a20addr < 0x000c0000) {
                // devices are not allowed to access SMMRAM under VGA memory
                if (cpu) *data_ptr = *(BX_MEM_THIS get_vector(a20addr));
                goto inc_one;
            }

#if BX_SUPPORT_PCI
            if (BX_MEM_THIS pci_enabled && ((a20addr & 0xfffc0000) == 0x000c0000)) {
                unsigned area = (unsigned)(a20addr >> 14) & 0x0f;
                if (area > BX_MEM_AREA_F0000) area = BX_MEM_AREA_F0000;
                if (BX_MEM_THIS memory_type[area][0] == 0) {
                    // Read from ROM
                    if ((a20addr & 0xfffe0000) == 0x000e0000) {
                        // last 128K of BIOS ROM mapped to 0xE0000-0xFFFFF
                        if (BX_MEM_THIS flash_type > 0) {
                            *data_ptr = BX_MEM_THIS flash_read(BIOS_MAP_LAST128K(a20addr));
                        }
                        else {
                            *data_ptr = BX_MEM_THIS rom[BIOS_MAP_LAST128K(a20addr)];
                        }
                    }
                    else {
                        *data_ptr = BX_MEM_THIS rom[(a20addr & EXROM_MASK) + BIOSROMSZ];
                    }
                }
                else {
                    // Read from ShadowRAM
                    *data_ptr = *(BX_MEM_THIS get_vector(a20addr));
                }
            }
            else
#endif  // #if BX_SUPPORT_PCI
            {
                if ((a20addr & 0xfffc0000) != 0x000c0000) {
                    *data_ptr = *(BX_MEM_THIS get_vector(a20addr));
                }
                else if ((a20addr & 0xfffe0000) == 0x000e0000) {
                    // last 128K of BIOS ROM mapped to 0xE0000-0xFFFFF
                    *data_ptr = BX_MEM_THIS rom[BIOS_MAP_LAST128K(a20addr)];
                }
                else {
                    *data_ptr = BX_MEM_THIS rom[(a20addr & EXROM_MASK) + BIOSROMSZ];
                }
            }

        inc_one:
            a20addr++;
#ifdef BX_LITTLE_ENDIAN
            data_ptr++;
#else // BX_BIG_ENDIAN
            data_ptr--;
#endif

        }
    }
    else  // access outside limits of physical memory
    {
#if BX_PHY_ADDRESS_LONG
        if (a20addr > BX_CONST64(0xffffffff)) {
            memset(data, 0xFF, len);
            return;
        }
#endif

#ifdef BX_LITTLE_ENDIAN
        data_ptr = (Bit8u*)data;
#else // BX_BIG_ENDIAN
        data_ptr = (Bit8u*)data + (len - 1);
#endif

        if (is_bios) {
            for (unsigned i = 0; i < len; i++) {
                if (BX_MEM_THIS flash_type > 0) {
                    *data_ptr = BX_MEM_THIS flash_read(a20addr & BIOS_MASK);
                }
                else {
                    *data_ptr = BX_MEM_THIS rom[a20addr & BIOS_MASK];
                }
                a20addr++;
#ifdef BX_LITTLE_ENDIAN
                data_ptr++;
#else // BX_BIG_ENDIAN
                data_ptr--;
#endif
            }
        }
        else {
            // bogus memory
            memset(data, 0xFF, len);
        }
    }
}