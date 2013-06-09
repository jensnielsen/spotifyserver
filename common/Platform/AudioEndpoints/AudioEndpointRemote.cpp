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

#include "AudioEndpointRemote.h"
#include "MessageFactory/Message.h"
#include <stdlib.h>

#include <iostream>
#include <errno.h>


namespace Platform {

AudioEndpointRemote::AudioEndpointRemote( const std::string& id, const std::string& serveraddr, 
                                          const std::string& serverport, unsigned int bufferNSecs) : sock_(SOCKTYPE_DATAGRAM),
                                                                                                     id_(id)
{
    fifo_.setFifoBuffer(bufferNSecs);
    sock_.Connect(serveraddr, serverport);
}

std::string AudioEndpointRemote::getId() const
{
    return id_;
}

void AudioEndpointRemote::sendAudioData()
{
    AudioFifoData* afd;

    while( (afd = fifo_.getFifoDataTimedWait(10) ) != NULL )
    {
        int rc = 0;
        Message* msg = new Message( AUDIO_DATA_IND );
        msg->addTlv( TLV_AUDIO_CHANNELS, afd->channels );
        msg->addTlv( TLV_AUDIO_RATE, afd->rate );
        msg->addTlv( TLV_AUDIO_NOF_SAMPLES, afd->nsamples );
        msg->addTlv( new BinaryTlv( TLV_AUDIO_DATA, (const uint8_t*) /*ugh, should hton this*/ afd->samples, afd->nsamples * sizeof(int16_t) * afd->channels ) );
        MessageEncoder* enc = msg->encode();

        if((rc = sock_.Send(enc->getBuffer(), enc->getLength()) < 0))
        {
            /* TODO: its probably NOT ok to use errno here, doubt its valid on WIN */
            std::cout << "JESPER: rc=" << rc << " perror=" << errno;
        }

        delete enc;
        free( afd );
    }

}

int AudioEndpointRemote::enqueueAudioData(unsigned short channels, unsigned int rate, unsigned int nsamples, const int16_t* samples)
{
    if (nsamples == 0)
        return 0; // Audio discontinuity, do nothing

    nsamples = fifo_.addFifoDataBlocking(channels, rate, nsamples, samples);

    sendAudioData();

    return nsamples;
}

void AudioEndpointRemote::flushAudioData()
{
}


}
