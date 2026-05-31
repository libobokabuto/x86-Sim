#pragma once
#define BX_IODEV_USB_UHCI_H

#if BX_USE_USB_UHCI_SMF
#  define BX_UHCI_THIS theUSB_UHCI->
#  define BX_UHCI_THIS_PTR theUSB_UHCI
#else
#  define BX_UHCI_THIS this->
#  define BX_UHCI_THIS_PTR this
#endif

class bx_usb_uhci_c : public bx_uhci_core_c {
public:
	bx_usb_uhci_c();
	virtual ~bx_usb_uhci_c();
	virtual void init(void);
	virtual void reset(unsigned);
	virtual void register_state(void);
	virtual void after_restore_state(void);
	
private:
	Bit8u device_change;
	int rt_conf_id;

	void init_device(Bit8u port, bx_list_c* portconf);
	static void remove_device(Bit8u port);

	static void runtime_config_handler(void*);
	void runtime_config(void);

	static Bit64s usb_param_handler(bx_param_c* param, bool set, Bit64s val);
	static Bit64s usb_param_oc_handler(bx_param_c* param, bool set, Bit64s val);
	static bool usb_param_enable_handler(bx_param_c* param, bool en);
};