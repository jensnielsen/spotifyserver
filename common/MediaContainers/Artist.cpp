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

#include "Artist.h"

namespace LibSpotify
{

Artist::Artist(const char* name) : name_(name), link_("") { }
Artist::Artist(const TlvContainer* tlv)
{
    const StringTlv* tlvName = (const StringTlv*) tlv->getTlv(TLV_NAME);
    const StringTlv* tlvLink = (const StringTlv*) tlv->getTlv(TLV_LINK);
    name_ = (tlvName ? tlvName->getString() : "no-name");
    link_ = (tlvLink ? tlvLink->getString() : "no-link");
}
Artist::~Artist() { }

const std::string& Artist::getName() const { return name_; }

const std::string& Artist::getLink() const { return link_; }
void Artist::setLink(const std::string& link){ link_ = link; }

Tlv* Artist::toTlv() const
{
    TlvContainer* artist = new TlvContainer( TLV_ARTIST );

    artist->addTlv(TLV_NAME, name_);
    artist->addTlv(TLV_LINK, link_);

    return artist;
}

} /* namespace LibSpotify */
