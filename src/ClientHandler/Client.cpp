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

#include "Client.h"
#include "applog.h"
#include "MessageFactory/TlvDefinitions.h"

Client::Client(Socket* socket, MediaInterface& spotifyif) : SocketPeer(socket),
                                                            spotify_(spotifyif),
                                                            loggedIn_(true),
                                                            networkUsername_(""),
                                                            networkPassword_(""),
                                                            audioEp(NULL),
                                                            reqId_(0)
{
    spotify_.registerForCallbacks(*this);
}

Client::~Client()
{
    log(LOG_DEBUG) << "~Client";
    spotify_.unRegisterForCallbacks(*this);
    /*todo clear pendingMessageMap_*/
    /*todo unregister and free audioEp*/
}

void Client::setUsername(std::string username) { networkUsername_ = username; }
void Client::setPassword(std::string password) { networkPassword_ = password; }

void Client::processMessage(const Message* msg)
{
    log(LOG_NOTICE) << *(msg);

    /*require login before doing anything else (??if either username or password is set, for backward compatibility??)*/
    if ( /*(!networkUsername_.empty() || !networkPassword_.empty()) && */!loggedIn_ && msg->getType() != HELLO_REQ )
    {
        return;
    }

    switch(msg->getType())
    {
        case HELLO_REQ:          handleHelloReq(msg);         break;
        case GET_PLAYLISTS_REQ:  handleGetPlaylistsReq(msg);  break;
        case GET_TRACKS_REQ:     handleGetTracksReq(msg);     break;
        case PLAY_REQ:           handlePlayReq(msg);          break;
        case PLAY_TRACK_REQ:     handlePlayTrackReq(msg);     break;
        case PLAY_CONTROL_REQ:   handlePlayControlReq(msg);   break;
        case GENERIC_SEARCH_REQ: handleGenericSearchReq(msg); break;
        case GET_STATUS_REQ:     handleGetStatusReq(msg);     break;
        case GET_IMAGE_REQ:      handleGetImageReq(msg);      break;
        case GET_ALBUM_REQ:      handleGetAlbumReq(msg);      break;
        case ADD_AUDIO_ENDPOINT_REQ: handleAddAudioEpReq(msg); break;

        default:
            break;
    }
}

void Client::connectionState( bool up )
{}
void Client::rootFolderUpdatedInd()
{
    log(LOG_DEBUG) << "Client::rootFolderUpdatedInd()";
}
static void addStatusMsgMandatoryParameters( Message* msg, PlaybackState_t state, bool repeatStatus, bool shuffleStatus )
{
    msg->addTlv( TLV_STATE, state );
    msg->addTlv( TLV_PLAY_MODE_REPEAT, repeatStatus );
    msg->addTlv( TLV_PLAY_MODE_SHUFFLE, shuffleStatus );
}
static void addStatusMsgOptionalParameters( Message* msg, const Track& currentTrack, unsigned int progress )
{
    msg->addTlv( currentTrack.toTlv() );
    msg->addTlv( TLV_PROGRESS, progress );
}

void Client::statusUpdateInd( PlaybackState_t state, bool repeatStatus, bool shuffleStatus, const Track& currentTrack, unsigned int progress )
{
    Message* msg = new Message(STATUS_IND);

    addStatusMsgMandatoryParameters( msg, state, repeatStatus, shuffleStatus );
    addStatusMsgOptionalParameters( msg, currentTrack, progress );

    queueMessage( msg, reqId_++ );
}

void Client::statusUpdateInd( PlaybackState_t state, bool repeatStatus, bool shuffleStatus )
{
    Message* msg = new Message(STATUS_IND);

    addStatusMsgMandatoryParameters( msg, state, repeatStatus, shuffleStatus );

    queueMessage( msg, reqId_++ );
}

void Client::getStatusResponse( MediaInterfaceRequestId reqId, PlaybackState_t state, bool repeatStatus, bool shuffleStatus, const Track& currentTrack, unsigned int progress )
{
    PendingMessageMap::iterator msgIt = pendingMessageMap_.find(reqId);
    if (msgIt != pendingMessageMap_.end())
    {
        Message* msg = msgIt->second;

        addStatusMsgMandatoryParameters( msg, state, repeatStatus, shuffleStatus );
        addStatusMsgOptionalParameters( msg, currentTrack, progress );

        queueResponse( msg, reqId );
        pendingMessageMap_.erase(msgIt);
    }
    else log(LOG_WARN) << "Could not match the getStatusResponse() to a pending response";

}
void Client::getStatusResponse( MediaInterfaceRequestId reqId, PlaybackState_t state, bool repeatStatus, bool shuffleStatus )
{
    PendingMessageMap::iterator msgIt = pendingMessageMap_.find(reqId);
    if (msgIt != pendingMessageMap_.end())
    {
        Message* msg = msgIt->second;

        addStatusMsgMandatoryParameters( msg, state, repeatStatus, shuffleStatus );

        queueResponse( msg, reqId );
        pendingMessageMap_.erase(msgIt);
    }
    else log(LOG_WARN) << "Could not match the getStatusResponse() to a pending response";
}


void Client::getPlaylistsResponse( MediaInterfaceRequestId reqId, const Folder& rootfolder )
{
    PendingMessageMap::iterator msgIt = pendingMessageMap_.find(reqId);
    if (msgIt != pendingMessageMap_.end())
    {
        Message* msg = msgIt->second;

        /*get playlist*/
        msg->addTlv( rootfolder.toTlv() );

        queueResponse( msg, reqId );
        pendingMessageMap_.erase(msgIt);
    }
    else log(LOG_WARN) << "Could not match the getPlaylistsResponse() to a pending response";
}
void Client::getTracksResponse(MediaInterfaceRequestId reqId, const std::deque<Track>& tracks)
{
    log(LOG_DEBUG) << "Client::getTracksResponse()";

    PendingMessageMap::iterator msgIt = pendingMessageMap_.find(reqId);
    if (msgIt != pendingMessageMap_.end())
    {
        Message* msg = msgIt->second;
        for (std::deque<Track>::const_iterator trackIt = tracks.begin(); trackIt != tracks.end(); trackIt++)
        {
            log(LOG_DEBUG) << "\t" << (*trackIt).getName();
            msg->addTlv((*trackIt).toTlv());
        }
        log(LOG_DEBUG) << "#tracks found=" << tracks.size();

        queueResponse( msg, reqId );
        pendingMessageMap_.erase(msgIt);
    }
    else log(LOG_WARN) << "Could not match the getTracksResponse() to a pending response";
}

void Client::getAlbumResponse(MediaInterfaceRequestId reqId, const Album& album)
{
    log(LOG_DEBUG) << "Client::getAlbumResponse()";

    PendingMessageMap::iterator msgIt = pendingMessageMap_.find(reqId);
    if (msgIt != pendingMessageMap_.end())
    {
        Message* msg = msgIt->second;
        TlvContainer* albumTlv = album.toTlv();
        const std::deque<Track>& tracks = album.getTracks();

        for (std::deque<Track>::const_iterator trackIt = tracks.begin(); trackIt != tracks.end(); trackIt++)
        {
            log(LOG_DEBUG) << "\t" << (*trackIt).getName();
            albumTlv->addTlv((*trackIt).toTlv());
        }
        log(LOG_DEBUG) << "#tracks found=" << tracks.size();

        msg->addTlv(albumTlv);

        queueResponse( msg, reqId );
        pendingMessageMap_.erase(msgIt);
    }
    else log(LOG_WARN) << "Could not match the getAlbumResponse() to a pending response";
}

void Client::getImageResponse(MediaInterfaceRequestId reqId, const void* data, size_t dataSize)
{
    log(LOG_DEBUG) << "Client::getImageResponse() " << dataSize << " bytes";
    PendingMessageMap::iterator msgIt = pendingMessageMap_.find(reqId);
    if (msgIt != pendingMessageMap_.end())
    {
        Message* msg = msgIt->second;
        if(data && dataSize)
        {
            TlvContainer* image = new TlvContainer(TLV_IMAGE);
            image->addTlv(TLV_IMAGE_FORMAT, IMAGE_FORMAT_JPEG);
            image->addTlv(new BinaryTlv(TLV_IMAGE_DATA, (const uint8_t*)data, (uint32_t)dataSize));
            msg->addTlv(image);
        }
        queueResponse( msg, reqId );
        pendingMessageMap_.erase(msgIt);
    }
    else log(LOG_WARN) << "Could not match the getImageResponse() to a pending response";

}

void Client::genericSearchCallback(MediaInterfaceRequestId reqId, const std::deque<Track>& tracks, const std::string& didYouMean)
{
    log(LOG_DEBUG) << "\tdid you mean:" << didYouMean;

    PendingMessageMap::iterator msgIt = pendingMessageMap_.find(reqId);
    if (msgIt != pendingMessageMap_.end())
    {
        Message* msg = msgIt->second;
        for (std::deque<Track>::const_iterator trackIt = tracks.begin(); trackIt != tracks.end(); trackIt++)
        {
            log(LOG_DEBUG) << "\t" << (*trackIt).getName();
            msg->addTlv((*trackIt).toTlv());
        }
        log(LOG_DEBUG) << "#tracks found=" << tracks.size();

        queueResponse( msg, reqId );
        pendingMessageMap_.erase(msgIt);
    }
    else log(LOG_WARN) << "Could not match the genericSearchCallback() to a pending response";
}

void Client::handleGetTracksReq(const Message* msg)
{
    GetTracksReq* req = (GetTracksReq*)msg;
    log(LOG_DEBUG) << "get tracks: " << req->getPlaylist();

    Message* rsp  = new Message(GET_TRACKS_RSP);
    unsigned int headerId = req->getId();
    pendingMessageMap_[headerId] = rsp;

    spotify_.getTracks( req->getPlaylist(), this, headerId );
}

void Client::handleHelloReq(const Message* msg)
{
    Message* rsp = new Message(HELLO_RSP);
    const IntTlv* protoMajorTlv = (const IntTlv*)msg->getTlvRoot()->getTlv(TLV_PROTOCOL_VERSION_MAJOR);
    const IntTlv* protoMinorTlv = (const IntTlv*)msg->getTlvRoot()->getTlv(TLV_PROTOCOL_VERSION_MINOR);

    if ( protoMajorTlv == NULL || protoMinorTlv == NULL )
    {
        /*we probably need to know the protocol version*/
        rsp->addTlv(TLV_FAILURE, FAIL_MISSING_TLV);
    }
    else
    {
        peerProtocolMajor_ = protoMajorTlv->getVal();
        peerProtocolMinor_ = protoMinorTlv->getVal();

        if (peerProtocolMajor_ != PROTOCOL_VERSION_MAJOR)
        {
            /*not compatible if major version differ, we accept differences in minor even though functionality might be missing*/
            rsp->addTlv(TLV_FAILURE, FAIL_PROTOCOL_MISMATCH);
        }
        else
        {
            const StringTlv* usernameTlv = (const StringTlv*)msg->getTlvRoot()->getTlv(TLV_LOGIN_USERNAME);
            const StringTlv* passwordTlv = (const StringTlv*)msg->getTlvRoot()->getTlv(TLV_LOGIN_PASSWORD);
            std::string username = usernameTlv ? usernameTlv->getString() : std::string("");
            std::string password = passwordTlv ? passwordTlv->getString() : std::string("");
            if ( (!networkUsername_.empty() && networkUsername_ != username) ||
                 (!networkPassword_.empty() && networkPassword_ != password)    )
            {
                /*incorrect login*/
                log(LOG_DEBUG) << networkUsername_.empty() << " " << networkUsername_ << " " << username;
                log(LOG_DEBUG) << networkPassword_.empty() << " " << networkPassword_ << " " << password;
                rsp->addTlv(TLV_FAILURE, FAIL_BAD_LOGIN);
            }
            else
            {
                /*all is well!*/
                loggedIn_ = true;
            }
        }
    }
    rsp->addTlv(TLV_PROTOCOL_VERSION_MAJOR, PROTOCOL_VERSION_MAJOR);
    rsp->addTlv(TLV_PROTOCOL_VERSION_MINOR, PROTOCOL_VERSION_MINOR);

    queueResponse( rsp, msg );
}

void Client::handleGetPlaylistsReq(const Message* msg)
{
    unsigned int headerId = msg->getId();
    Message* rsp = new Message(GET_PLAYLISTS_RSP);

    /* make sure that the pending message queue does not already contain such a message */
    if (pendingMessageMap_.find(headerId) == pendingMessageMap_.end())
    {
        pendingMessageMap_[headerId] = rsp;
        spotify_.getPlaylists( this, headerId );
    }
    else
    {
        delete rsp;
    }
}

void Client::handleGetStatusReq(const Message* msg)
{
    unsigned int headerId = msg->getId();
    Message* rsp = new Message(GET_STATUS_RSP);

    /* make sure that the pending message queue does not already contain such a message */
    if (pendingMessageMap_.find(headerId) == pendingMessageMap_.end())
    {
        pendingMessageMap_[headerId] = rsp;
        spotify_.getStatus( this, headerId );
    }
    else
    {
        delete rsp;
    }
}

void Client::handlePlayReq(const Message* msg)
{
    const StringTlv* url = (const StringTlv*)msg->getTlvRoot()->getTlv(TLV_LINK);

    if ( url )
    {
        const IntTlv* startIndex = (const IntTlv*)msg->getTlvRoot()->getTlv(TLV_TRACK_INDEX);
        log(LOG_DEBUG) << "spotify_.play(" << url->getString() << ")";
        if ( startIndex != NULL )
        {
            spotify_.play( url->getString(), startIndex->getVal(), this, msg->getId() );
        }
        else
        {
            spotify_.play( url->getString(), this, msg->getId() );
        }
    }

    Message* rsp = new Message( PLAY_RSP );
    queueResponse( rsp, msg );
}

void Client::handlePlayTrackReq(const Message* msg)
{
    const TlvContainer* track = (const TlvContainer*) msg->getTlvRoot()->getTlv(TLV_TRACK);
    const StringTlv* url = (const StringTlv*) track->getTlv(TLV_LINK);
    log(LOG_DEBUG) << "spotify_.play(" << url->getString() << ")";
    spotify_.play( url->getString(), this, msg->getId() );

    Message* rsp = new Message( PLAY_TRACK_RSP );
    queueResponse( rsp, msg );
}

void Client::handlePlayControlReq(const Message* msg)
{
    for (TlvContainer::const_iterator it = msg->getTlvRoot()->begin() ; it != msg->getTlvRoot()->end() ; it++)
    {
        Tlv* tlv = (*it);
        log(LOG_DEBUG) << *tlv;

        switch(tlv->getType())
        {
            case TLV_PLAY_OPERATION:
            {
                switch(((IntTlv*)tlv)->getVal())
                {
                    case PLAY_OP_NEXT:
                        spotify_.next();
                        break;
                    case PLAY_OP_PREV:
                        spotify_.previous();
                        break;
                    case PLAY_OP_PAUSE:
                        spotify_.pause();
                        break;
                    case PLAY_OP_RESUME:
                        spotify_.resume();
                        break;
                    default:
                        break;
                }
            }
            break;

            case TLV_PLAY_MODE_SHUFFLE:
            {
                spotify_.setShuffle( (((IntTlv*)tlv)->getVal() != 0) );
            }
            break;

            case TLV_PLAY_MODE_REPEAT:
            {
                spotify_.setRepeat( (((IntTlv*)tlv)->getVal() != 0) );
            }
            break;

            default:
                break;
        }
    }
    Message* rsp = new Message( PLAY_CONTROL_RSP );
    queueResponse( rsp, msg );
}


void Client::handleGetImageReq(const Message* msg)
{
    const StringTlv* link = (const StringTlv*) msg->getTlvRoot()->getTlv(TLV_LINK);

    unsigned int headerId = msg->getId();
    Message* rsp = new Message(GET_IMAGE_RSP);

    /* make sure that the pending message queue does not already contain such a message */
    if (pendingMessageMap_.find(headerId) == pendingMessageMap_.end())
    {
        pendingMessageMap_[headerId] = rsp;
        spotify_.getImage( link ? link->getString() : std::string(""), this, headerId );
    }
    else
    {
        delete rsp;
    }
}

void Client::handleGenericSearchReq(const Message* msg)
{
    const StringTlv* query = (const StringTlv*) msg->getTlvRoot()->getTlv(TLV_SEARCH_QUERY);

    if (query && query->getString() != "")
    {
        unsigned int headerId = msg->getId();
        Message* rsp = new Message(GENERIC_SEARCH_RSP);

        /* make sure that the pending message queue does not already contain such a message */
        if (pendingMessageMap_.find(headerId) == pendingMessageMap_.end())
        {
            pendingMessageMap_[headerId] = rsp;
            spotify_.search( query->getString(), this, headerId );
        }
        else
        {
            delete rsp;
        }
    }
    else
    {
        /*error handling?*/
    }
}

void Client::handleGetAlbumReq(const Message* msg)
{
    const StringTlv* link = (const StringTlv*) msg->getTlvRoot()->getTlv(TLV_LINK);

    unsigned int headerId = msg->getId();
    Message* rsp = new Message(GET_ALBUM_RSP);

    /* make sure that the pending message queue does not already contain such a message */
    if (pendingMessageMap_.find(headerId) == pendingMessageMap_.end())
    {
        pendingMessageMap_[headerId] = rsp;
        spotify_.getAlbum( link ? link->getString() : std::string(""), this, headerId );
    }
    else
    {
        delete rsp;
    }
}


void Client::handleAddAudioEpReq(const Message* msg)
{
    /*audioEp = new Platform::AudioEndpointRemote();
    spotify_.setAudioEndpoint(audioEp);*/
}



