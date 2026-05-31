#pragma once
#define BX_IODEV_UHCI_CORE_H

#define USB_UHCI_PORTS 2

#define STATUS2_IOC (1<<0)
#define STATUS2_SPD (1<<1)

#define USB_UHCI_QUEUE_STACK_SIZE  256
#define USB_UHCI_LOOP_COUNT        256

#define USB_UHCI_IS_LINK_VALID(item)  ((item & 1) == 0)  // return TRUE if valid link address
#define USB_UHCI_IS_LINK_QUEUE(item)  ((item & 2) == 2)  // return TRUE if link is a queue pointer

// the standard max bandwidth (bytes per frame) for the UHCI is 1280 bytes
#define USB_UHCI_STD_MAX_BANDWIDTH  1280

struct USB_UHCI_QUEUE_STACK {
	int    queue_cnt;
	Bit32u queue_stack[USB_UHCI_QUEUE_STACK_SIZE];
};

typedef struct {
	int    timer_index;
    struct {
        bool  max_packet_size; //(bit 7) 0 = 32 bytes, 1 = 64 bytes
        bool  configured;      //(bit 6)
        bool  debug;           //(bit 5)
        bool  resume;          //(bit 4)
        bool  suspend;         //(bit 3)
        bool  reset;           //(bit 2)
        bool  host_reset;      //(bit 1)
        bool  schedule;        //(bit 0) 0 = Stop, 1 = Run
    } usb_command;

    struct {
        bool  host_halted;     //(bit 5)
        bool  host_error;      //(bit 4)
        bool  pci_error;       //(bit 3)
        bool  resume;          //(bit 2)
        bool  error_interrupt; //(bit 1)
        bool  interrupt;       //(bit 0)
        Bit8u status2; // bit 0 and 1 are used to generate the interrupt
    } usb_status;

    struct {
        bool  short_packet; //(bit 3)
        bool  on_complete;  //(bit 2)
        bool  resume;       //(bit 1)
        bool  timeout_crc;  //(bit 0)
    } usb_enable;

    struct {
        Bit16u frame_num;
    } usb_frame_num;

    struct {
        Bit32u frame_base;
    } usb_frame_base;

    struct {
        Bit8u sof_timing;
    } usb_sof;

    struct {
        // our data
        usb_device_c* device;   // device connected to this port

        // bit reps of actual port
        bool suspend;
        bool over_current_change;
        bool over_current;
        bool reset;
        bool low_speed;
        bool resume;
        bool line_dminus;
        bool line_dplus;
        bool enable_changed;
        bool enabled;
        bool connect_changed;
        bool status;
    } usb_port[USB_UHCI_PORTS];

    int    max_bandwidth;  // standard USB 1.1 is 1280 bytes (VTxxxxx models allowed a few less (1023))
    int    loop_reached;   // did we reach our bandwidth loop limit
    Bit8u  devfunc;
} bx_uhci_core_t;

#pragma pack (push, 1)
struct TD {
    Bit32u dword0;
    Bit32u dword1;
    Bit32u dword2;
    Bit32u dword3;
};

struct QUEUE {
    Bit32u horz;
    Bit32u vert;
};
#pragma pack (pop)

class bx_uhci_core_c : public bx_pci_device_c {
public:
    bx_uhci_core_c();
    virtual ~bx_uhci_core_c();
    virtual void init_uhci(Bit8u devfunc, Bit16u venid, Bit16u devid, Bit8u rev, Bit8u headt, Bit8u intp);
    virtual void reset_uhci(unsigned);
    void    uhci_register_state(bx_list_c* parent);
    virtual void after_restore_state(void);
    virtual void set_port_device(int port, usb_device_c* dev);
    virtual void pci_write_handler(Bit8u address, Bit32u value, unsigned io_len);

    int event_handler(int event, void* ptr, int port);

protected:
    bx_uhci_core_t hub;
    Bit8u          global_reset;

    USBAsync* packets;

    void update_irq(void);

    int  broadcast_packet(USBPacket* p);
    bool set_connect_status(Bit8u port, bool connected);

    bool uhci_add_queue(struct USB_UHCI_QUEUE_STACK* stack, const Bit32u addr);
    static void uhci_timer_handler(void*);
    void uhci_timer(void);
    bool DoTransfer(Bit32u address, struct TD*);
    void set_status(struct TD* td, bool active, bool stalled, bool data_buffer_error, bool babble,
        bool nak, bool crc_time_out, bool bitstuff_error, Bit16u act_len);

    static Bit32u read_handler(void* this_ptr, Bit32u address, unsigned io_len);
    static void   write_handler(void* this_ptr, Bit32u address, Bit32u value, unsigned io_len);
    Bit32u read(Bit32u address, unsigned io_len);
    void   write(Bit32u address, Bit32u value, unsigned io_len);
};