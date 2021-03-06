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

#include "../Runnable.h"

#include "FreeRTOS.h"
#include "task.h"


namespace Platform
{

typedef struct ThreadHandle_t
{
    xTaskHandle handle;
}ThreadHandle_t;

/* Wrapper for pointing out the correct instance for the runnable implementation */

static void runnableWrapper( void* arg )
{
    Runnable* runnable = reinterpret_cast<Runnable*> (arg);
    runnable->run();
    /*vTaskDelete?*/
    while(1); /*a freertos task must never return*/
}


Runnable::Runnable(bool isJoinable, Size size, Prio prio) : isCancellationPending_(false),
                                                            isJoinable_(isJoinable),
                                                            size_(size),
                                                            prio_(prio)
{
	threadHandle_ = new ThreadHandle_t;
}

Runnable::~Runnable()
{
//	pthread_mutex_destroy(&threadHandle_->cancellationMutex_);
	delete threadHandle_;
}


void Runnable::startThread()
{
    unsigned int stackdepth;
    unsigned int prio;
    switch(size_)
    {
        case SIZE_SMALL:  stackdepth = 500; break;
        case SIZE_MEDIUM: stackdepth = 1200; break;
        case SIZE_LARGE:  stackdepth = 1800; break;
    }
    switch(prio_)
    {
        case PRIO_LOW:  prio = 1; break;
        case PRIO_MID:  prio = 2; break;
        case PRIO_HIGH: prio = 3; break;
    }
    xTaskCreate( runnableWrapper, ( signed char * ) "tsk", stackdepth, this, tskIDLE_PRIORITY + prio, &threadHandle_->handle );
}

void Runnable::joinThread()
{

}

bool Runnable::isCancellationPending()
{
    return isCancellationPending_;
}
void Runnable::cancelThread()
{
	isCancellationPending_ = true;
}


}
