#pragma once
class bx_sound_lowlevel_c;
class bx_soundlow_waveout_c;
class bx_soundlow_wavein_c;
class bx_soundlow_midiout_c;

class BOCHSAPI bx_soundmod_ctl_c {
public:
	bx_soundmod_ctl_c();
	~bx_soundmod_ctl_c();
	void init(void);
	bx_soundlow_waveout_c* get_waveout(bool using_file);
private:
	bx_sound_lowlevel_c* get_driver(const char* modname);
};

BOCHSAPI extern bx_soundmod_ctl_c bx_soundmod_ctl;