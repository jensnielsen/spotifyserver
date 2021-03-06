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
import pickle
from threading import Lock

class ConfigHandler(object):
    def __init__(self):
        self.__ipaddr = "127.0.0.1"
        self.__port = 7788
        
        self.__configSubscriberLock = Lock()
        self.__configSubscribers = []
    
    def __getstate__(self):
        result = self.__dict__.copy()
        # Not very pretty, is it possible to append class name automatically?
        del result['_ConfigHandler__configSubscriberLock']
        del result['_ConfigHandler__configSubscribers']
        return result
    
    def __setstate__(self,state):
        self.__dict__.update(state)
        
    def setIpAddr(self, ipaddr):
        self.__ipaddr = ipaddr
        
    def getIpAddr(self):
        return self.__ipaddr
    
    def setPort(self, port):
        self.__port = port
        
    def getPort(self):
        return self.__port
    
    def configUpdateDoneInd(self):
        self.__configSubscriberLock.acquire()
        for subscriber in self.__configSubscribers:
            subscriber.configUpdateIndCb()
        self.__configSubscriberLock.release()
    
    def registerForConfigUpdates(self, registrator):
        self.__configSubscriberLock.acquire()
        self.__configSubscribers.append(registrator)
        self.__configSubscriberLock.release()
        
    def writeToFile(self):
        outputFile = open("config.dat", "wb")
        pickle.dump(self,outputFile)
        outputFile.close()
        print "wrote to file"
            
    def readFromFile(self):
        inputFile = open("config.dat", "rb")
        # Not very pretty, if I pickle directly to self, things wont get updated properly,
        # is there another way to do this?
        tmp = pickle.load(inputFile)
        self.__ipaddr = tmp.__ipaddr
        self.__port = tmp.__port
        inputFile.close()