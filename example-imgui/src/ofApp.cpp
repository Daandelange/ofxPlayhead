#include "ofApp.h"
#include "ofMath.h" // glm inclusions

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);

#ifdef ofxAddons_ENABLE_IMGUI
    gui.setup();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    // Configure / override defaults
    playhead.setDuration(60.0);
    playhead.setFps(ofGetTargetFrameRate());
    playhead.setLoop(ofxPlayheadLoopMode::LoopInfinite);
    playhead.startNextFrame();

    // Bind event notifications
    ofAddListener(playhead.onStart, this, &ofApp::onTimelineRestart);
    ofAddListener(playhead.onStop, this, &ofApp::onTimelineStop);
    ofAddListener(playhead.onPause, this, &ofApp::onTimelinePause);
    ofAddListener(playhead.onFrameTick, this, &ofApp::onFrameTick);
    ofAddListener(playhead.onUpdateTick, this, &ofApp::onUpdateTick);
    ofAddListener(playhead.onSeek, this, &ofApp::onTimelineSeek);

    // Keyboard shortcuts
    playhead.registerKeyboardShortcuts();

    // PugiXML : Load last used settings
    bool success = true;
    if(ofFile(ofToDataPath("ofxPlayHeadSettings.xml", true)).exists()){
        pugi::xml_document doc;
        pugi::xml_parse_result parseResult = doc.load_file(ofToDataPath("ofxPlayHeadSettings.xml", true).c_str());
        success = parseResult;
        if(parseResult){
            pugi::xml_node rootNode = doc.child("ofxPlayHead");
            if(rootNode){
                success *= playhead.retrieveXmlNode(rootNode);
            }
            else success = false;
        }
    }
    if(!success) {
        ofLogNotice("ofxPlayhead") << "Failed parsing previous settings from data file `" << ofToDataPath("ofxPlayHeadSettings.xml", true) << "`.";
    }
}

void ofApp::exit(){
    // PugiXML : Save timeline settings
    pugi::xml_document doc;
    pugi::xml_node rootNode = doc.append_child("ofxPlayHead");
    bool success = true;
    success *= playhead.populateXmlNode(rootNode);
    if(success) doc.save_file(ofToDataPath("ofxPlayHeadSettings.xml").c_str());

    // Remove listeners
    ofRemoveListener(playhead.onStart, this, &ofApp::onTimelineRestart);
    ofRemoveListener(playhead.onStop, this, &ofApp::onTimelineStop);
    ofRemoveListener(playhead.onPause, this, &ofApp::onTimelinePause);
    ofRemoveListener(playhead.onFrameTick, this, &ofApp::onFrameTick);
    ofRemoveListener(playhead.onUpdateTick, this, &ofApp::onUpdateTick);
    ofRemoveListener(playhead.onSeek, this, &ofApp::onTimelineSeek);
    playhead.removeKeyboardShortcuts();
}

//--------------------------------------------------------------
void ofApp::update(){
    playhead.tickUpdate();
}

//--------------------------------------------------------------
void ofApp::draw(){
    playhead.tickFrame();

    ofSetColor(255);
    ofFill();

    ofRectangle wr = gui.getMainWindowViewportRect();

    const glm::vec2 ws ={wr.getWidth(), wr.getHeight()};
    ofDrawBitmapStringHighlight("Absolute Animation", wr.getTopLeft()+ws*glm::vec2(0.5, 0.3));
    ofDrawCircle(wr.getTopLeft()+ws*glm::vec2(playhead.getProgress(), 0.4), 10);

    static glm::vec2 posSim(0.0, ws.y*0.7);

    posSim.y = ws.y*0.7;
    static bool bUseSeekDelta = true;
    const double totalDelta = playhead.getCounters().tDelta+playhead.getCounters().tDeltaSeek*bUseSeekDelta;
    if(ws.x!=0) posSim.x = glm::mod<double>((posSim.x+(ws.x*totalDelta/playhead.getDuration())), ws.x);
    ofDrawBitmapStringHighlight("Simulation Delta Animation\n(doesn't support seeking)", wr.getTopLeft()+ws*glm::vec2(0.5, 0.6));
    ofDrawCircle(wr.getTopLeft()+posSim, 10);

    ofSetColor(255,0,0,155);
    ofSetLineWidth(3);
    ofDrawLine(wr.getTopLeft()+posSim, wr.getTopLeft()+ws*glm::vec2(playhead.getProgress(), 0.7));

#ifdef ofxAddons_ENABLE_IMGUI
    gui.begin();

    // Docking space
    {
        ImGuiDockNodeFlags dockingFlags = ImGuiDockNodeFlags_PassthruCentralNode; // Make the docking space transparent
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0,0,0,0));
        dockingFlags |= ImGuiDockNodeFlags_NoDockingOverCentralNode;
        ImGuiID dockNodeID = ImGui::DockSpaceOverViewport(0, NULL, dockingFlags); // Also draws the docked windows
        ImGui::PopStyleColor(1);
    }

    // Playhead control window
    playhead.drawImGuiTimelineWindow();

    // Simulation window
    if(ImGui::Begin("Simulation")){
        ImGui::TextWrapped("In this example, the upper circle is an absolute animation while the lower circle is animated by a simulation using tDelta.");
        ImGui::Separator();
        ImGui::Text("Simulation offset = %.3f", posSim.x - (ws.x*playhead.getProgress()));
        if(ImGui::Button("Sync simulation")){
            posSim.x = ws.x*playhead.getProgress();
        }
        ImGui::Checkbox("Use seek delta too", &bUseSeekDelta);

        //playhead.drawImGuiPlayControls();
    }
    ImGui::End();


    gui.end();
    gui.draw();
#endif
}

//--------------------------------------------------------------
void ofApp::onTimelineRestart(const std::size_t& _loopCount){
    std::cout << "onStart(): " << (_loopCount>0?"looped":"restarted") << std::endl;
}
void ofApp::onTimelineStop(){
    std::cout << "onStop()" << std::endl;
}
void ofApp::onTimelinePause(const bool& _paused){
    std::cout << "onPause()" << std::endl;
}
void ofApp::onFrameTick(const ofxPHTimeCounters& _counters){
    // Received each frame
}
void ofApp::onUpdateTick(const ofxPHTimeCounters& _counters){
    // Received each update()
}
void ofApp::onTimelineSeek(const ofxPHTimeCounters& _counters){
    std::cout << "onSeek: seeked " << _counters.tDeltaSeek << " sec." << std::endl;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
