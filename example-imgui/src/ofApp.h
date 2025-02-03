#pragma once

#include "ofMain.h"
#include "ofxPlayhead.h"

#ifdef ofxAddons_ENABLE_IMGUI
#include "ofxImGui.h"
#endif


class ofApp : public ofBaseApp{

	public:
		void setup();
		void exit();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		// Event listeners
		void onTimelineRestart(const std::size_t& _loopCount);
		void onTimelineStop();
		void onTimelinePause(const bool& _paused);
		void onFrameTick(const ofxPHTimeCounters& _counters);
		void onUpdateTick(const ofxPHTimeCounters& _counters);
		void onTimelineSeek(const ofxPHTimeCounters& _counters);
		
#ifdef ofxAddons_ENABLE_IMGUI
		ofxImGui::Gui gui;
#endif
		ofxPlayhead playhead;
};
