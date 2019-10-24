#pragma once

#include <string>
#include <list>
#include "AvCodec.h"

using namespace std;

namespace ofxFFmpeg {

    list<int> getOutputFormats(bool videoFormats = true, bool audioFormats = true);
    
    int getVideoEncoder(string name);
    string getCodecName(int codecId);
    string getCodecLongName(int codecId);
    AvCodecPtr getEncoder(int codecId);
}
