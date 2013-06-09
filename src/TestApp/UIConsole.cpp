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

#include "UIConsole.h"
#include "applog.h"

static void printFolder( const Folder& f, int indent );
static void printTracks( const std::deque<Track>& tracks );


UIConsole::UIConsole( MediaInterface& m ) : m_(m),
                                            itPlaylists_(playlists.begin()),
                                            isShuffle(false),
                                            isRepeat(false)
{
    m_.registerForCallbacks( *this );
    startThread();
}
UIConsole::~UIConsole()
{
}

void UIConsole::destroy()
{
    m_.unRegisterForCallbacks( *this );
    cancelThread();
    joinThread();
}


void UIConsole::run()
{
    char c;
    std::string cmd;

    while(isCancellationPending() == false)
    {
        std::vector<std::string> argv;
        std::string arg;

        std::cout << "'g' get playlists\n"
                     "'t' get tracks\n"
                     "'a' get album\n"
                     "'p' load a play queue\n"
                     "'?' search using query\n"
                     "'s' get status\n"
                     "'z' previous\n"
                     "'x' play\n"
                     "'c' pause\n"
                     "'v' next\n"
                     "'e' toggle shuffle\n" << std::endl;

        getline( std::cin, cmd );
        std::istringstream iss(cmd);

        while( iss >> arg )
        {
            argv.push_back( arg );
        }

        int argc = argv.size();
        if ( argc == 0)
            continue;

        if ( argv[0] == "addAudio")
        {
            std::string id = "";
            if ( argc > 1 )
                id = argv[1];
            m_.addAudioEndpoint(id, this, NULL);
            continue;
        }
        if ( argv[0] == "remAudio")
        {
            std::string id = "";
            if ( argc > 1 )
                id = argv[1];
            m_.removeAudioEndpoint(id, this, NULL);
            continue;
        }

        //handle the old commands the old way for now..
        c = argv[0][0];
        switch(c)
        {
        case 'i':
        {
            std::string uri;
            std::cout << "Enter Album URI" << std::endl;
            std::cin >> uri;

            m_.getImage( uri, this, NULL );
            break;
        }

        case 'z':
        {
            m_.previous();
            break;
        }
        case 'v':
        {
            m_.next();
            break;
        }
        case 'x':
        {
            m_.resume();
            break;
        }
        case 'c':
        {
            m_.pause();
            break;
        }
        case 'e':
        {
            isShuffle = !isShuffle;
            m_.setShuffle(isShuffle);
            break;
        }
        case 'r':
        {
            isRepeat = !isRepeat;
            m_.setRepeat(isRepeat);
            break;
        }

        case 's':
        {
            m_.getStatus( this, NULL );
            break;
        }
        case 'g':
        {
            m_.getPlaylists( this, NULL );
            break;
        }
        case 't':
        {
            std::string uri;
            std::cout << "Enter Spotify URI" << std::endl;
            std::cin >> uri;

            m_.getTracks( uri, this, NULL );
            break;
        }
        case 'p':
        {
            std::string uri;
            std::cout << "Enter Spotify URI ('w' for wonderwall)" << std::endl;
            std::cin >> uri;

            if (uri == "w") uri = "spotify:track:2CT3r93YuSHtm57mjxvjhH";
            m_.play( uri, this, NULL );
            break;
        }
        case 'a':
        {
            std::string uri;
            std::cout << "Enter Spotify URI ('w' for wonderwall)" << std::endl;
            std::cin >> uri;

            if (uri == "w") uri = "spotify:album:1f4I0SpE0O8yg4Eg2ywwv1";
            m_.getAlbum( uri, this, NULL );
            break;
        }

        case '?':
        {
            std::string query;
            std::cout << "Write your search query, end with enter:" << std::endl;
            std::cin >> query;

            m_.search( query, this, NULL );
            break;
        }

        case 'q':
            cancelThread();
            continue;

        default:
            continue;
        }
    }

    log(LOG_NOTICE) << "Exiting UI";
}


void UIConsole::rootFolderUpdatedInd()
{}
void UIConsole::connectionState( bool up )
{}
void UIConsole::getPlaylistsResponse( const Folder& rootfolder, void* userData )
{
    printFolder( rootfolder, 2 );

    for( LibSpotify::FolderContainer::const_iterator it = rootfolder.getFolders().begin(); it != rootfolder.getFolders().end() ; it++)
        playlists.insert( playlists.end(), (*it).getPlaylists().begin(), (*it).getPlaylists().end());

    playlists.insert( playlists.end(), rootfolder.getPlaylists().begin(), rootfolder.getPlaylists().end());

    itPlaylists_ = playlists.begin();
}
void UIConsole::getTracksResponse( const std::deque<Track>& tracks, void* userData )
{
    printTracks( tracks );
}
void UIConsole::getImageResponse( const void* data, size_t dataSize, void* userData )
{
    std::cout << "Got " << dataSize << " bytes image " << std::endl;
}
void UIConsole::getAlbumResponse( const Album& album, void* userData )
{
    std::cout << "  " << album.getName() << " - " << album.getLink() << std::endl;
    std::cout << "  By " << album.getArtist().getName() << " - " << album.getArtist().getLink() << std::endl;
    printTracks( album.getTracks() );
}
void UIConsole::genericSearchCallback( const std::deque<Track>& listOfTracks, const std::string& didYouMean, void* userData )
{
    printTracks( listOfTracks );
}

void UIConsole::statusUpdateInd( PlaybackState_t state, bool repeatStatus, bool shuffleStatus, const Track& currentTrack, unsigned int progress )
{
    statusUpdateInd( state, repeatStatus, shuffleStatus );

    std::cout << "  Current track: " << currentTrack.getName() << "  -  "  << currentTrack.getLink() << std::endl;
    std::cout << "    " << currentTrack.getAlbum() << "  -  "  << currentTrack.getAlbumLink() << std::endl;
    for ( std::vector<Artist>::const_iterator it = currentTrack.getArtists().begin();
            it != currentTrack.getArtists().end(); it++ )
    {
        std::cout << "    " << (*it).getName() << "  -  "  << (*it).getLink() << std::endl;
    }
    std::cout << "  Duration " << currentTrack.getDurationMillisecs() << std::endl;
    std::cout << "  Progress " << progress << std::endl << std::endl;
}
void UIConsole::statusUpdateInd( PlaybackState_t state, bool repeatStatus, bool shuffleStatus )
{
    switch( state )
    {
        case PLAYBACK_IDLE:    std::cout << "  Playback stopped "; break;
        case PLAYBACK_PLAYING: std::cout << "  Playback playing "; break;
        case PLAYBACK_PAUSED:  std::cout << "  Playback paused  "; break;
    }
    std::cout << " - Repeat " << (repeatStatus ? "on" : "off") << ", Shuffle " << (shuffleStatus ? "on" : "off") << std::endl << std::endl;
}
void UIConsole::getStatusResponse( PlaybackState_t state, bool repeatStatus, bool shuffleStatus, const Track& currentTrack, unsigned int progress, void* userData )
{
    statusUpdateInd( state, repeatStatus, shuffleStatus, currentTrack, progress );
}
void UIConsole::getStatusResponse( PlaybackState_t state, bool repeatStatus, bool shuffleStatus, void* userData )
{
    statusUpdateInd( state, repeatStatus, shuffleStatus );
}



void printFolder( const Folder& f, int indent )
{
    std::cout << std::string( indent, ' ') << f.getName() << std::endl;

    for( LibSpotify::FolderContainer::const_iterator it = f.getFolders().begin(); it != f.getFolders().end() ; it++)
        printFolder( (*it), indent+2 );

    for( LibSpotify::PlaylistContainer::const_iterator it = f.getPlaylists().begin(); it != f.getPlaylists().end() ; it++)
        std::cout << std::string( indent+2, ' ') << (*it).getName() << "  -  " << (*it).getLink() << std::endl;
}

void printTracks( const std::deque<Track>& tracks )
{
    for( std::deque<Track>::const_iterator it = tracks.begin(); it != tracks.end() ; it++)
    {
        if ( (*it).getIndex() == -1 )
            std::cout << "  " << (*it).getName() << "  -  "  << (*it).getLink() << std::endl;
        else
            std::cout << "  " << (*it).getIndex() << ". " <<(*it).getName() << "  -  "  << (*it).getLink() << std::endl;
    }
}
