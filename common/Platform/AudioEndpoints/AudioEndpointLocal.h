/*
 * Copyright (c) 2012, Jens Nielsen
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
 * DISCLAIMED. IN NO EVENT SHALL JENS NIELSEN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AUDIOENDPOINTLOCAL_H_
#define AUDIOENDPOINTLOCAL_H_

#include "AudioEndpoint.h"
#include "ConfigHandling/ConfigHandler.h"
#include "../Threads/Runnable.h"
#include <map>

namespace Platform {

class AudioEndpointLocal : public AudioEndpoint, Runnable
{
private:
    ConfigHandling::AudioEndpointConfig config_;

    int adjustSamples_;
    void adjustSamples( AudioFifoData* afd );

    uint8_t actualVolume_;

    class AudioEndpointCounters : public Counters
    {
        typedef enum
        {
            PACKETS_RECEIVED,
            NROF_COUNTERS,
        } AudioEndpointCounterTypes;
        std::map<AudioEndpointCounterTypes, uint32_t> counters;
        void increment(AudioEndpointCounterTypes counter) { counters[counter]++; }

        virtual uint8_t getNrofCounters() const { return NROF_COUNTERS; }
        virtual std::string getCounterName(uint8_t counter) const { return ""; }
        virtual uint32_t getCounterValue(uint8_t counter) const { return counters.at((AudioEndpointCounterTypes)counter); }
    public:
        AudioEndpointCounters() { for(uint8_t i = 0; i < NROF_COUNTERS; i++) counters[(AudioEndpointCounterTypes)i] = 0; }
    };

    class AudioEndpointCounters counters;

public:
    AudioEndpointLocal(const ConfigHandling::AudioEndpointConfig& config, const EndpointIdIf& epId );
    virtual ~AudioEndpointLocal();

    virtual int enqueueAudioData( unsigned int timestamp, unsigned short channels, unsigned int rate, unsigned int nsamples, const int16_t* samples );
    virtual void flushAudioData();

    virtual unsigned int getNumberOfQueuedSamples();

    virtual void setMasterVolume( uint8_t volume );
    virtual void doSetRelativeVolume( uint8_t volume );

    virtual const Counters& getStatistics();

    virtual void run();
    virtual void destroy();
    virtual bool isLocal() const {return true;};
};

}
#endif /* AUDIOENDPOINTLOCAL_H_ */
