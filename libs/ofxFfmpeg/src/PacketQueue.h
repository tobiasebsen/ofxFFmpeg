#pragma once

#include <deque>
#include <stdint.h>
#include <mutex>

#include "Queue.h"
#include "Reader.h"

struct AVPacket;

namespace ofxFFmpeg {

    class PacketQueue : public Queue<AVPacket>, public PacketSupplier {
	public:
        AVPacket * clone(AVPacket * p);
        void free(AVPacket * p);

        AVPacket * supplyPacket();
	};

}
