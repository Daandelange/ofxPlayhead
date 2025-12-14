#pragma once

#ifdef ofxAddons_ENABLE_IMGUI

#define ofxPHTL_Ramp_Hist_Size 120

#include "imgui.h"


// ImGui Extensions
namespace ImGuiEx {
namespace ofxPH {

    // Helpmarkers (similar to ImGuiDemo code
    void ShowHelpMarker(const char* desc);
    bool BeginHelpMarker(const char* marker); // Call EndHelpMarker() if true, after submitting toltip content.
    void EndHelpMarker();
    bool ButtonActive(const char* id, bool isActive = false);

    // Updown / LeftRight button set
    ImGuiDir ButtonPair(ImGuiDir dir1=ImGuiDir_Up, ImGuiDir dir2=ImGuiDir_Down);

    // - - - - - - - - - -

    void PlotRampHistory( float (&historyEntry)[ofxPHTL_Ramp_Hist_Size], const float& newValue, const ImVec2& graphPos, const ImVec2& graphSize, bool bUpdateHist=true);
    void RampGraph(const char* name, float (&historyEntry)[ofxPHTL_Ramp_Hist_Size], const float& newValue, bool bUpdateHist=true);


}
}

#endif
