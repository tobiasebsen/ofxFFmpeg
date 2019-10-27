#pragma once

#include "Player.h"
#include "Recorder.h"

#include <string>
#include <list>

using namespace std;

namespace ofxFFmpeg {

    list<int> getOutputFormats(bool videoFormats = true, bool audioFormats = true);
    
    int getVideoEncoder(string name);
    string getCodecName(int codecId);
    string getCodecLongName(int codecId);
}
