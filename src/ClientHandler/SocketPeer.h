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
#ifndef SOCKETPEER_H_
#define SOCKETPEER_H_

#include "MessageFactory/SocketReader.h"
#include "MessageFactory/SocketWriter.h"
#include "MessageFactory/Message.h"
#include "Platform/Socket/Socket.h"
#include "Platform/Threads/Mutex.h"
#include <queue>

class SocketPeer
{
private:
    unsigned int requestId;

    SocketReader reader_;
    SocketWriter writer_;

    virtual void processMessage(const Message* msg) = 0;

    Platform::Mutex messageQueueMtx;
    std::queue<Message*> messageQueue;

    Socket* socket_;

protected:
    void queueRequest(Message* msg);
    void queueMessage(Message* msg);
    Message* popMessage();

public:
    SocketPeer( Socket* socket );
    virtual ~SocketPeer();

    int doRead();
    int doWrite();

    bool pendingSend();
    Socket* getSocket() const;

};

#endif /* SOCKETPEER_H_ */
