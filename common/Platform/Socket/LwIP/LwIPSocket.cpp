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

#include "../Socket.h"
#include "LwipSocketPimpl.h"

#include "lwip/sockets.h"

#include "applog.h"

#include <string.h>
#include <stdlib.h>     /* atoi */

Socket::Socket(SocketHandle_t* socket) : socket_(socket)
{
    /* Set socket non-blocking */
    int opt = 1;
    lwip_ioctl(socket_->fd, FIONBIO, &opt);

}

Socket::Socket( SockType_t type )
{
    int on = 1;

    socket_ = new SocketHandle_t;

    switch(type)
    {
        case SOCKTYPE_STREAM:
            socket_->fd = lwip_socket(PF_INET, SOCK_STREAM, 0);
            break;
        case SOCKTYPE_DATAGRAM:
            socket_->fd = lwip_socket(PF_INET, SOCK_DGRAM, 0);
            break;
        default:
            break;
    }

    lwip_setsockopt(socket_->fd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));

    /* Set socket non-blocking */
    lwip_ioctl(socket_->fd, FIONBIO, &on);

}

int Socket::BindToAddr(const std::string& addr, const std::string& port)
{
    struct sockaddr_in my_addr;

    memset(&my_addr, 0, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(atoi(port.c_str()));

    if (addr != "ANY" && inet_addr(addr.c_str()) != INADDR_NONE)
    {
        my_addr.sin_addr.s_addr = inet_addr(addr.c_str());
    }

    log(LOG_DEBUG) << "binding to " << addr << " -> " << inet_ntoa(my_addr.sin_addr) << " port " << port;

    if (lwip_bind(socket_->fd, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0)
    {
        log(LOG_EMERG) << "Error on bind!";
        return -1;
    }

    return 0;
}

int Socket::BindToDevice(const std::string& device, const std::string& port)
{
    return -1;
}

int Socket::Connect(const std::string& addr, const std::string& port)
{
    int rc;
    struct sockaddr_in serv_addr;
    memset( &serv_addr, 0, sizeof(struct sockaddr_in) );
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr( addr.c_str() );
    serv_addr.sin_port = htons(atoi(port.c_str()));
    rc = lwip_connect( socket_->fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );

    if (rc < 0 && errno == EINPROGRESS)
        rc = 0;

    return WaitForConnect();
}


int Socket::WaitForConnect()
{
    int error;
    socklen_t len = sizeof(error);
    std::set<Socket*> s;
    s.insert(this);
    //log(LOG_WARN) << "waiting";

    if ( select(NULL, &s, NULL, 5000) <= 0)
    {
        return -1;
    }

    if ( lwip_getsockopt(socket_->fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 ||
         error > 0 )
    {
        return -1;
    }

    return 0;
}

int Socket::Listen()
{
    return lwip_listen(socket_->fd, 5); /*todo 5?*/
}

Socket::~Socket()
{
    Close();
    delete socket_;
}

Socket* Socket::Accept()
{
    struct sockaddr_in sockaddr;
    socklen_t len = sizeof(struct sockaddr_in);
    int newSocket = lwip_accept(socket_->fd, (struct sockaddr*)&sockaddr, &len);

    if(newSocket >= 0)
    {
        log(LOG_DEBUG) << "accept! " << inet_ntoa(sockaddr.sin_addr) << " fd " << newSocket;
        SocketHandle_t* handle = new SocketHandle_t;
        handle->fd = newSocket;
        return new Socket(handle);
    }
    else
    {
        log(LOG_WARN) << "accept failed: " << strerror(errno);
        return NULL;
    }
}

int Socket::Send(const void* msg, int msgLen)
{
    return lwip_send(socket_->fd, msg, msgLen, 0);
}

int Socket::SendTo(const void* msg, int msgLen, const std::string& addr, const std::string& port)
{
    struct sockaddr_in toAddr;
    memset( &toAddr, 0, sizeof(struct sockaddr_in) );
    toAddr.sin_family = AF_INET;
    toAddr.sin_addr.s_addr = inet_addr( addr.c_str() );
    toAddr.sin_port = htons(atoi(port.c_str()));
    int rc = -1;

    rc = lwip_sendto(socket_->fd, msg, msgLen, 0, (struct sockaddr*) &toAddr, sizeof(toAddr) );

    return rc;
}

int Socket::Receive(void* buf, int bufLen)
{
    int n = lwip_recv(socket_->fd, buf, bufLen, 0);
    if (n < 0 && errno == EWOULDBLOCK)
        return 0;
    else if (n == 0)
        return -1;
    return n;
}

int Socket::ReceiveFrom(void* buf, int bufLen, std::string& addr, std::string& port)
{
    struct sockaddr_in sockaddr;
    socklen_t len = sizeof(struct sockaddr_in);
    char str[20];
    int n;

    n = lwip_recvfrom( socket_->fd, buf, bufLen, 0, (struct sockaddr*)&sockaddr, &len );
    if (n < 0 && errno == EWOULDBLOCK)
        return 0;
    else if (n == 0)
        return -1;

    inet_ntoa_r( sockaddr.sin_addr, str, sizeof(str));
    addr = str;

    snprintf(str, sizeof(str), "%u", ntohs(sockaddr.sin_port) );
    port = str;

    return n;
}

int Socket::EnableBroadcast()
{
    return 0;
}

void Socket::Close()
{
    lwip_close(socket_->fd);
}

void Socket::Shutdown()
{
    lwip_shutdown(socket_->fd, SHUT_RD);
}


uint32_t Ntohl(uint32_t x)
{
    return ntohl(x);
}

uint16_t Ntohs(uint16_t x)
{
    return ntohs(x);
}

uint32_t Htonl(uint32_t x)
{
    return htonl(x);
}

uint16_t Htons(uint16_t x)
{
    return htons(x);
}

