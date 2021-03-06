'''
Copyright (c) 2012, Jesper Derehag
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the <organization> nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL JESPER DEREHAG BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
'''
from Tkinter import *
import MultiListbox
import SpotifyClientConfigGui
from SpotifyClientIf import SpotifyClient, ConfigHandler
from threading import Thread, Event


class SpotifyClientMainGui(Tk):
    def __init__(self):
        Tk.__init__(self)
        
        self.title("SpotiServClient")
        self.protocol("WM_DELETE_WINDOW", self.__del__)
        self.geometry("1000x600")
        
        # Menubar
        self.menubar = Menu(self)
        menu = Menu(self.menubar, tearoff=0)
        self.menubar.add_cascade(label="File", menu=menu)
        menu.add_command(label="Preferences", command=self.openConfigGui)
        menu.add_separator()
        menu.add_command(label="Quit", command=self.__del__)

        try:
            self.config(menu=self.menubar)
        except AttributeError:
            self.tk.call(self, "config", "-menu", self.menubar)
        
        self.searchFrame = Frame(self, height=32)
        self.searchFrame.pack_propagate(0)
        self.searchFrame.pack(side=TOP, fill=X)
        searchLabel = Label(self.searchFrame, justify=LEFT, text="Search: ")
        searchLabel.pack(side=LEFT, anchor=W)
        self.searchEntry = Entry(self.searchFrame)
        self.searchEntry.pack(side=LEFT, anchor=W)
        self.searchEntry.bind("<Return>", self.executeSearchReq)
        
        self.playbackBar = PlaybackBar(self, height=32)
        self.playbackBar.pack_propagate(0)
        self.playbackBar.pack(side=BOTTOM, fill=X)
        
        self.leftFrame = LeftFrame(self, width=200)
        self.leftFrame.pack_propagate(0)
        self.leftFrame.pack(side=LEFT, anchor=W, fill=Y)
        
        self.__tracks = []
        self.multiListboxTracks = MultiListbox.MultiListbox(self, (('Title', 30), ('Artist', 20), ('Album', 10)))
        self.multiListboxTracks.pack(expand=YES,fill=BOTH)  
        self.multiListboxTracks.bind("<Double-Button-1>", self.listboxTracksPlayReq)       
            
        #Spotify client stuff
        self.configHandler = ConfigHandler.ConfigHandler()
        try:
            self.configHandler.readFromFile()
        except:
            print "Could not read settings from file!"
        
        self.configHandler.registerForConfigUpdates(self)
        self.spotify = SpotifyClient.SpotifyClient(self.configHandler.getIpAddr(), 
                                                   self.configHandler.getPort())
        self.leftFrame.setSpotifyClient(self.spotify)
        self.playbackBar.setSpotifyClient(self.spotify)
        self.spotify.registerConnectionObserver(self)
        
    def __del__(self):
        self.spotify.stop()
        self.playbackBar.stop()
        self.destroy()
    
    def configUpdateIndCb(self):
        self.spotify.reconnect(self.configHandler.getIpAddr(), self.configHandler.getPort())
    
    def connectedIndCb(self):
        self.spotify.sendGetStatusReq()
    
    def disconnectedIndCb(self):
        self.__tracks[:] = []
        self.multiListboxTracks.delete(0, END)
        
    def openConfigGui(self):
        SpotifyClientConfigGui.SpotifyClientConfigGui(self.configHandler)
        
    def executeSearchReq(self, *event):
        self.spotify.sendSearchReq(self, self.searchEntry.get())
        # For now, we dont support search playback with index offset
        self.__pendingTrackListParent = None
        
    def listboxTracksPlayReq(self, *event):
        tr = self.__tracks[int(self.multiListboxTracks.curselection()[0])]
        if(self.__pendingTrackListParent != None):
            self.spotify.sendPlayReq(self.__pendingTrackListParent.getUri(), tr.getIndex())
        else:
            self.spotify.sendPlayReq(tr.getUri(), tr.getIndex())
    
    def getTracksRspCb(self, listOfTracks):
        self.__tracks = listOfTracks
        self.updateListboxTracks()
    
    def searchRspCb(self, listOfTracks):
        self.__tracks = listOfTracks
        self.updateListboxTracks()
    
    def updateListboxTracks(self):
        self.multiListboxTracks.delete(0, END)
        for track in self.__tracks:
            artistStr = ""
            for artist in track.getArtistList():
                artistStr += artist.getName() + " "
            self.multiListboxTracks.insert(END, (track.getName(), artistStr, track.getAlbum().getName()))
            
    def setPendingTrackListParent(self, parent):
        self.__pendingTrackListParent = parent
        
    def getPendingTrackListParent(self):
        return self.__pendingTrackListParent


class LeftFrame(Frame):
    def __init__(self, parentFrame,  **kwargs):
        Frame.__init__(self, parentFrame, **kwargs)
        self.parent = parentFrame
        self.spotify = None
        
        self.__playlists = []
        
        # Playlist lisbox
        self.playlistFrame = Frame(self)
        self.playlistFrame.pack(side=TOP, fill=BOTH, expand=1)
        self.listboxPlaylist = Listbox(self.playlistFrame, exportselection=0)
        self.listboxPlaylist.pack(side=LEFT, fill=BOTH, expand=1)
        yscroll = Scrollbar(self.playlistFrame, command=self.listboxPlaylist.yview, orient=VERTICAL)
        yscroll.pack(side=LEFT, fill=Y)
        self.listboxPlaylist.configure(yscrollcommand=yscroll.set)
        self.listboxPlaylist.bind("<<ListboxSelect>>", self.listboxPlaylistSelectionChanged)
        
        self.nowPlayingFrame = Frame(self, height=150)
        self.nowPlayingFrame.pack_propagate(0)
        self.nowPlayingFrame.pack(side=BOTTOM, fill=BOTH)
        self.labelTrack = Label(self.nowPlayingFrame, justify=LEFT)
        self.labelTrack.pack(side=BOTTOM, anchor=W)
        self.labelArtist = Label(self.nowPlayingFrame, justify=LEFT)
        self.labelArtist.pack(side=BOTTOM, anchor=W)
        self.nowPlayingTrack = None
    
    def __del__(self):
        return
             
    def setSpotifyClient(self,spotify):
        self.spotify = spotify
        self.spotify.registerPlaylistUpdatedIndObserver(self)
        self.spotify.registerStatusIndObserver(self)
        self.spotify.registerConnectionObserver(self)
        self.spotify.sendGetPlaylistReq()
        
    def connectedIndCb(self):
        self.spotify.sendGetPlaylistReq()
        
    def disconnectedIndCb(self):
        self.__playlists[:] = []
        self.listboxPlaylist.delete(0, END)
        self.labelTrack.config(text="Track:")
        self.labelArtist.config(text="Artist:")
        
        
    def statusIndCb(self, playStatus, track, progress):
        if(track != None):
            self.labelTrack.config(text="Track: " + track.getName())
            artistString = ""
            for artist in track.getArtistList():
                artistString += artist.getName() + " " 
            self.labelArtist.config(text="Artist: " + artistString)
            ''' 
            # Since spotify image format is JPEG, PIL is required to read it,
            # which in turn needs to be installed on the host, therefore skip image support for now. 
            self.spotify.sendGetImageReq(self, track.getUri())
            '''
        return
    
    def getImageReqCb(self, imageFormat, image):
        '''
        # Since spotify image format is JPEG, PIL is required to read it,
        # which in turn needs to be installed on the host, therefore skip image support for now. 
        if(imageFormat == self.spotify.IMAGE_FORMAT_JPEG):
            im = Image.Image.open(StringIO.StringIO(image))
            im.show()
            tkim = ImageTk.PhotoImage(im) 
        '''
    
    def playlistUpdatedIndCb(self):
        self.updateListboxPlaylist()
        
    def listboxPlaylistSelectionChanged(self, *event):
        pl = self.__playlists[int(self.listboxPlaylist.curselection()[0])]
        self.spotify.sendGetTracksReq(self.parent, pl.getUri())
        self.parent.setPendingTrackListParent(pl)
        
    def updateListboxPlaylist(self):
        self.__playlists = self.spotify.getPlaylists()
        self.listboxPlaylist.delete(0, END)
        for pl in self.spotify.getPlaylists():
            self.listboxPlaylist.insert(END, pl)
                
class PlaybackBar(Frame):
    STATE_STOPPED = 0
    STATE_PLAYING = 1
    STATE_PAUSED = 3
    
    def __init__(self, parentFrame, **kwargs):
        Frame.__init__(self, parentFrame, **kwargs)
        self.spotify = None
        #
        self.__playbackState = self.STATE_STOPPED
        
        self.__buttonPrev = Button(self, text="Prev", command=self.__PlayPrev)
        self.__buttonPrev.pack(side=LEFT,anchor=W)
        
        self.__buttonPlay = Button(self, text="Play", width=5, command=self.__Play)
        self.__buttonPlay.pack(side=LEFT,anchor=W)

        self.__buttonNext = Button(self, text="Next", command=self.__PlayNext)
        self.__buttonNext.pack(side=LEFT,anchor=W)
        
        
        #Trackbar
        self.__progressTimeIntVar = IntVar(value=0)
        self.__progressTimeStrVar = StringVar(value="0:00")
        self.__totalTimeStrVar = StringVar(value="0:00")
        self.__labelElapsedTime = Label(self, justify=LEFT, textvariable=self.__progressTimeStrVar)
        self.__labelElapsedTime.pack(side=LEFT, anchor=W)
        self.__labelTotalTime = Label(self, justify=RIGHT, textvariable=self.__totalTimeStrVar)
        self.__labelTotalTime.pack(side=RIGHT, anchor=E)
        self.__trackbarPlay = Scale(self, from_=0, to=100, orient=HORIZONTAL, showvalue=0,
                                                        variable=self.__progressTimeIntVar)
        self.__trackbarPlay.pack(side=BOTTOM, fill=X)
        self.__trackbarPollTimer = RepeatTimer(1.0, self.updateTrackbarTimerTick)

    def stop(self):
        self.__trackbarPollTimer.stop()
        
    def __PlayPrev(self):
        self.spotify.sendPlayOperation(self.spotify.PLAY_OP_PREV)
    
    def __Play(self):
        if(self.__playbackState == self.STATE_PAUSED):
            self.__playbackState = self.STATE_PLAYING
            self.__buttonPlay.config(text="Pause")
            self.spotify.sendPlayOperation(self.spotify.PLAY_OP_RESUME)
        elif(self.__playbackState == self.STATE_PLAYING):
            self.__playbackState = self.STATE_PAUSED
            self.__buttonPlay.config(text="Play")
            self.spotify.sendPlayOperation(self.spotify.PLAY_OP_PAUSE)
    
    def __PlayNext(self):
        self.spotify.sendPlayOperation(self.spotify.PLAY_OP_NEXT)
    
    def updateTrackbarTimerTick(self):
        self.__progressTimeIntVar.set(self.__progressTimeIntVar.get() + 1)
        self.__progressTimeStrVar.set(self.getTimeAsString(self.__progressTimeIntVar.get()))
    
    def connectedIndCb(self):
        return
    
    def disconnectedIndCb(self):
        self.__progressTimeIntVar.set(0)
        self.__progressTimeStrVar.set("0:00")
        self.__totalTimeStrVar.set("0:00")
        self.__trackbarPollTimer.pause()
        
        
    def statusIndCb(self, playStatus, track, progress):
        if(playStatus == self.spotify.PLAYBACK_IDLE):
            self.__playbackState = self.STATE_STOPPED
            self.__buttonPlay.config(text="Play")
            self.__trackbarPollTimer.pause()
        elif(playStatus == self.spotify.PLAYBACK_PAUSED):
            self.__playbackState = self.STATE_PAUSED
            self.__buttonPlay.config(text="Play")
            self.__trackbarPollTimer.pause()
        elif(playStatus == self.spotify.PLAYBACK_PLAYING):
            self.__playbackState = self.STATE_PLAYING
            self.__buttonPlay.config(text="Pause")
            self.__trackbarPollTimer.play()
        
        if(track != None):
            self.__trackbarPlay.config(to=track.getDurationMillisecs()/1000)
            self.__totalTimeStrVar.set(self.getTimeAsString(track.getDurationMillisecs()/1000))
        
        if(progress != None):
            self.__progressTimeIntVar.set(progress/1000)
            self.__progressTimeStrVar.set(self.getTimeAsString(self.__progressTimeIntVar.get()))
        else:
            self.__progressTimeIntVar.set(0)
            self.__progressTimeStrVar.set("0:00")
            #self.__trackBarPollTimer.run()
    
    def setSpotifyClient(self,spotify):
        self.spotify = spotify
        self.spotify.registerStatusIndObserver(self)
        self.spotify.registerConnectionObserver(self)
        
    def getTimeAsString(self, sec):
        return str(int(sec)/60) + ":" + str(int(sec)%60).rjust(2,'0')


class RepeatTimer(Thread):
    def __init__(self, interval, function, iterations=0, args=[], kwargs={}):
        Thread.__init__(self)
        self.interval = interval
        self.function = function
        self.iterations = iterations
        self.args = args
        self.kwargs = kwargs
        self.paused = Event()
        self.__cancellationPending = Event()
        
        self.paused.set()
        self.daemon = True
        self.start()
    
    def stop(self):
        self.__cancellationPending.set()
        self.join()
        
    def run(self):
        count = 0
        self.__cancellationPending.clear()
        while not self.__cancellationPending.is_set() and (self.iterations <= 0 or count < self.iterations):
            self.__cancellationPending.wait(self.interval)
            if((self.paused.is_set() == False) and (self.__cancellationPending.is_set() == False)):
                self.function(*self.args, **self.kwargs)
                count += 1
 
    def pause(self):
        self.paused.set()
        
    def play(self):
        self.paused.clear()

        

    
        
        
        