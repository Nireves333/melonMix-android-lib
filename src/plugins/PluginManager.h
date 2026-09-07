#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "PluginDefault.h"

namespace Plugins
{

class PluginManager
{
public:
    static Plugin* load(u32 gameCode);
    // [KHMM] true when a real (non-default) plugin exists for this gamecode — used by the
    // Android frontend to decide whether the single-screen layout applies to the loaded ROM.
    static bool isSupported(u32 gameCode);
};
}

#endif
