//
// [KHMM] Android port stub for PluginJoystick.
//
// The KH Melon Mix desktop frontend implements PluginJoystick::applyMappings in
// src/frontend/qt_sdl/plugins/PluginJoystick.cpp using SDL2 to enumerate game
// controllers and auto-map their bindings. Android has no SDL and drives input
// through its own controller/touch layer, so controller auto-mapping does not
// apply here — it belongs to the "refined controls" feature chunk, which will
// rebuild input mapping as Android input.
//
// This no-op definition satisfies the linker (KingdomHeartsHDCollection.cpp calls
// applyMappings) without pulling in SDL. Replace with a real Android implementation
// when the controls feature is ported.
//

#include "PluginJoystick.h"

namespace Plugins
{

void PluginJoystick::applyMappings(std::function<void(std::string, int)> setIntConfig, std::map<std::string, std::vector<PluginJoystickInput>> map)
{
    // Intentionally empty on Android — see file header.
}

}
