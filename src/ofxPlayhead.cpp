#include "ofxPlayhead.h"

#ifdef ofxAddons_ENABLE_IMGUI
#include "ofxImGuiHelpers.h"
#endif

#include "ofUtils.h" // ofToString

// - - - - - - - - - -

ofxPHTimeSignature::ofxPHTimeSignature(unsigned int _bars, unsigned int _noteValue, unsigned int _bpm){
    set(_bars, _noteValue, _bpm);
}

double ofxPHTimeSignature::getBeatDurationSecs() const {
    return durations.beat;
}

double ofxPHTimeSignature::getNoteDurationSecs() const {
    return durations.note;
}

double ofxPHTimeSignature::getBarDurationSecs() const {
    return durations.bar;
}

void ofxPHTimeSignature::set(unsigned int _bars, unsigned int _noteValue, unsigned int _bpm){
    if(_bpm==0) _bpm = bpm; // Don't change by default
    beatsPerBar = _bars;
    notesPerBeat = _noteValue;
    bpm = _bpm;
    updateValues();
}

void ofxPHTimeSignature::updateValues(){
    durations.beat = 60.f/bpm;
    durations.bar = getBeatDurationSecs()*beatsPerBar;
    durations.note = getBeatDurationSecs()/notesPerBeat;
}

// - - - - - - - - - -
ofxPHTimeRamps::ofxPHTimeRamps() {
    initializeRamps();
}

void ofxPHTimeRamps::initializeRamps() {
    barProgress = 0.0;
    barPulse = 0.0;
    barStep = 0.0;
    beatProgress = 0.0;
    beatPulse = 0.0;
    beatStep = 0.0;
    noteProgress = 0.0;
    notePulse = 0.0;
    noteStep = 0.0;
}

void ofxPHTimeRamps::updateRamps(double elapsedSeconds, const ofxPHTimeSignature& _ts, const double& _duration) {

    // Todo: align playhead on FPS frames before (optionally?), to ensure reaching 0 and 1 in-between frames ???
    // (FPS aren't aligned on the BPM grid)

    // Bars
    double secondsPerBar = _ts.getBarDurationSecs();//60.0 / _ts.bpm * _ts.beatsPerBar;
    barElapsed = elapsedSeconds / secondsPerBar;
    barProgress = std::fmod(barElapsed, 1.0);
    barPulse = (barProgress < 0.04 || barProgress >= 1.0) ? 1.0 : 0.0;
    //const float totalBars = _ts.bpm / _ts.beatsPerBar; // In minutes; todo: could be nice to use track length instead ?
    const float totalBars = _duration > secondsPerBar ? (_duration / secondsPerBar) : (_ts.bpm / _ts.beatsPerBar); // Note: Infinite duration falls back on 1 min
    if(totalBars > 1) barStep = std::fmod( glm::floor(elapsedSeconds / secondsPerBar), totalBars)/(totalBars-1);
    else barStep = 0;

    // Beats
    double secondsPerBeat = _ts.getBeatDurationSecs();//60.0 / _ts.bpm;
    //double beatTime = std::fmod(elapsedSeconds / secondsPerBeat, 1.0);
    beatsElapsed = elapsedSeconds / secondsPerBeat;
    beatProgress = std::fmod(beatsElapsed, 1.0);//beatTime;// / secondsPerBar;
    beatPulse = (beatProgress < 0.01 || beatProgress > 0.99) ? 1.0 : 0.0;
    if(_ts.beatsPerBar > 1) beatStep = std::floor(std::fmod(elapsedSeconds / secondsPerBeat, _ts.beatsPerBar))/(_ts.beatsPerBar-1);//beatProgress * _ts.beatsPerBar;
    else beatStep = 0;

    // Notes
    double secondsPerNote = _ts.getNoteDurationSecs();//secondsPerBeat / _ts.notesPerBeat;
    //double noteTime = std::fmod(elapsedSeconds/ secondsPerNote, 1.0);
    noteElapsed = elapsedSeconds/ secondsPerNote;
    noteProgress = std::fmod(noteElapsed, 1.0);//noteTime / secondsPerNote;
    notePulse = (noteProgress < 0.01 || noteProgress > 0.99) ? 1.0 : 0.0;
    if(_ts.notesPerBeat > 1) noteStep = std::floor(std::fmod(elapsedSeconds / secondsPerNote, _ts.notesPerBeat))/(_ts.notesPerBeat-1);//noteProgress * _ts.notesPerBeat;
    else noteStep = 0;
}

// - - - - - - - - - -
#ifdef ofxAddons_ENABLE_IMGUI
namespace ImGuiEx{
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
#endif

// - - - - - - - - - -

ofxPlayhead::ofxPlayhead(unsigned int _fps, double _duration, ofxPlayheadLoopMode _loopMode, unsigned int _beatsPerBar, unsigned int _notesPerBeat, int _beatsPerMinute)
    : fps(_fps),
      duration(_duration),
      timeSignature(_beatsPerBar, _notesPerBeat, _beatsPerMinute),
      loopMode(_loopMode),
      playSpeed(1.0),
      playbackMode(ofxPlayheadMode::ofxPlayheadMode_Offline)
{
    reset();
}

const ofxPHTimeSignature& ofxPlayhead::getTimeSignature() const {
    return timeSignature;
}

const ofxPHTimeCounters& ofxPlayhead::getCounters() const {
    return counters;
}
const ofxPHTimeRamps& ofxPlayhead::getRamps() const {
    return timeRamps;
}

void ofxPlayhead::start(unsigned int _offset) {
    reset();
    if(_offset>0) goToFrame(_offset, false, false);
    playing = true;
    //std::cout << "Timeline started." << std::endl;

    onStart.notify(counters.loopCount);
}

void ofxPlayhead::startNextFrame(unsigned int _offset){
    bStartNextFrame = true;
    startNextFrameOffet = _offset;
}

// Function to pause the timeline
void ofxPlayhead::pause() {
    if (!paused) {
        if(!playing) start(); // Ensure play is on

        paused = true;
        last_frame_time = std::chrono::high_resolution_clock::now();
        paused_time = last_frame_time;
        onPause.notify(paused);
    }
}

bool ofxPlayhead::isPaused(){
    return paused;
}

bool ofxPlayhead::isRunning(){
    return playing && !paused;
}

// Function to resume the timeline
void ofxPlayhead::resume() {
    if (playing && paused) {
        paused = false;
        last_frame_time = std::chrono::high_resolution_clock::now();
        paused_duration += last_frame_time - paused_time;
        onPause.notify(paused);
    }
}

void ofxPlayhead::stop() {
    reset();
    onStop.notify();
}

void ofxPlayhead::togglePause() {
    if(!playing) start();
    else if(paused) resume();
    else pause();
}

// Frame-by-frame, relative
void ofxPlayhead::nextFrame(int _direction){
    // Only when paused or speed = 0
    if(playing && paused && _direction!=0){
        // Only when paused or speed = 0
        if(playSpeed==0 || paused){
            goToFrame(_direction , true);
    //                //auto current_time = playhead;//std::chrono::duration<double>(playhead - last_frame_time);//playhead;//std::chrono::high_resolution_clock::now();
    //                double frame_time = 1.0 / fps;

    //                // Update playhead
    //                if (playbackMode == ofxPlayheadMode_RealTime_Absolute || playbackMode == ofxPlayheadMode_RealTime_Relative) {
    //                    playhead += frame_time * _direction;
    //                }
    //                else /*if (playbackMode == ofxPlayheadMode_Offline)*/ {
    //                    playhead = frame_time * ( frameNum + _direction);
    //                }
    //                //auto tmp = std::chrono::seconds((long long)playhead);

    //                // Incremment rendered frames
    //                frameCount++;

    //                // Remember (for rt modes)
    ////                last_frame_time = start_time + std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>(playhead);
    //                last_frame_time = start_time + std::chrono::seconds((long long)playhead);

    //                checkLoops();

    //                updateInternals();
        }
    }
}

void ofxPlayhead::goToFrame(int _frame, bool _relative){
    goToFrame(_frame, _relative, true);
}

void ofxPlayhead::goToFrame(int _frame, bool _relative, bool _doPause){
    // Enable play, stay paused (allows setting frame)
    if(!playing && _doPause){
        pause();
    }
    // Set frame
    {
        //std::cout << "goToFrame("<< _frame << ", " << _relative << ")";
        // Sanitize args
        if(_relative){
            _frame = _frame + counters.frameNum;
        }
        //std::cout << " absFrame="<< _frame << " totalFrames="<< duration<<"x"<<fps<<"="<< getTotalFrames();
        // Do not exceed duration
        _frame = _frame % (int)getTotalFrames();
        //std::cout << " frame=" << _frame;
        // Negative frame indicates distance from end
        if(_frame<0) _frame = getTotalFrames() + _frame;

        double frame_time = 1.0 / fps;
        //std::cout << " final_frame=" << _frame << std::endl;

        goToSeconds(frame_time * _frame, false);
        return;

        // OLD CODE BELOW
//        auto current_time = std::chrono::high_resolution_clock::now();

//        // Update playhead
//        // todo: this section below is fucked up !
//        if (playbackMode == ofxPlayheadMode_RealTime_Absolute) {
//            // Absolute: Update absolutely
////            if(_relative) counters.playhead += frame_time * _frame;
////            else counters.playhead = frame_time * _frame;

//            auto startDiff = current_time - start_time;
//            start_time += startDiff;
//            //paused_duration = std::chrono::duration<long long, std::nano>(0);

//            //last_frame_time = current_time;//start_time + std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0));
//            if(paused) paused_time = current_time;
//        }
//        else if(playbackMode == ofxPlayheadMode_RealTime_Relative) {
//            // Relative : Update relatively
//            if(_relative) counters.playhead += frame_time * _frame;
//            else counters.playhead = frame_time * _frame;


////            last_frame_time = current_time;//start_time + std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0));
////            if(paused) paused_time = current_time;
//        }
//        else {
//            // Update statically
//            if(_relative) counters.playhead = frame_time * ( counters.frameNum + _frame);
//            else counters.playhead = frame_time * _frame;
//        }

//        // Incremment rendered frames
//        counters.frameCount++;

//        // Remember (for rt modes)
//        // Fixme : be nice with rtAbs ?
//        last_frame_time = current_time;//start_time + std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0));
////        if(paused) paused_time = current_time;

//        checkLoops();

//        updateInternals();

//        onSeek.notify(counters);
    }
}

bool ofxPlayhead::goToSeconds(double _seconds, bool _relative){
    return goToSeconds(_seconds, _relative, true);
}

bool ofxPlayhead::goToSeconds(double _seconds, bool _relative, bool _doPause){
    // Ensure play is on
    if(!playing && _doPause){
        pause();
    }

    // Set seconds
    if( _seconds >= 0 && _seconds <= duration){
        // Backup for diff calc
        const double origPos = counters.playhead;
        //std::cout << "OrigSeekPos=" << counters.playhead << std::endl;

        if(_relative){
            counters.playhead += _seconds;
        }
        else {
            counters.playhead = _seconds;
        }

        resetPausedDuration();

        auto current_time = std::chrono::high_resolution_clock::now();
        //start_time = current_time - std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0/playSpeed));
        //auto seekTime = current_time-last_frame_time;
        //seekedTime = seekTime.count()*0.00001;
        //seekedTime += std::chrono::duration<double>(seekTime).count();// * playSpeed;
        seekedTime += counters.playhead - origPos;
        //std::cout << "seekedTime = " << seekedTime << ", ph=" << counters.playhead << ", ph_o=" << origPos << " // " << _seconds << std::endl;
        //if(counters.playhead == origPos) std::cout << "No difference !" << std::endl;
        last_frame_time = current_time;

        // Ensure correct behaviour on resume
        if(paused) paused_time = current_time;

        // Incremment rendered frames
        counters.frameCount++;

//        auto timeDiff = current_time - start_time - paused_duration - curPauseDiff;
//        start_time += timeDiff - std::chrono::duration_cast<std::chrono::nanoseconds>(timeDiff * (this->playSpeed/_playSpeed));
        //std::cout << "goToSeconds("<< _seconds <<", " << _relative << ")" << " // ph=" << counters.playhead << std::endl;

        checkLoops();

        updateInternals();

        onSeek.notify(counters);

        return true;
    }
    return false;
}

void ofxPlayhead::setLoop(ofxPlayheadLoopMode loopMode) {
    this->loopMode = loopMode;
}

void ofxPlayhead::setDuration(double duration) {
    if(duration <= 0) return; // zero or infinite durations are not supported yet !
    this->duration = duration;
}

void ofxPlayhead::setDurationFromBeats(int bars, int noteValue) {
    if(bars <= 0 && noteValue <= 0) return; // zero or infinite durations are not supported yet !
    ofxPHTimeSignature timeSignature(bars, noteValue);
    double beatsInBar = static_cast<double>(timeSignature.beatsPerBar) * timeSignature.notesPerBeat;
    double beatsInMinute = static_cast<double>(timeSignature.bpm) / 60.0;
    setDuration(beatsInBar / beatsInMinute);
}

bool ofxPlayhead::setPlaySpeed(double _playSpeed) {
    if(_playSpeed==0) return false; // Not supported yet ! (Use pause).

    // rtAbs needs to compensate start time if speed changes !
    if(playing && playbackMode == ofxPlayheadMode_RealTime_Absolute){
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<long long, std::nano> curPauseDiff = paused ? (current_time - paused_time) : std::chrono::duration<long long, std::nano>(0);
        auto timeDiff = current_time - start_time - paused_duration - curPauseDiff;
        start_time += timeDiff - std::chrono::duration_cast<std::chrono::nanoseconds>(timeDiff * (this->playSpeed/_playSpeed));

        // Alternative
        //std::chrono::duration<long long, std::nano> originalDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(timeDiff);
        //std::chrono::duration<long long, std::nano> newDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(timeDiff * (this->playSpeed/_playSpeed));
        //start_time += originalDiff - newDiff;
    }
    this->playSpeed = _playSpeed;
    return true;
}

double ofxPlayhead::getPlaySpeed() const {
    return playSpeed;
}

// Optional, to update realtime modes in-between frames
void ofxPlayhead::tickUpdate(){
    // If not playing or paused, do nothing
    if (!playing || paused) {
        return;
    }

    tickPlayHead(false);
    checkLoops();
    updateInternals();

    //onFrameTick.notify(counters);
}

// At the begining of your frame, tick this. Todo: after frame so update() can prapare data for the next frame ?
void ofxPlayhead::tickFrame(){
    if(bStartNextFrame){
        start(startNextFrameOffet);
        bStartNextFrame = false;
        startNextFrameOffet = 0u;
        return;
    }

    // If not playing or paused, do nothing
    if (!playing || paused) {
        // Ensure delta is set to 0
        counters.tDelta = 0.0;
        counters.tDeltaSeek = 0.0;
        return;
    }

    tickPlayHead(true);
    checkLoops();
    updateInternals();

    onFrameTick.notify(counters);
}

// Updates the playhead and framecount to the most actual ones
void ofxPlayhead::tickPlayHead(const bool isNewFrame){
    // If not playing or paused, do nothing
    if (!playing || paused) {
        return;
    }

    // Incremment rendered frames
    if(isNewFrame){
        counters.frameCount++;
        // Include seeked time
        if(seekedTime!=0){
            counters.tDeltaSeek = seekedTime;
            //std::cout << "Seeked=" << seekedTime << std::endl;
            seekedTime = 0;
        }
        else {
            counters.tDeltaSeek = 0;
        }
    }

    auto current_time = std::chrono::high_resolution_clock::now();
    //auto elapsed_time = std::chrono::duration<double>(current_time - last_frame_time) * playSpeed;

    // Update playhead for rt modes
    if (playbackMode == ofxPlayheadMode_RealTime_Absolute || playbackMode == ofxPlayheadMode_RealTime_Relative) {

        //auto elapsed_time = std::chrono::duration<double>(current_time - ((playbackMode == ofxPlayheadMode_RealTime_Absolute)?start_time:last_frame_time)) * playSpeed;
        //playhead = elapsed_time.count();
        auto elapsedFromLastFrame = std::chrono::duration<double>(current_time - last_frame_time) * playSpeed;

        if (playbackMode == ofxPlayheadMode_RealTime_Absolute) {
            auto elapsedFromStart = std::chrono::duration<double>(current_time - paused_duration - start_time) * glm::abs(playSpeed);
            if(playSpeed<0){ // Reverse playing ?
//                counters.playhead = duration - std::abs(std::fmod(elapsedFromStart.count(), duration));
//                counters.playhead = duration + elapsedFromStart.count();
                counters.playhead = elapsedFromStart.count() * glm::sign(playSpeed);
                //counters.tDelta = 0.0; // nope !
                //std::cout << "ReversePlay=" << playSpeed << " // ph=" << counters.playhead << std::endl;
            }
            else { // Forward playing
                //counters.tDelta = elapsed_time.count() - counters.playhead;
//                auto frameDiff = std::chrono::duration<double>(current_time-last_frame_time) * playSpeed;
//                counters.tDelta = frameDiff.count();
                counters.playhead = elapsedFromStart.count();
            }


            //playhead = glm::abs(glm::mod(playhead, duration));
            //std::cout << "elapsed=" << elapsed_time.count() << " Playhead=" << playhead << std::endl;
            //std::cout << "elapsed=" << glm::fract(1.0) << " Playhead=" << glm::mod(1.0,1.0) << std::endl;
        } else if (playbackMode == ofxPlayheadMode_RealTime_Relative) {
            //std::cout << "elapsed=" << ((double)elapsed_time.count()/1000000000) << " Playhead=" << counters.playhead << std::endl;
            if(isNewFrame){
                counters.playhead += elapsedFromLastFrame.count();
            }
            else {
                // Todo : this could also be updated in-between frames
                //counters.playhead = last_frame_time + elapsedFromLastFrame;
            }
        }
        if(isNewFrame) counters.tDelta = elapsedFromLastFrame.count() * playSpeed;
    }
    // Update playhead for offline mode
    else /*if (playbackMode == ofxPlayheadMode_Offline)*/ {
        if(isNewFrame){
            double frame_time = 1.0 / (double)fps;
            counters.tDelta = frame_time * playSpeed;
            // Add one frame for the speed timer
            counters.frameNumSpeed += 1;
            // Add 1 frame's time to the playhead, will compute the framenum from that.
            //counters.playhead = frame_time * (counters.frameNumSpeed*(playSpeed)); // Fixme: playSpeed=0=??!
            //counters.playhead = frame_time * ((double)counters.frameNum+(1.0*playSpeed)); // Fixme: playSpeed=0=??!
            counters.playhead = frame_time * (playSpeed*counters.frameNumSpeed);
//            counters.playhead = counters.tDelta * counters.frameNum;
            //std::cout << "New offline frame ! OFFrame=" << ofGetFrameNum() << " FrameNum=" << counters.frameNum << " / " << counters.frameNumSpeed << " \t ph=" << counters.playhead << " --> " << (counters.playhead / frame_time)<<std::endl;
        }
    }
    // Remember (for rt modes)
    if(isNewFrame) last_frame_time = current_time;
    // No new frame : set delta to 0 to things don't move.
    // Todo: when playSpeed between -1 and 1, frames are added. Use real frame delta.
    else {
        counters.tDelta = 0.0;
    }
}

// Updates playhead for looping
void ofxPlayhead::checkLoops(){
    bool bNewLoop = false;
    // Check if duration exceeded
    if (playbackMode == ofxPlayheadMode_RealTime_Absolute) {
        //const long curLoopCount = counters.playhead < 0 ? glm::ceil(counters.playhead / duration) : glm::floor(counters.playhead / duration);
        const long curLoopCount = glm::floor(counters.playhead / duration);
        const bool newLoop = rtAbsLoopCount != curLoopCount;
        if(newLoop){
            //std::cout << "newLoop="<<bNewLoop<<" / rcnt="<< rtAbsLoopCount << " / cnt=" << counters.loopCount << " // ph=" << counters.playhead << " / d=" << duration << std::endl;

            rtAbsLoopCount = curLoopCount;

            // Exception: don't count 1st loop from start if reverse playing
            if(bIgnoreFirstReverseLoop && playSpeed<0){
                bNewLoop = false;
            }
            else {
                counters.loopCount++;
                bNewLoop = true;
            }
            // Never ignore again
            //if(bIgnoreFirstReverseLoop) bIgnoreFirstReverseLoop = false;
            bIgnoreFirstReverseLoop &= false;
            //std::cout << "newLoop="<<bNewLoop<<" / rcnt="<< rtAbsLoopCount << " / cnt=" << counters.loopCount << " // ph=" << counters.playhead << " / d=" << duration << std::endl;
        }
        //else std::cout << "noNewLoop="<< rtAbsLoopCount << " / " << counters.loopCount << " // ph=" << counters.playhead << " / d=" << duration << std::endl;
        // Playhead relative to start time : compute loopcount each frame
//            std::size_t newCount = glm::floor(glm::abs(counters.playhead) / duration);
//            if(counters.loopCount != newCount){
//                counters.loopCount = newCount;
//                // Below playhead will be fmodded to be in range
//                bNewLoop = true;
//                std::cout << "newLoop="<< newCount << " / ph=" << counters.playhead << " / d=" << duration << std::endl;
//            }
//            else std::cout << "noNewLoop="<< newCount << " / ph=" << counters.playhead << " / d=" << duration << std::endl;
    }
    // rt-rel + offline
    else if ( counters.playhead >= duration || counters.playhead < 0) {
        bNewLoop = true;

        // Exception: don't count 1st loop from start if reverse playing
        if(bIgnoreFirstReverseLoop && playSpeed<0){
            bNewLoop = false;
        }
        else {
            // Loop count is simply incremented
            counters.loopCount++;
        }
        // Never ignore again
        //if(bIgnoreFirstReverseLoop) bIgnoreFirstReverseLoop = false;
        bIgnoreFirstReverseLoop &= false;
        //std::cout << "bNewLoop=" << bNewLoop << " // cnt="<< counters.loopCount << std::endl;
    }
    //std::cout << "bNewLoop=" << bNewLoop << " // cnt="<< counters.loopCount << std::endl;
    // Never ignore again
    // //if(bIgnoreFirstReverseLoop) bIgnoreFirstReverseLoop = false;
    // bIgnoreFirstReverseLoop &= false;

    // Still playing ? Start over !
    //if(playing){

        // Move playhead
        // Todo: In offline modes (and maybe realtime optional) : Ensure to align the playhead on the frame grid.
        if(counters.playhead < 0){
            // Reverse play: loop at end
            // counters.playhead = duration; // todo : use fmod like below ?
            // counters.playhead = std::fmod(counters.playhead, duration); // todo : use fmod like below ?
            counters.playhead = duration + std::fmod(counters.playhead, duration); // todo : use fmod like below ?
            //if(counters.playhead<0) counters.playhead = duration-counters.playhead;
        }
        else {
            // Forward play
            counters.playhead = std::fmod(counters.playhead, duration); // note: fmod sets the frame to 0 + time_passed_over_the_end
        }

        // Notify
        if(bNewLoop){
            onStart.notify(counters.loopCount);
        }
    //}

    if(bNewLoop){

        // Handle looping / Stopping
        switch(loopMode){
            case ofxPlayheadLoopMode::NoLoop :
                stop();
            break;
            case ofxPlayheadLoopMode::LoopOnce :
                if(counters.loopCount>1) stop();
            break;
            case ofxPlayheadLoopMode::LoopInfinite :
                // keep playing
            break;
        }

        // Notify
        onStart.notify(counters.loopCount);

    }
}

// Updates internals and ramps according to playhead
void ofxPlayhead::updateInternals(){
    double frame_time = 1.0 / (double)fps;

    // Update counters
    counters.beatCount = std::floor(counters.playhead / timeSignature.getBeatDurationSecs());
    counters.noteCount = std::floor(counters.playhead / timeSignature.getNoteDurationSecs());
    counters.barCount  = std::floor(counters.playhead / timeSignature.getBarDurationSecs());
    //if(playbackMode == ofxPlayheadMode_Offline){
        counters.frameNum = std::floor(counters.playhead / frame_time + 0.00001); // Note: Some weird conversions happens sometimes here, printing not the same numbers below. floor / cast don't work, ceil seems to do fine.
        counters.frameNumSpeed = std::floor((counters.playhead) / (frame_time*playSpeed) + 0.00001);
    //}
    //else counters.frameNum = std::floor(counters.playhead / frame_time + 0.00001); // Note: Some weird conversions happens sometimes here, printing not the same numbers below. floor / cast don't work, ceil seems to do fine.

    // Update ramps
    timeRamps.updateRamps(counters.playhead, timeSignature, duration);

    counters.progress = counters.playhead / duration; // Fixme: 0 duration !
}

//void ofxPlayhead::tick() {
//    // If not playing or paused, do nothing
//    if (!playing || paused) {
//        return;
//    }

//    tickPlayHead();

//    checkLoops();

//    updateInternals();
//}

double ofxPlayhead::getElapsedSeconds() const {
    //        auto current_time = std::chrono::high_resolution_clock::now();
    //        std::chrono::duration<double> elapsed_seconds = current_time - (start_time + paused_time);
    //        return elapsed_seconds.count();
    return counters.playhead;
}

unsigned int ofxPlayhead::getFrameNum() const {
    return static_cast<unsigned int>(counters.frameNum);
}

// Returns the number of rendered frames
unsigned int ofxPlayhead::getElapsedFrames() const {
    return counters.frameCount;
}

// Returns the number of frames if the composition is finite; 0 otherwise.
unsigned int ofxPlayhead::getTotalFrames() const {
    return duration * fps;
}

// Returns the number of bars if the composition is finite; 0 otherwise.
unsigned int ofxPlayhead::getTotalBars() const {
    //return ((float)duration/60.)*timeSignature.beatsPerBar;
    return glm::floor((float)duration / timeSignature.getBarDurationSecs());
}

// Returns the number of notes if the composition is finite; 0 otherwise.
unsigned int ofxPlayhead::getTotalNotes() const {
    return glm::floor((float)duration / timeSignature.getNoteDurationSecs());
}

double ofxPlayhead::getDuration() const {
    return duration;
}

// Playhead position
double ofxPlayhead::getProgress() const {
    return counters.progress;
    //return counters.playhead/getDuration();
    //return (double)frame_count/(double)getTotalFrames();
}

unsigned int ofxPlayhead::getFps() const {
    return fps;
}

void ofxPlayhead::setFps(unsigned int _fps) {
    fps = _fps;
}

//    double getCurrentBeat() const {
//        double beatsElapsed = getElapsedSeconds() * timeSignature.bpm / 60.0;
//        double beatsPerFrame = static_cast<double>(fps) / 60.0;
//        return std::fmod(beatsElapsed, timeSignature.notesPerBeat) + 1.0; // Beats are 1-indexed
//    }

bool ofxPlayhead::isPlaying() const {
    return playing;
}

ofxPlayheadMode ofxPlayhead::getPlayMode() const {
    return playbackMode;
}

bool ofxPlayhead::setPlayMode(ofxPlayheadMode _mode) {
    if(!bAllowLossyOperations && !playing) return false;

    // Prevent changing the timeline
    resetPausedDuration();
    if(_mode == ofxPlayheadMode_RealTime_Absolute){
        auto current_time = std::chrono::high_resolution_clock::now();
        //start_time = std::chrono::high_resolution_clock::now() - std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0));
        start_time = current_time - std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0/playSpeed));
        //std::cout << "Forcing playhead to: " << counters.playhead << std::endl;
        //start_time = last_frame_time - std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0));
        if(paused) paused_time = current_time;
    }
    playbackMode = _mode;
    return true;
}

void ofxPlayhead::registerKeyboardShortcuts(){
    ofAddListener(ofEvents().keyPressed, this, &ofxPlayhead::keyPressed, OF_EVENT_ORDER_AFTER_APP);
}

void ofxPlayhead::removeKeyboardShortcuts(){
    ofRemoveListener(ofEvents().keyPressed, this, &ofxPlayhead::keyPressed, OF_EVENT_ORDER_AFTER_APP);
}

void ofxPlayhead::keyPressed(ofKeyEventArgs &key){
    const bool hasModifier = key.hasModifier(OF_KEY_CONTROL) || key.hasModifier(OF_KEY_COMMAND) || key.hasModifier(OF_KEY_SHIFT) || key.hasModifier(OF_KEY_ALT);
    switch(key.key){
        case ' ':
            this->togglePause();
            return;
        case OF_KEY_LEFT:
        case OF_KEY_RIGHT:
            if(isPaused()){
                this->nextFrame(key.keycode==OF_KEY_LEFT?-1:1);
            }
            return;
        case 'l':
            if(!hasModifier){
                std::cout << "l-switch" << std::endl;
                switch(this->loopMode){
                    case NoLoop :
                        this->setLoop(LoopOnce);
                        return;
                    case LoopOnce :
                        this->setLoop(LoopInfinite);
                        return;
                    case LoopInfinite :
                        this->setLoop(NoLoop);
                        return;
                }
            }
            break;
        case 'm':
            if(!hasModifier){
                switch(this->playbackMode){
                    case ofxPlayheadMode_RealTime_Relative:
                        this->setPlayMode(ofxPlayheadMode_RealTime_Absolute);
                        return;
                    case ofxPlayheadMode_RealTime_Absolute:
                        this->setPlayMode(ofxPlayheadMode_Offline);
                        return;
                    case ofxPlayheadMode_Offline:
                        this->setPlayMode(ofxPlayheadMode_RealTime_Relative);
                        return;
                }
            }
            return;
        case '.':
            if(!hasModifier){
                this->stop();
            }
            return;
        default:
            break;
    }
}

#ifdef ofxAddons_ENABLE_IMGUI
void ofxPlayhead::drawImGuiPlayControls(bool horizontalLayout){
    ImGui::PushID((const void *)this);

    static bool bShowSettings = false, bShowRamps = false;
    static double speedMin=-10, speedMax=10;

    // Show "tabs" / Settings toggle
    ImGui::BeginGroup();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    if(ImGui::Button("T##tab-time")){
        bShowSettings = false;
        bShowRamps = false;
    }
    if(ImGui::IsItemActive() || ImGui::IsItemHovered()){
        pos.x += 20;
        ImGui::GetForegroundDrawList()->AddRectFilled(pos, pos+ImVec2(35.f, ImGui::GetFrameHeight()), ImGui::GetColorU32(ImGuiCol_Button, 1.f));
        ImGui::GetForegroundDrawList()->AddText(pos+ImVec2(3,3), ImGui::GetColorU32(ImGuiCol_Text), "Time");
    }

    pos = ImGui::GetCursorScreenPos();
    if(ImGui::Button("S##tab-settings")){
        bShowSettings = true;
        bShowRamps = false;
    }
    if(ImGui::IsItemActive() || ImGui::IsItemHovered()){
        pos.x += 20;
        ImGui::GetForegroundDrawList()->AddRectFilled(pos, pos+ImVec2(58.f, ImGui::GetFrameHeight()), ImGui::GetColorU32(ImGuiCol_Button, 1.f));
        ImGui::GetForegroundDrawList()->AddText(pos+ImVec2(3,3), ImGui::GetColorU32(ImGuiCol_Text), "Settings");
    }

    pos = ImGui::GetCursorScreenPos();
    if(ImGui::Button("R##settings-ramps")){
        bShowSettings = false;
        bShowRamps = true;
    }
    if(ImGui::IsItemActive() || ImGui::IsItemHovered()){
        pos.x += 20;
        ImGui::GetForegroundDrawList()->AddRectFilled(pos, pos+ImVec2(42.f, ImGui::GetFrameHeight()), ImGui::GetColorU32(ImGuiCol_Button, 1.f));
        ImGui::GetForegroundDrawList()->AddText(pos+ImVec2(3,3), ImGui::GetColorU32(ImGuiCol_Text), "Ramps");
    }

    ImGui::EndGroup();

    if(horizontalLayout) ImGui::SameLine();

    if(bShowSettings){
        // Time signature
        ImGui::BeginGroup();

        // Time Signature
        ImGui::PushItemWidth(60);

        ImGui::SeparatorText("TimeSig");
        // Time Signature Info
        ImGui::SameLine();
    //            int posX = ImGui::GetCursorPosX();
    //            ImGui::SetCursorPosX(posX-10);
        if (ImGuiEx::BeginHelpMarker("[i]")){
            ImGui::SeparatorText("TS Info");
            ImGui::Text("1 beat = %.3fsec", timeSignature.getBeatDurationSecs());
            ImGui::Text("1 bar = %.3fsec", timeSignature.getBarDurationSecs());
            ImGui::Text("1 note = %.3fsec", timeSignature.getNoteDurationSecs());
            ImGuiEx::EndHelpMarker();
        }

        static unsigned int bpmMin = 1, bpmMax=512;
        if(ImGui::DragScalar("BPM", ImGuiDataType_U16, &timeSignature.bpm, 1.f, &bpmMin, &bpmMax, "%u")){
            timeSignature.updateValues();
        }
        static unsigned int noteStep = 1, noteStepFast=1;
        if(ImGui::InputScalar("Bars", ImGuiDataType_U16, &timeSignature.beatsPerBar, &noteStep, &noteStepFast, "%u")){
            timeSignature.updateValues();
        }
        if(ImGui::InputScalar("Notes", ImGuiDataType_U16, &timeSignature.notesPerBeat, &noteStep, &noteStepFast, "%u")){
            timeSignature.updateValues();
        }
        ImGui::PopItemWidth();
        ImGui::EndGroup();

        if(horizontalLayout) {
            ImGui::SameLine();
            ImGui::Dummy({8,8});
            ImGui::SameLine();
        }

        // Playback
        ImGui::BeginGroup();
        if(horizontalLayout) ImGui::PushItemWidth(80);
        ImGui::SeparatorText("Playback");

        if(ImGui::InputScalar("Fps", ImGuiDataType_U16, &fps, &noteStep, &noteStepFast, "%u")){
            //timeSignature.updateValues();
        }

        // Playback
        static int currentPBMode=-1;
        currentPBMode = playbackMode==ofxPlayheadMode::ofxPlayheadMode_RealTime_Absolute?0:(playbackMode==ofxPlayheadMode::ofxPlayheadMode_RealTime_Relative?1:2);
        if(ImGui::Combo("Playback", &currentPBMode, "Real-time (absolute)\0Real-time (relative)\0Offline\0\0")){
            setPlayMode(currentPBMode==0?ofxPlayheadMode::ofxPlayheadMode_RealTime_Absolute : (currentPBMode==1?ofxPlayheadMode::ofxPlayheadMode_RealTime_Relative : ofxPlayheadMode::ofxPlayheadMode_Offline));
        }

        // Looping
        static int currentLoopMode=-1;
        currentLoopMode = loopMode==ofxPlayheadLoopMode::NoLoop?0:(loopMode==ofxPlayheadLoopMode::LoopOnce?1:2);
        if(ImGui::Combo("Loop", &currentLoopMode, "No Loop\0Loop Once\0Loop Infinite\0\0")){
            loopMode = currentLoopMode==0?ofxPlayheadLoopMode::NoLoop:(currentLoopMode==1?ofxPlayheadLoopMode::LoopOnce:ofxPlayheadLoopMode::LoopInfinite);
        }

        // Play speed
        double speed = playSpeed;
        if(ImGui::DragScalar("Speed", ImGuiDataType_Double, &speed, 0.05f, &speedMin, &speedMax, "%7.3f")){
            setPlaySpeed(speed);
        }

        if(horizontalLayout) ImGui::PopItemWidth();
        ImGui::EndGroup();

        if(horizontalLayout) ImGui::SameLine();

        // Duration
        ImGui::BeginGroup();
        if(horizontalLayout) ImGui::PushItemWidth(80);
        ImGui::SeparatorText("Duration");

        unsigned int durationHours = glm::floor(duration/(3600));
        unsigned int durationMinutes = glm::mod(glm::floor(duration/60), 60.);
        unsigned int durationSeconds = glm::mod(duration, 60.);
        static unsigned int durationMin = 0, durationHoursMax = 23, durationMinSecsMax=59;
        bool updateDurations = false;
        if(ImGui::DragScalar("##hours", ImGuiDataType_U16, &durationHours, 1.f, &durationMin, &durationHoursMax, "%u hours")){
            updateDurations = true;
        }
        if(ImGui::DragScalar("##minutes", ImGuiDataType_U16, &durationMinutes, 1.f, &durationMin, &durationMinSecsMax, "%u mins")){
            updateDurations = true;
        }
        if(ImGui::DragScalar("##seconds", ImGuiDataType_U16, &durationSeconds, 1.f, &durationMin, &durationMinSecsMax, "%u secs")){
            updateDurations = true;
        }

        if(updateDurations){
            duration = glm::max(durationHours*3600 + durationMinutes*60 + durationSeconds, 1u);
        }
        if(horizontalLayout) ImGui::PopItemWidth();
        ImGui::EndGroup();

        if(horizontalLayout) ImGui::SameLine();

        // Duration info
        ImGui::BeginGroup();
        //if(horizontalLayout) ImGui::PushItemWidth(80);
        ImGui::SeparatorText("Info");

        ImGui::Text("Duration: %.0f sec", duration);
        ImGui::Text("%u frames total", getTotalFrames());
        ImGui::Text("x bars (measures)");
        ImGui::Text("x notes");

        //ImGui::Spacing();
        //if(horizontalLayout) ImGui::PopItemWidth();
        ImGui::EndGroup();

        // More options
        if(horizontalLayout) ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::SeparatorText("Advanced");

        ImGui::Checkbox("Allow Lossy Operations", &bAllowLossyOperations);
        ImGuiEx::ShowHelpMarker("Allows changing playMode and playSpeed while playing. Off is more robust.");

        ImGui::EndGroup();


        //if(horizontalLayout) ImGui::SameLine();
    } // End settings

    // Ramps
    else if(bShowRamps){
//        for(unsigned int i=0; i < ofxPHTL_Ramp_Hist_Size-1; i++){
//            beatProgressHist[i] = beatProgressHist[i+1];
//        }
//        beatProgressHist[ofxPHTL_Ramp_Hist_Size-1] = timeRamps.beatProgress;

        // Signals & Ramps
        ImGui::BeginGroup();

        // Bar Ramps
        static float barProgressHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Bar progress: %.2f", barProgressHist, timeRamps.barProgress, !paused);
        static float barPulseHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Bar pulse: %.2f", barPulseHist, timeRamps.barPulse, !paused);
        static float barStepHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Bar Step: %.2f", barStepHist, timeRamps.barStep, !paused);

        ImGui::Separator();

        // Beat Ramps
        static float beatProgressHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Beat Progress: %.2f", beatProgressHist, timeRamps.beatProgress, !paused);
        static float beatPulseHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Beat Pulse: %.2f", beatPulseHist, timeRamps.beatPulse, !paused);
        static float beatStepHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Beat Step: %.2f", beatStepHist, timeRamps.beatStep, !paused);

        ImGui::Separator();

        // Note Ramps
        static float noteProgressHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Note Progress: %.2f", noteProgressHist, timeRamps.noteProgress, !paused);
        static float notePulseHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Note Pulse: %.2f", notePulseHist, timeRamps.notePulse, !paused);
        static float noteStepHist[ofxPHTL_Ramp_Hist_Size] = {0};
        ImGuiEx::RampGraph("Note Step: %.2f", noteStepHist, timeRamps.noteStep, !paused);

        ImGui::EndGroup(); // End Ramps group
    } // end ramps section

    // Timer clock & Controls
    else {

        // TimeSig
        ImGui::BeginGroup();
        ImGui::PushItemWidth(100);
        //ImGui::BeginGroup();
        //ImGui::SeparatorText("TimeSig");

        ImGui::BeginDisabled();
        //ImGui::Dummy({100,20});
        ImGui::BeginGroup();
        ImGui::Text("Bar");
        ImGui::VSliderFloat("##barprogress", ImVec2(14, 50), &timeRamps.barProgress, 0.0f, 1.0f, "");
        ImGui::SameLine();
        ImGui::VSliderFloat("##barstep", ImVec2(6, 50), &timeRamps.barStep, 0.0f, 1.0f, "");
        ImGui::Text("%04lu", counters.barCount%10000);
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Beat");
        ImGui::VSliderFloat("##beatprogress", ImVec2(14, 50), &timeRamps.beatProgress, 0.0f, 1.0f, "");
        ImGui::SameLine();
        ImGui::VSliderFloat("##beatstep", ImVec2(6, 50), &timeRamps.beatStep, 0.0f, 1.0f, "");
        ImGui::Text("%04lu", counters.beatCount%10000);
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Note");
        ImGui::VSliderFloat("##noteprogress", ImVec2(14, 50), &timeRamps.noteProgress, 0.0f, 1.0f, "");
        ImGui::SameLine();
        ImGui::VSliderFloat("##notestep", ImVec2(6, 50), &timeRamps.noteStep, 0.0f, 1.0f, "");
        ImGui::Text("%04lu", counters.noteCount%10000);
        ImGui::EndGroup();

        ImGui::EndDisabled();
        //ImGui::EndGroup();
        ImGui::PopItemWidth();
        ImGui::EndGroup();

        if(horizontalLayout) ImGui::SameLine();

        // Timers
        ImGui::BeginGroup();

        ImGui::BeginDisabled();

        if(horizontalLayout){
            ImGui::Dummy({30,5});
            ImGui::SameLine();
        }
        ImGui::Text("Time");
        ImGui::EndDisabled();

        static bool bShowSecsWithMins = false;
        double elapsed = getElapsedSeconds();
        if(bShowSecsWithMins){
            //unsigned int durationHours = glm::floor(elapsed/(3600));
            unsigned int durationMinutes = glm::floor(glm::mod(glm::floor(elapsed/60), 60.));
            unsigned int durationSeconds = glm::floor(glm::mod(elapsed, 60.));
            unsigned int durationMs = glm::round(glm::mod(elapsed, 1.)*1000);
            ImGui::Text(" %02u:%02u:%03u s", durationMinutes, durationSeconds, durationMs);
        }
        else ImGui::Text("%10.4f s", elapsed);
        // Toggle display style on click
        if(ImGui::IsItemClicked()){
            bShowSecsWithMins = !bShowSecsWithMins;
        }
        // Show delta time on hover
        else if(ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary)){
            if(ImGui::BeginTooltip()){
                ImGui::Text("Delta time: %.5f sec", counters.tDelta);
                ImGui::Text("Seek delta: %.5f sec", counters.tDeltaSeek);
            }
            ImGui::EndTooltip();
        }
        ImGui::Text("%10u f", getFrameNum());
        ImGui::Text("%04lu.%02lu.%02lu tc", counters.barCount, counters.beatCount%timeSignature.beatsPerBar, counters.noteCount%timeSignature.notesPerBeat);
        ImGui::Text("%10.6f %%", getProgress()*100);
        //if(counters.loopCount>0) ImGui::Text("Loops   : %10lu", counters.loopCount);

        //

        ImGui::EndGroup(); // Timers

        if(horizontalLayout){
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
        }

        // Time
        ImGui::BeginGroup();
        //ImGui::SeparatorText("Time");
        static double pHeadMin=0.;
        ImVec2 barSize = {ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight()-2};
        ImGui::Dummy({barSize.x, 8});
        ImVec2 barPos = ImGui::GetCursorScreenPos();
        double barScaleSecond = barSize.x / duration;
        barPos.y+=1;
        ImGui::SetNextItemWidth(barSize.x);
        double phCopy = counters.playhead;
        if(ImGui::SliderScalar("##Playhead",ImGuiDataType_Double, &phCopy, &pHeadMin, &duration, "" )){
            goToSeconds(phCopy, false, false);
        }

        ImDrawList* wdl = ImGui::GetWindowDrawList();
        //wdl->AddRect(barPos, barPos+barSize, IM_COL32(255,255,255,255));

        // Draw Bars
        unsigned int numBars = glm::floor(duration / timeSignature.getBarDurationSecs());
        for(unsigned int bi=0; bi < numBars; bi++){
            float posX = bi * timeSignature.getBarDurationSecs() * barScaleSecond;
            wdl->AddLine({barPos.x+posX, barPos.y}, {barPos.x+posX, barPos.y+barSize.y}, ImGui::GetColorU32(ImGuiCol_TextDisabled));
        }

        // Draw Beats
        unsigned int numBeats = glm::floor(duration / timeSignature.getBeatDurationSecs());
        for(unsigned int bi=0; bi < numBeats; bi++){
            float posX = bi * timeSignature.getBeatDurationSecs() * barScaleSecond;
            wdl->AddLine({barPos.x+posX, barPos.y}, {barPos.x+posX, barPos.y+barSize.y}, ImGui::GetColorU32(ImGuiCol_TextDisabled, .2f));
        }

        // Draw seconds
        int fontPixels = ImGui::GetFontSize();
        double tScale = 10.0; // By default, every 10 secs
        unsigned int maxCharSlots = barSize.x/(fontPixels*3);
        unsigned int numTimeSlots = duration/tScale;
        if(numTimeSlots > maxCharSlots){
            tScale = 30.0;
        }
        for(float t = 0; t <= duration; t+=tScale){
            float posX = t * barScaleSecond;
            std::string label = ofToString(glm::floor(t/60.f)) + ":" + ofToString(glm::mod(t,60.f));
            wdl->AddLine({barPos.x+posX, barPos.y}, {barPos.x+posX, barPos.y+barSize.y}, ImGui::GetColorU32(ImGuiCol_Text));
            wdl->AddText({barPos.x+posX-fontPixels*1.f, barPos.y-barSize.y}, ImGui::GetColorU32(ImGuiCol_Text, .6f), label.c_str());
        }

        // Big cursor line
        float playHeadX = getProgress() * barSize.x;
        wdl->AddLine({barPos.x+playHeadX, barPos.y-4}, {barPos.x+playHeadX, barPos.y+barSize.y+4}, ImGui::GetColorU32(ImGuiCol_ButtonActive));


        // Play Controls
        ImGui::BeginGroup();

        // Start over
        if(ImGui::Button("<-")){
            goToFrame(0);
        }
        ImGui::SameLine();

        // Start/Stop
        if(!playing){
            if(ImGui::Button("Play")){
                start();
            }
        }
        else {
            if(ImGui::Button("Stop")){
                stop();
            }
        }
        ImGui::SameLine();

        // Pause / Resume
        if(playing && !paused){
            if(ImGui::Button("Pause ")){
                pause();
            }
        }
        else if(ImGui::Button("Resume")){
            if(!playing) start();
            else resume();
        }
        ImGui::SameLine();

        // Prev-next controls
        ImGui::Spacing();
        ImGui::SameLine();

        bool endDisabled = false;
        if(!(paused || playSpeed==0)){
            ImGui::BeginDisabled();
            endDisabled = true;
        }
        ImGuiDir frameNav = ImGuiEx::ButtonPair(ImGuiDir_Left, ImGuiDir_Right);
        if(frameNav!=ImGuiDir_None){
            if(frameNav==ImGuiDir_Left) nextFrame(ImGui::IsKeyDown(ImGuiMod_Shift)?(-1*fps):-1);
            else nextFrame(ImGui::IsKeyDown(ImGuiMod_Shift)?fps:1);
        }
        if(endDisabled) ImGui::EndDisabled();

        if(horizontalLayout) ImGui::SameLine();

        // Speed
        const bool disableLossy = !bAllowLossyOperations && (playing || paused);
        if(disableLossy) ImGui::BeginDisabled();
        ImGui::Text(" ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        static double speed = playSpeed;
        ImGui::DragScalar("##playSpeed", ImGuiDataType_Double, &speed, 0.001f, &speedMin, &speedMax, "%6.3f" );
        if(ImGui::IsItemDeactivatedAfterEdit()){
            setPlaySpeed(speed);
            speed = playSpeed;
        }
        else if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_ForTooltip) && !ImGui::IsItemActive() && ImGui::BeginItemTooltip()){
            ImGui::Text("Play Speed: %.001f", playSpeed);
            ImGui::EndTooltip();
        }
        if(disableLossy) ImGui::EndDisabled();


        ImGui::SameLine();

        // Loop toggle
        if(loopMode==ofxPlayheadLoopMode::NoLoop){
            if(ImGui::Button("x##noloop")){
                loopMode=ofxPlayheadLoopMode::LoopOnce;
            }
        }
        else if(loopMode==ofxPlayheadLoopMode::LoopOnce){
            if(ImGui::Button("|##looponce")){
                loopMode=ofxPlayheadLoopMode::LoopInfinite;
            }
        }
        else {
            if(ImGui::Button("o##loop")){
                loopMode=ofxPlayheadLoopMode::NoLoop;
            }
        }
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_ForTooltip) && !ImGui::IsItemActive() && ImGui::BeginItemTooltip()){
            ImGui::Text("Looping: %s", loopMode==ofxPlayheadLoopMode::LoopOnce?"Once":(loopMode==ofxPlayheadLoopMode::NoLoop?"No Loop":"Loop Infinite"));
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        ImGui::Text("%03lu", counters.loopCount);

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // Playback Mode toggle
        if(disableLossy) ImGui::BeginDisabled();
        if(playbackMode==ofxPlayheadMode::ofxPlayheadMode_Offline){
            if(ImGui::Button("offline##bpmo")){
                setPlayMode(ofxPlayheadMode::ofxPlayheadMode_RealTime_Absolute);
            }
            else if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
                setPlayMode(ofxPlayheadMode::ofxPlayheadMode_RealTime_Relative);
        }
        else if(playbackMode==ofxPlayheadMode::ofxPlayheadMode_RealTime_Absolute){
            if(ImGui::Button("r-t Abs##pbmra")){
                setPlayMode(ofxPlayheadMode::ofxPlayheadMode_RealTime_Relative);
            }
            else if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
                setPlayMode(ofxPlayheadMode::ofxPlayheadMode_Offline);
        }
        else{
            if(ImGui::Button("r-t Rel##pbmrr")){
                setPlayMode(ofxPlayheadMode::ofxPlayheadMode_Offline);
            }
            else if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
                setPlayMode(ofxPlayheadMode::ofxPlayheadMode_RealTime_Absolute);
        }
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_ForTooltip) && !ImGui::IsItemActive() && ImGui::BeginItemTooltip()){
            ImGui::Text("Playback Mode: %s", playbackMode==ofxPlayheadMode::ofxPlayheadMode_Offline?"Offline":(playbackMode==ofxPlayheadMode::ofxPlayheadMode_RealTime_Absolute?"Real Time (absolute)":"Real Time (relative)"));
            ImGui::EndTooltip();
        }
        if(disableLossy) ImGui::EndDisabled();

        ImGui::EndGroup(); // Controls
        ImGui::EndGroup(); // time section
    }

    ImGui::PopID();
}

void ofxPlayhead::drawImGuiTimelineWindow(bool* p_open){
    if(ImGui::Begin("Timeline", p_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)){
        drawImGuiPlayControls();
    }

    ImGui::End();
}
#endif

void ofxPlayhead::reset() {
    playing = false;
    paused = false;
    counters.playhead = 0.0;
    counters.frameCount = 0;
    counters.frameNum = 0;
    counters.frameNumSpeed = 0; // for frame-based index in offline mode, when speed is smaller then a frame
    counters.loopCount = 0;
    counters.noteCount = 0;
    counters.beatCount = 0;
    counters.barCount = 0;
    counters.progress = 0;
    bIgnoreFirstReverseLoop = true;

    //loop_complete = false;
    start_time = std::chrono::high_resolution_clock::now();
    last_frame_time = start_time;
    paused_time = start_time;
    paused_duration = std::chrono::duration<long long, std::nano>(0);
    rtAbsLoopCount = 0.f;

    updateInternals();
    //timeRamps.updateRamps(counters.playhead, timeSignature, duration);
}

// Load + Save
#if ofxPH_XML_ENGINE == ofxPH_XML_ENGINE_PUGIXML
bool ofxPlayhead::populateXmlNode(pugi::xml_node &_node){

    // FPS
    _node.append_child("fps").text().set(fps);

    // Duration
    _node.append_child("duration").text().set(duration);

    // LoopMode
    _node.append_child("loop_mode").text().set(loopMode);

    // PlaybackMode
    _node.append_child("play_mode").text().set(playbackMode);

    _node.append_child("play_speed").text().set(playSpeed);

    // Timesignature
    pugi::xml_node tsNode = _node.append_child("time_signature");
    tsNode.append_attribute("bpm").set_value(timeSignature.bpm);
    tsNode.append_attribute("beats_per_bar").set_value(timeSignature.beatsPerBar);
    tsNode.append_attribute("notes_per_beat").set_value(timeSignature.notesPerBeat);

    // Advanced
    _node.append_child("allow_lossy_operations").text().set(bAllowLossyOperations);

    return true;
}

bool ofxPlayhead::retrieveXmlNode(pugi::xml_node &_node){
    bool ret = true;

    // FPS
    if(pugi::xml_node fpsNode = _node.child("fps")){
        setFps(fpsNode.text().as_uint(fps));
    }
    else ret = false;

    // Duration
    if(pugi::xml_node dNode = _node.child("duration")){
        setDuration(dNode.text().as_double(duration));
    }
    else ret = false;

    // LoopMode
    if(pugi::xml_node lNode = _node.child("loop_mode")){
        setLoop(static_cast<ofxPlayheadLoopMode>(lNode.text().as_int(loopMode)));
    }
    else ret = false;

    // PlaybackMode
    if(pugi::xml_node fpsNode = _node.child("play_mode")){
        setPlayMode(static_cast<ofxPlayheadMode>(fpsNode.text().as_uint(playbackMode)));
    }
    else ret = false;

    if(pugi::xml_node sNode = _node.child("play_speed")){
        setPlaySpeed(sNode.text().as_double(playSpeed));
    }
    else ret = false;

    // Todo: Playhead position ?

    // Timesignature
    if(pugi::xml_node tsNode = _node.child("time_signature")){
        timeSignature.set(
            tsNode.attribute("beats_per_bar").as_uint(timeSignature.beatsPerBar),
            tsNode.attribute("notes_per_beat").as_uint(timeSignature.notesPerBeat),
            tsNode.attribute("bpm").as_uint(timeSignature.bpm)
        );
    }
    else ret = false;

    // Advanced
    if(pugi::xml_node aloNode = _node.child("allow_lossy_operations")){
        bAllowLossyOperations = aloNode.text().as_bool(bAllowLossyOperations);
    }
    else ret = false;

    reset();

    return ret;
}

// Resets/Cancels-out the paused_duration without moving the playhead
void ofxPlayhead::resetPausedDuration(){
    if(paused_duration.count()==0) return; // nothing to reset

    // rtAbs is the only one to use paused_duration
    const auto pd = (playbackMode == ofxPlayheadMode_RealTime_Absolute) ? paused_duration : std::chrono::duration<long long, std::nano>(0);

    // In rt modes we compensate the paused duration by changing the start time; to prevent the playhead from moving
    start_time += pd;//std::chrono::high_resolution_clock::now() - pd - std::chrono::nanoseconds((long long)(counters.playhead*1000000000.0));
    paused_duration = std::chrono::duration<long long, std::nano>(0);

}
#endif
