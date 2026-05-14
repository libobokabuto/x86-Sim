#pragma once
#if BX_USE_DMA_SMF
#  define BX_DMA_SMF  static
#  define BX_DMA_THIS theDmaDevice->
#else
#  define BX_DMA_SMF
#  define BX_DMA_THIS this->
#endif

class bx_dma_c : public bx_dma_stub_c {
public:
	bx_dma_c();
    virtual ~bx_dma_c();
    struct {
        bool DRQ[4];  // DMA Request
        bool DACK[4]; // DMA Acknowlege

        bool mask[4];
        bool flip_flop;
        Bit8u   status_reg;
        Bit8u   command_reg;
        bool ctrl_disabled;
        struct {
            struct {
                Bit8u mode_type;
                bool address_decrement;
                bool autoinit_enable;
                Bit8u transfer_type;
            } mode;
            Bit16u  base_address;
            Bit16u  current_address;
            Bit16u  base_count;
            Bit16u  current_count;
            Bit8u   page_reg;
            bool used;
        } chan[4]; /* DMA channels 0..3 */
    } s[2];  // state information DMA-1 / DMA-2

    bool HLDA;    // Hold Acknowlege
    bool TC;      // Terminal Count

    Bit8u   ext_page_reg[16]; // Extra page registers (unused)

    struct {
        Bit16u(*dmaRead8)(Bit8u* data_byte, Bit16u maxlen);
        Bit16u(*dmaWrite8)(Bit8u* data_byte, Bit16u maxlen);
        Bit16u(*dmaRead16)(Bit16u* data_word, Bit16u maxlen);
        Bit16u(*dmaWrite16)(Bit16u* data_word, Bit16u maxlen);
    } h[4]; // DMA read and write handlers
};
