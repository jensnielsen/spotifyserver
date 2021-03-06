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


#ifndef UIEMBEDDED_H_
#define UIEMBEDDED_H_

#include "MediaInterface/MediaInterface.h"
#include "SocketHandling/Messenger.h"

using namespace LibSpotify;

class UIEmbedded : public IMediaInterfaceCallbackSubscriber
{
private:
    MediaInterface& m_;
    unsigned int reqId_;

    PlaybackState_t playbackState;

    PlaylistContainer::iterator itPlaylists_;
    PlaylistContainer playlists;

    /* Implements IMediaInterfaceCallbackSubscriber */
    virtual void connectionState( bool up );
    virtual void rootFolderUpdatedInd();
    virtual void statusUpdateInd( PlaybackState_t state, bool repeatStatus, bool shuffleStatus, const Track& currentTrack, unsigned int progress );
    virtual void statusUpdateInd( PlaybackState_t state, bool repeatStatus, bool shuffleStatus );

    virtual void getPlaylistsResponse( MediaInterfaceRequestId reqId, const Folder& rootfolder );
    virtual void getTracksResponse( MediaInterfaceRequestId reqId, const std::deque<Track>& tracks );
    virtual void getImageResponse( MediaInterfaceRequestId reqId, const void* data, size_t dataSize );
    virtual void getAlbumResponse( MediaInterfaceRequestId reqId, const Album& album );
    virtual void genericSearchCallback( MediaInterfaceRequestId reqId, const std::deque<Track>& listOfTracks, const std::string& didYouMean);
    virtual void getStatusResponse( MediaInterfaceRequestId reqId, PlaybackState_t state, bool repeatStatus, bool shuffleStatus, const Track& currentTrack, unsigned int progress );
    virtual void getStatusResponse( MediaInterfaceRequestId reqId, PlaybackState_t state, bool repeatStatus, bool shuffleStatus );

public:
    UIEmbedded(MediaInterface& m);
    virtual ~UIEmbedded();

    void shortButtonPress();
    void longButtonPress();
};


#endif /* UIEMBEDDED_H_ */
