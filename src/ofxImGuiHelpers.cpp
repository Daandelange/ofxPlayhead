#include "ofxImGuiHelpers.h"

#ifdef ofxAddons_ENABLE_IMGUI
#include "imgui_internal.h" // ArrowButtonEx

// ImGui namespace extensions
namespace ImGuiEx {
namespace ofxPH {
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

    void PlotRampHistory( float (&historyEntry)[ofxPHTL_Ramp_Hist_Size], const float& newValue, const ImVec2& graphPos, const ImVec2& graphSize, bool bUpdateHist){
        float graphStep = graphSize.x / ofxPHTL_Ramp_Hist_Size;

        // Sync :
        if(bUpdateHist){
            for(unsigned int i=0; i < ofxPHTL_Ramp_Hist_Size-1; i++){
                historyEntry[i] = historyEntry[i+1];
            }
            historyEntry[ofxPHTL_Ramp_Hist_Size-1] = newValue;
        }

        // Draw
        ImVec2 prevPos;
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        ImColor graphCol = ImGui::GetColorU32(ImGuiCol_Text);
        for(unsigned int gi=0; gi<ofxPHTL_Ramp_Hist_Size; gi++){
            ImVec2 newPos = graphPos+ImVec2(gi*graphStep, graphSize.y - (historyEntry[gi]*graphSize.y));
            //wdl->AddCircleFilled(newPos, 3, IM_COL32(255,0,0,255));
            if(gi > 0) wdl->AddLine(prevPos, newPos, graphCol);

            prevPos = newPos;
        }
    }

    void RampGraph(const char* name, float (&historyEntry)[ofxPHTL_Ramp_Hist_Size], const float& newValue, bool bUpdateHist){
        ImColor graphColBorder = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        float graphPosX = 200; // Space for text

        ImVec2 graphSize = ImGui::GetContentRegionAvail();
        graphSize.x -= graphPosX;
        graphSize.y = ImGui::GetFrameHeight()*2.f;

        //wdl->AddRect(graphPos, graphPos+graphSize, IM_COL32(255,0,0,255));

        ImVec2 graphPos = ImGui::GetCursorScreenPos();
        graphPos.x += graphPosX;
        ImGui::Text(name, newValue);
        ImGui::SameLine();
        ImGui::Dummy(graphSize);

        static bool bPauseRamps = false;
        if(ImGui::IsItemClicked(ImGuiMouseButton_Left)){
            bPauseRamps = !bPauseRamps;
        }

        // Border
        ImGui::GetWindowDrawList()->AddRect(graphPos, graphPos+graphSize, graphColBorder);

        //  plot...
        PlotRampHistory(historyEntry, newValue, graphPos, graphSize, bUpdateHist && !bPauseRamps);
    }
}
}
#endif
