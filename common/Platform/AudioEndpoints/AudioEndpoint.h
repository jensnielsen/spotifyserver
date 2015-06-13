/*
 * Copyright (c) 2012, Jesper Derehag
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL JESPER DEREHAG BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AUDIOENDPOINT_H_
#define AUDIOENDPOINT_H_

#include "AudioFifo.h"
#include "EndpointId/EndpointId.h"
#include "Utils/ActionFilter.h"
#include <string>

class Counters
{
public:
    virtual uint8_t getNrofCounters() const = 0;
    virtual std::string getCounterName(uint8_t counter) const = 0;
    virtual uint32_t getCounterValue(uint8_t counter) const = 0;
};


namespace Platform {
static void setRelativeVolumeCb( void* arg, uint32_t volume );

class AudioEndpoint
{
private:
    const EndpointIdIf& epId_;

protected:
    AudioFifo fifo_;

    bool paused_;

    uint8_t masterVolume_;
    uint8_t relativeVolume_;

    ActionFilter setVolumeFilter;

public:
    AudioEndpoint(const EndpointIdIf& epId, uint8_t volume = 255, bool dynamicFifo = true) : epId_(epId), fifo_(dynamicFifo), paused_(false), relativeVolume_(volume), setVolumeFilter(100, setRelativeVolumeCb, this) {}
    virtual ~AudioEndpoint() {}
    AudioFifoData* getFifoDataBuffer( size_t length ) { return fifo_.getFifoDataBuffer( length ); }
    void returnFifoDataBuffer(AudioFifoData* afd) { fifo_.returnFifoDataBuffer(afd); }
    virtual int enqueueAudioData( unsigned int timestamp, unsigned short channels, unsigned int rate, unsigned int nsamples, const int16_t* samples ) = 0;
    int enqueueAudioData( AudioFifoData* afd ) { return fifo_.addFifoDataBlocking(afd); }
    virtual void flushAudioData() = 0;

    virtual unsigned int canAcceptSamples( unsigned int availableSamples, unsigned int rate ) { return fifo_.canAcceptSamples( availableSamples, rate ); }
    virtual unsigned int getNumberOfQueuedSamples() = 0;

    virtual void setMasterVolume( uint8_t volume ) = 0;
    void setRelativeVolume( uint8_t volume ) { setVolumeFilter.Event(volume); }
    virtual void doSetRelativeVolume( uint8_t volume ) = 0;
    uint8_t getRelativeVolume() { return relativeVolume_; }
    const std::string& getId() const { return epId_.getId(); };

    virtual const Counters& getStatistics() = 0;

    /*todo do something proper with these...*/
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }

    virtual bool isLocal() const = 0;

};

static void setRelativeVolumeCb( void* arg, uint32_t volume )
{
    AudioEndpoint* this_ = static_cast<AudioEndpoint*>( arg );
    this_->doSetRelativeVolume( volume );
}
}
#endif /* AUDIOENDPOINT_H_ */
