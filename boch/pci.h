#pragma once
#define BX_IODEV_PCI_BRIDGE_H

#if BX_USE_PCI_SMF
#  define BX_PCI_SMF  static
#  define BX_PCI_THIS thePciBridge->
#else
#  define BX_PCI_SMF
#  define BX_PCI_THIS this->
#endif

#define BX_PCI_DEVICE(device, function) ((device)<<3 | (function))

enum {
	BX_PCI_INTA = 1,
	BX_PCI_INTB = 2,
	BX_PCI_INTC = 3,
	BX_PCI_INTD = 4
};

#if BX_SUPPORT_PCI
class bx_pci_vbridge_c;

class bx_pci_bridge_c : public bx_pci_device_c {
public:
	


private:
	

	unsigned chipset;
	Bit8u DRBA[8];
	Bit8u dram_detect;
	Bit32u gart_base;
	bx_pci_vbridge_c* vbridge;
};

class bx_pci_vbridge_c : public bx_pci_device_c {
public:
	
};
#endif



