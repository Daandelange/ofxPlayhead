#include "ofxImGuiHelpers.h"

#ifdef ofxAddons_ENABLE_IMGUI
#include "imgui_internal.h" // ArrowButtonEx

// ImGui namespace extensions
namespace ImGuiEx {
    // Help marker - From imgui_demo.cpp
    bool BeginHelpMarker(const char* marker = "[?]"){
        ImGui::SameLine();
        ImGui::TextDisabled("%s", marker);
        if (ImGui::IsItemHovered()) {
            if (ImGui::BeginTooltip()) {
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                return true;
            }
        }
        return false;
    }

    void ShowHelpMarker(const char* desc){
        if (BeginHelpMarker()){
            ImGui::TextUnformatted(desc);
            EndHelpMarker();
        }
    }

    void EndHelpMarker(){
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    bool ButtonActive(const char* id, bool isActive){
        if(isActive){
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_TabSelected));
        }
        bool ret = ImGui::Button(id);
        if(isActive){
            ImGui::PopStyleColor();
        }
        return ret;
    }

    // Button Pair
    ImGuiDir ButtonPair(ImGuiDir dir1, ImGuiDir dir2){
        ImGuiDir result = ImGuiDir_None;
        const float size = ImGui::GetFrameHeight();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0,0});
        if(ImGui::ArrowButtonEx("##buttonpair1", dir1, {size*0.9f,size})){
            result = dir1;
        }
        ImGui::SameLine();
        if(ImGui::ArrowButtonEx("##buttonpair2", dir2, {size*0.9f,size})){
            result = dir2;
        }
        ImGui::PopStyleVar();
        return result;
    }
}
#endif
