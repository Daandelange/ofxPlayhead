#pragma once

#ifdef ofxAddons_ENABLE_IMGUI

#include "imgui.h"


// ImGui Extensions
namespace ImGuiEx {

    // Helpmarkers (similar to ImGuiDemo code
    void ShowHelpMarker(const char* desc);
    bool BeginHelpMarker(const char* marker); // Call EndHelpMarker() if true, after submitting toltip content.
    void EndHelpMarker();
    bool ButtonActive(const char* id, bool isActive = false);

    // Updown / LeftRight button set
    ImGuiDir ButtonPair(ImGuiDir dir1=ImGuiDir_Up, ImGuiDir dir2=ImGuiDir_Down);

}
#endif
