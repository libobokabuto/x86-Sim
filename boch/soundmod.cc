#include "bochs.h"
#include "plugin.h"
#include "siminterface.h"
#include "param_names.h"

#if BX_SUPPORT_SOUNDLOW

#include "soundmod.h"
#include "soundlow.h"

#if BX_WITH_SDL || BX_WITH_SDL2
#include <SDL.h>
#endif

#define LOG_THIS bx_soundmod_ctl.

bx_soundmod_ctl_c bx_soundmod_ctl;

const char** sound_driver_names;

bx_soundmod_ctl_c::bx_soundmod_ctl_c()
{
	//put("soundctl", "SNDCTL");
}

bx_soundmod_ctl_c::~bx_soundmod_ctl_c()
{
	free(sound_driver_names);
}

void bx_soundmod_ctl_c::init()
{
    Bit8u i, count = 0;

    count = PLUG_get_plugins_count(PLUGTYPE_SND);
    sound_driver_names = (const char**)malloc((count + 1) * sizeof(char*));
    for (i = 0; i < count; i++) {
        sound_driver_names[i] = PLUG_get_plugin_name(PLUGTYPE_SND, i);
    }
    sound_driver_names[count] = NULL;
    // move 'dummy' module to the top of the list
    if (strcmp(sound_driver_names[0], "dummy")) {
        for (i = 1; i < count; i++) {
            if (!strcmp(sound_driver_names[i], "dummy")) {
                sound_driver_names[i] = sound_driver_names[0];
                sound_driver_names[0] = "dummy";
                break;
            }
        }
    }
}

void bx_soundmod_ctl_c::exit()
{
    bx_sound_lowlevel_c::cleanup();
}

bx_sound_lowlevel_c* bx_soundmod_ctl_c::get_driver(const char* modname)
{
    if (!bx_sound_lowlevel_c::module_present(modname)) {
#if BX_PLUGINS
        PLUG_load_plugin_var(modname, PLUGTYPE_SND);
#else
        //BX_PANIC(("could not find sound driver '%s'", modname));
#endif
    }
    return bx_sound_lowlevel_c::get_module(modname);
}

bx_soundlow_waveout_c* bx_soundmod_ctl_c::get_waveout(bool using_file)
{
    bx_sound_lowlevel_c* module = NULL;

    if (!using_file) {
#if defined(WIN32)
        module = get_driver("win");
#else
        module = get_driver("dummy");
#endif
    }
    else {
        module = get_driver("file");
    }

    if (module != NULL) {
        return module->get_waveout();
    }
    else {
        return NULL;
    }
}

#endif