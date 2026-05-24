#pragma once
#define BX_PC_SPEAKER_H

#if BX_SUPPORT_SOUNDLOW
#define DSP_EVENT_BUFSIZE 4800
#endif

class bx_soundlow_waveout_c;

class bx_speaker_c : public bx_speaker_stub_c {
public:
    bx_speaker_c();
    virtual ~bx_speaker_c();

    virtual void init(void);
    virtual void reset(unsigned int);

    void beep_on(float frequency);
    void beep_off();
    void set_line(bool level);
#if BX_SUPPORT_SOUNDLOW
    Bit32u beep_generator(Bit16u rate, Bit8u* buffer, Bit32u len);
#if BX_HAVE_REALTIME_USEC
    Bit32u dsp_generator(Bit16u rate, Bit8u* buffer, Bit32u len);
#endif
#endif
private:
    float beep_frequency;  // 0 : beep is off
    unsigned output_mode;
#ifdef __linux__
    /* Do we have access?  If not, just skip everything else. */
    signed int consolefd;
    const static unsigned int clock_tick_rate = 1193180;
#elif defined(WIN32)
    Bit64u usec_start;
#endif
#if BX_SUPPORT_SOUNDLOW
    bx_soundlow_waveout_c* waveout;
    int beep_callback_id;
    bool beep_active;
    Bit16s beep_level;
    Bit8u beep_volume;
#if BX_HAVE_REALTIME_USEC
    bool dsp_active;
    Bit64u dsp_start_usec;
    Bit64u dsp_cb_usec;
    Bit32u dsp_count;
    Bit64u* dsp_event_buffer;
#endif
#endif
};