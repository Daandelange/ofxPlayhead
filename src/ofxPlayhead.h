#pragma once

#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>

#include "ofEvent.h"

#ifdef ofxAddons_ENABLE_IMGUI
#include "ofxImGui.h"
#endif

#define ofxPH_XML_ENGINE_PUGIXML 1
#define ofxPH_XML_ENGINE ofxPH_XML_ENGINE_PUGIXML
//#define ofxPH_TIMELINE_SINGLETON

#if ofxPH_XML_ENGINE == ofxPH_XML_ENGINE_PUGIXML
#include "pugixml.hpp"
#endif

// - - - - - - - - - -

// Interesting reads :
// - https://docs.unity3d.com/Manual/TimeFrameManagement.html
// - https://gafferongames.com/post/fix_your_timestep/
// - Framedrop player, realtime player : https://github.com/monadgroup/re19/blob/80989ebf8ae2a3e203a443e583a7f359f0114333/player/src/realtime_player.rs#L209

// ofxAddon candidates // Related projects :
// - https://github.com/azuremous/ofxSimpleMetronome: Quite similar without the ramps, and event-based.
// - https://github.com/sfjmt/ofxSimpleTimer/: Quite similar without the ramps.
// - https://github.com/cas2void/ofxDawMetro : A pure DAW metro, no timeline.
// - https://github.com/sevagh/libmetro : A C++ metronome class, to be used within this class ? (facilitates the maths + exactness ?)
// - https://github.com/bakercp/ofxPlayer/ : A player for varius media.

// Todo: rt-Abs mode changes start_time to move the time cursor manually, it could use paused_time instead not to change the start_time.
// Todo: Some `counters.frameCount++;` are probably missing at some places
// Todo: Remove `std::chrono::duration<double>` usage in favour of `std::chrono::duration<long long, std::nano>`
// Todo: Ability to move+zoom in the timeline view
// Todo: Support for infinite duration
// Todo: Rename ofxPH... types to ofxPH namespace types.
// Todo: Make a testbed with values graphing/plotting values to verify exectness.
// A helper to bind an ofApp automatically to events

enum ofxPlayheadMode {
    ofxPlayheadMode_RealTime_Relative,
    ofxPlayheadMode_RealTime_Absolute,
    ofxPlayheadMode_Offline,
};

enum ofxPlayheadLoopMode {
    NoLoop,
    LoopOnce,
    LoopInfinite
};

// - - - - - - - - - -

// A struct representing a time signature
struct ofxPHTimeSignature {
    ofxPHTimeSignature(unsigned int _bars, unsigned int _noteValue, unsigned int _bpm=120);

    double getBeatDurationSecs() const;
    double getNoteDurationSecs() const;
    double getBarDurationSecs() const;

    void set(unsigned int _bars, unsigned int _noteValue, unsigned int _bpm=0);

public:
    unsigned int bpm;
    unsigned int beatsPerBar; // aka. Measure
    unsigned int notesPerBeat; // aka. NoteValue

    struct {
        double bar;
        double note;
        double beat;
    } durations;

    //unsigned int beatsPerMeasure;
    void updateValues();
};

// Struct with all time counters available
struct ofxPHTimeCounters {
    friend class ofxPlayhead;

    std::size_t frameCount; // Number of rendered frames
    std::size_t frameNum; // Theoretical frame number
    std::size_t loopCount;
    std::size_t beatCount;
    std::size_t barCount;
    std::size_t noteCount;
    double playhead; // In seconds
    double tDelta; // Elapsed seconds since last frame. Experimental
    double tDeltaSeek; // Elapsed seconds while seeking. Experimental
    double progress;
    const double& elapsedSeconds(){ return playhead; } // Alias

    private:
    int frameNumSpeed; // Used to track offline frames with the speed parameter
};

// - - - - - - - - - -

struct ofxPHTimeRamps {
    friend class ofxPlayhead;

    // Animation loop
    //double loopProgress;

    // Bars
    float barElapsed; // bar counter
    float barProgress; // (linearly 0 to 1 each bar)
    float barPulse; // (a very short and smoothed impulse on each bar start)
    float barStep; //  Bar number, from 0 to 1
    //double barRandom; //

    // Beats
    float beatsElapsed; // beats counter
    float beatProgress; // (linearly 0 to 1 on each beat)
    float beatPulse; // (a very short and smoothed impulse on each beat start)
    float beatStep; //  Beat number, from 0 to 1
    //double beatRandom;

    // Notes
    float noteElapsed; // note counter
    float noteProgress; // (linearly 0 to 1 on each note)
    float notePulse; // (a very short and smoothed impulse on each note start)
    float noteStep; //  Note number, from 0 to 1
    //double noteRandom;


protected:
    ofxPHTimeRamps();

    void initializeRamps();
    void updateRamps(double elapsedSeconds, const ofxPHTimeSignature& _ts, const double& _duration);
};

// - - - - - - - - - -
#ifdef ofxAddons_ENABLE_IMGUI
#define ofxPHTL_Ramp_Hist_Size 120
namespace ImGuiEx{
    void PlotRampHistory( float (&historyEntry)[ofxPHTL_Ramp_Hist_Size], const float& newValue, const ImVec2& graphPos, const ImVec2& graphSize, bool bUpdateHist=true);
    void RampGraph(const char* name, float (&historyEntry)[ofxPHTL_Ramp_Hist_Size], const float& newValue, bool bUpdateHist=true);
}
#endif

// - - - - - - - - - -

// A clock class with playhead functionality and some accessory ramps
// That makes up a timeline
class ofxPlayhead {
#ifdef ofxPH_TIMELINE_SINGLETON
    private:
#else
    public:
#endif
        ofxPlayhead(unsigned int _fps=60, double _duration = 60, ofxPlayheadLoopMode _loopMode = ofxPlayheadLoopMode::NoLoop, unsigned int _beatsPerBar = 4, unsigned int _notesPerBeat = 4, int _beatsPerMinute = 120);

#ifdef ofxPH_TIMELINE_SINGLETON
    public:
        //ofxPlayhead(const ofxPlayhead&) = delete; // not assignable
        ofxPlayhead(ofxPlayhead&&) = delete; // not moveable
        static ofxPlayhead& getTimeline(){
            static ofxPlayhead tl;
            return tl;
        }
#endif
    public:
        // Copy assignment. Use with caution! Can conflict with singleton usage creating a copy.
        ofxPlayhead(const ofxPlayhead& _other) :
            fps(_other.fps),
            duration(_other.duration),
            timeSignature(_other.timeSignature),
            loopMode(_other.loopMode),
            playSpeed(1.0),
            playbackMode(ofxPlayheadMode::ofxPlayheadMode_Offline)
        {
            reset();
            // go to same frame as other ?
            if(_other.isPlaying()){
                goToFrame(_other.getFrameNum());
            }
        }

    //void setup(unsigned int _fps=60, double _duration = 60, ofxPlayheadLoopMode _loopMode = ofxPlayheadLoopMode::NoLoop, unsigned int _beatsPerBar = 4, unsigned int _notesPerBeat = 4, int _beatsPerMinute = 120);

    // Starts the timeline (aka. play())
    void start(unsigned int _offset=0);
    void startNextFrame(unsigned int _offset=0);

    // Function to pause the timeline
    void pause();
    bool isPaused();
    bool isRunning();

    // Function to resume the timeline
    void resume();
    void stop();
    void togglePause();

    // Frame-by-frame, relative
    void nextFrame(int _direction = 1);

    void goToFrame(int _frame, bool _relative=false);
    bool goToSeconds(double _seconds, bool _relative=false);

    void setLoop(ofxPlayheadLoopMode loopMode);
    void setDuration(double duration);

    void setDurationFromBeats(int bars, int noteValue);
    bool setPlaySpeed(double playSpeed);

    double getPlaySpeed() const;

    // Optional, to update realtime modes in-between frames
    void tickUpdate();

    // At the begining of your frame, tick this. Todo: after frame so update() can prapare data for the next frame ?
    void tickFrame();

private:
    void tickPlayHead(const bool isNewFrame = true);

    // Updates playhead for looping
    void checkLoops();

    // Updates internals and ramps according to playhead
    void updateInternals();

    // Note: Private bcoz last arg is for internal use only !
    void goToFrame(int _frame, bool _relative, bool _doPause);
    bool goToSeconds(double _seconds, bool _relative, bool _doPause);

public:
    // todo: make private
    //void tick();

    double getElapsedSeconds() const;

    unsigned int getFrameNum() const;

    // Returns the number of rendered frames
    unsigned int getElapsedFrames() const;

    // Returns the number of frames if the composition is finite; 0 otherwise.
    unsigned int getTotalFrames() const;

    // Returns the number of bars if the composition is finite; 0 otherwise.
    unsigned int getTotalBars() const;

    // Returns the number of notes if the composition is finite; 0 otherwise.
    unsigned int getTotalNotes() const;

    double getDuration() const;

    // Playhead position
    double getProgress() const;

    // FPS
    unsigned int getFps() const;
    void setFps(unsigned int _fps);

    const ofxPHTimeSignature& getTimeSignature() const;
    const ofxPHTimeCounters& getCounters() const;
    const ofxPHTimeRamps& getRamps() const;

//    double getCurrentBeat() const {
//        double beatsElapsed = getElapsedSeconds() * timeSignature.bpm / 60.0;
//        double beatsPerFrame = static_cast<double>(fps) / 60.0;
//        return std::fmod(beatsElapsed, timeSignature.notesPerBeat) + 1.0; // Beats are 1-indexed
//    }

    // Utils / getters
    bool isPlaying() const;
    ofxPlayheadMode getPlayMode() const;
    bool setPlayMode(ofxPlayheadMode _mode);

    // Keyboard shortcut helpers (optional)
    void registerKeyboardShortcuts();
    void removeKeyboardShortcuts();
    void keyPressed( ofKeyEventArgs & key );


    // ImGui Helpers
#ifdef ofxAddons_ENABLE_IMGUI
    void drawImGuiFullWidget(bool horizontalLayout = true);
    void drawImGuiSettings(bool horizontalLayout = true);
    void drawImGuiRamps();
    void drawImGuiMetronom(bool horizontalLayout = true);
    void drawImGuiTimers(bool horizontalLayout = true);
    void drawImGuiTimeline(bool horizontalLayout = true);
    void drawImGuiPlayHead(bool horizontalLayout = true);
    void drawImGuiPlayControls(bool horizontalLayout = true);
    void drawImGuiWindow(bool* p_open = nullptr);
    static const double speedMin, speedMax;
#endif

#if ofxPH_XML_ENGINE == ofxPH_XML_ENGINE_PUGIXML
	bool populateXmlNode(pugi::xml_node& _node);
	bool retrieveXmlNode(pugi::xml_node& _node);
#endif

	// Events you can listen to !
	ofEvent<const std::size_t> onStart; // Argument = current loop count (0 on restart).
	ofEvent<const ofxPHTimeCounters> onUpdateTick; // Argument = elapserSeconds.
	ofEvent<const ofxPHTimeCounters> onFrameTick; // Argument = elapserSeconds.
	ofEvent<const bool> onPause; // Argument = paused=1, resumed=0
	ofEvent<const ofxPHTimeCounters> onSeek;
	ofEvent<void> onStop;
private:
    // Composition Settings
    unsigned int fps; // Desired framerate (realtime) or Exact framerate (offline)
    double duration;
    ofxPHTimeSignature timeSignature;
    ofxPHTimeRamps timeRamps;
    bool bAllowLossyOperations = true; // Allows changing speed and playmode while the timer is running

    // Playback
    bool playing;
    bool paused;
    ofxPlayheadLoopMode loopMode;
    double playSpeed;
    ofxPlayheadMode playbackMode;
    bool bStartNextFrame = false;
    unsigned int startNextFrameOffet=0u;

    // Counters
    ofxPHTimeCounters counters;

    // Internals
    void resetPausedDuration();
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point last_frame_time;
    std::chrono::high_resolution_clock::time_point paused_time;
    std::chrono::duration<long long, std::nano> paused_duration;
    double seekedTime = 0.0;
    long rtAbsLoopCount = 0;
    bool bIgnoreFirstReverseLoop = true;
//    std::chrono::duration<double> paused_duration;
//    std::chrono::duration<double> pausedTimes;

    //bool loop_complete;


    void reset();
};

//int main() {
//    Timeline timeline(24, 10.0, LoopMode::LoopInfinite, 4, 120);  // Set your desired parameters

//    timeline.setPlaySpeed(0.5);  // Set the play speed (0.5 for half speed, 2.0 for double speed, etc.)

//    timeline.start();

//    // Let the timeline play for a while
//    for (int i = 0; i < 120; ++i) {
//        timeline.tick();
//        std::cout << "Elapsed Time: " << timeline.getElapsedSeconds()
//                  << " seconds, Current Frame: " << timeline.getCurrentFrame()
//                  << ", Current Beat: " << timeline.getCurrentBeat() << std::endl;

//        // Simulate rendering or processing work
//        // std::this_thread::sleep_for(std::chrono::milliseconds(50));
//    }

//    // Pause the timeline
//    timeline.pause();

//    // Resume after a while
//    for (int i = 0; i < 60; ++i) {
//        timeline.tick();
//        std::cout << "Elapsed Time: " << timeline.getElapsedSeconds()
//                  << " seconds, Current Frame: " << timeline.getCurrentFrame()
//                  << ", Current Beat: " << timeline.getCurrentBeat() << std::endl;
//    }

//    // Resume and let it play to completion
//    timeline.start();
//    while (timeline.isPlaying()) {
//        timeline.tick();
//        std::cout << "Elapsed Time: " << timeline.getElapsedSeconds()
//                  << " seconds, Current Frame: " << timeline.getCurrentFrame()
//                  << ", Current Beat: " << timeline.getCurrentBeat() << std::endl;

//        // Simulate rendering or processing work
//        // std::this_thread::sleep_for(std::chrono::milliseconds(50));
//    }

//    return 0;
//}


//int main() {
//    Timeline timeline(24, 10.0, true, 4, 120);  // Set your desired parameters

//    timeline.start();

//    // Let the timeline play for a while
//    for (int i = 0; i < 120; ++i) {
//        timeline.tick();
//        std::cout << "Elapsed Time: " << timeline.getElapsedSeconds()
//                  << " seconds, Current Frame: " << timeline.getCurrentFrame()
//                  << ", Current Beat: " << timeline.getCurrentBeat() << std::endl;

//        // Simulate rendering or processing work
//        // std::this_thread::sleep_for(std::chrono::milliseconds(50));
//    }

//    // Pause the timeline
//    timeline.pause();

//    // Resume after a while
//    for (int i = 0; i < 60; ++i) {
//        timeline.tick();
//        std::cout << "Elapsed Time: " << timeline.getElapsedSeconds()
//                  << " seconds, Current Frame: " << timeline.getCurrentFrame()
//                  << ", Current Beat: " << timeline.getCurrentBeat() << std::endl;
//    }

//    // Resume and let it play to completion
//    timeline.start();
//    while (timeline.isPlaying()) {
//        timeline.tick();
//        std::cout << "Elapsed Time: " << timeline.getElapsedSeconds()
//                  << " seconds, Current Frame: " << timeline.getCurrentFrame()
//                  << ", Current Beat: " << timeline.getCurrentBeat() << std::endl;

//        // Simulate rendering or processing work
//        // std::this_thread::sleep_for(std::chrono::milliseconds(50));
//    }

//    return 0;
//}

