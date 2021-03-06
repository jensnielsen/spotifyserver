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

#ifndef SPOTIFYPLAYBACKHANDLER_H_
#define SPOTIFYPLAYBACKHANDLER_H_

#include "MediaContainers/Track.h"
#include "MediaContainers/Folder.h"
#include "MediaContainers/Playlist.h"
#include "MediaContainers/Album.h"
#include "Platform/Threads/Mutex.h"
#include <deque>
#include "CircularQueue.h"


namespace LibSpotify
{
typedef std::deque<Track> TrackQueue;

class LibSpotifyPlaybackHandler
{
public:
    typedef enum
    {
        NONE,
        TRACK,
        FOLDER,
        PLAYLIST,
        ALBUM,
        SEARCH
    }CurrentlyPlayingFromType;
private:
    class LibSpotifyIf& libSpotifyIf_;

    Platform::Mutex mtx_;
    /*Contains all tracks that are enqued */
    TrackQueue enquedQueue_;

    #define HISTORY_QUEUE_DEPTH 50
    Util::CircularQueue<Track> historyQueue_;

    std::string currentlyPlayingFromName_;
    std::string currentlyPlayingFromUri_;
    CurrentlyPlayingFromType currentlyPlayingFromType_;

    TrackQueue::iterator playQueueIter_;
    TrackQueue playQueue_;

    bool isShuffle;
    bool isRepeat;

    void loadPlaylist(const Playlist& playlist, int startIndex);
    void shuffle();

public:
    LibSpotifyPlaybackHandler(class LibSpotifyIf& spotifyIf);
    virtual ~LibSpotifyPlaybackHandler();

    void playTrack(const Track& track);
    void playFolder(const Folder& folder);
    void playPlaylist(const Playlist& playlist, int startIndex = -1);
    void playAlbum(const Album& album, int startIndex = -1);
    void playSearchResult(const std::string& searchString,
                          const std::string& searchLink,
                          const TrackQueue& searchResult);
    void enqueueTrack(const Track& track);

    void playNext();
    void playPrevious();

    void rootFolderUpdatedInd();
    void trackEndedInd();
    void setShuffle(bool shuffleOn);
    void setRepeat(bool repeatOn);
    bool getShuffle();
    bool getRepeat();
};

} /* namespace LibSpotify */
#endif /* SPOTIFYPLAYBACKHANDLER_H_ */
