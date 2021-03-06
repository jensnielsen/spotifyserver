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

#ifndef MESSAGEENCODER_H_
#define MESSAGEENCODER_H_

#include "MessageFactory/TlvDefinitions.h"
#include <string>
#include <stdint.h>


typedef struct {
	uint32_t len;
	uint32_t type;
	uint32_t id;
}header_t;

typedef struct {
	uint32_t type;
	uint32_t len;
}tlvheader_t;

typedef struct tlvgroup_s tlvgroup_t;


class MessageEncoder {
private:
	char* msgbuf;
	unsigned int cursize;
	unsigned int wpos;
	void init();
public:
	MessageEncoder();
	MessageEncoder(MessageType_t type);
	MessageEncoder(const MessageEncoder& from);
	virtual ~MessageEncoder();

	const char* getBuffer() const;
	unsigned int getLength() const;

	void setId(unsigned int id);
	header_t* getHeader();

	char* getBufferForTlv(unsigned int size);
	void encode(TlvType_t tlv, unsigned int val);
	void encode(TlvType_t tlv, const std::string & str);
    void encode(TlvType_t tlv, const uint8_t* data, uint32_t len);
	tlvgroup_t* createNewGroup(TlvType_t tlv);
	void finalizeGroup(tlvgroup_t* group);
	void finalize();

	void printHex();
};

void printHexMsg(const uint8_t* msgbuf, uint32_t len);

#endif /* MESSAGEENCODER_H_ */
