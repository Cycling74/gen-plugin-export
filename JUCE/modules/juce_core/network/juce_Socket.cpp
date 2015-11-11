/*
  ==============================================================================

   This file is part of the juce_core module of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission to use, copy, modify, and/or distribute this software for any purpose with
   or without fee is hereby granted, provided that the above copyright notice and this
   permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
   NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
   IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ------------------------------------------------------------------------------

   NOTE! This permissive ISC license applies ONLY to files within the juce_core module!
   All other JUCE modules are covered by a dual GPL/commercial license, so if you are
   using any other modules, be sure to check that you also comply with their license.

   For more details, visit www.juce.com

  ==============================================================================
*/

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable : 4127 4389 4018)
#endif

#ifndef AI_NUMERICSERV  // (missing in older Mac SDKs)
 #define AI_NUMERICSERV 0x1000
#endif

#if JUCE_WINDOWS
 typedef int       juce_socklen_t;
 typedef SOCKET    SocketHandle;
#else
 typedef socklen_t juce_socklen_t;
 typedef int       SocketHandle;
#endif

//==============================================================================
namespace SocketHelpers
{
    static void initSockets()
    {
       #if JUCE_WINDOWS
        static bool socketsStarted = false;

        if (! socketsStarted)
        {
            socketsStarted = true;

            WSADATA wsaData;
            const WORD wVersionRequested = MAKEWORD (1, 1);
            WSAStartup (wVersionRequested, &wsaData);
        }
       #endif
    }

    static bool resetSocketOptions (const SocketHandle handle, const bool isDatagram, const bool allowBroadcast) noexcept
    {
        const int sndBufSize = 65536;
        const int rcvBufSize = 65536;
        const int one = 1;

        return handle > 0
                && setsockopt (handle, SOL_SOCKET, SO_RCVBUF, (const char*) &rcvBufSize, sizeof (rcvBufSize)) == 0
                && setsockopt (handle, SOL_SOCKET, SO_SNDBUF, (const char*) &sndBufSize, sizeof (sndBufSize)) == 0
                && (isDatagram ? ((! allowBroadcast) || setsockopt (handle, SOL_SOCKET, SO_BROADCAST, (const char*) &one, sizeof (one)) == 0)
                               : (setsockopt (handle, IPPROTO_TCP, TCP_NODELAY, (const char*) &one, sizeof (one)) == 0));
    }

    static void closeSocket (volatile int& handle, CriticalSection& readLock,
                             const bool isListener, int portNumber, bool& connected) noexcept
    {
        const SocketHandle h = handle;
        handle = -1;

       #if JUCE_WINDOWS
        ignoreUnused (portNumber, isListener, readLock);

        if (h != SOCKET_ERROR || connected)
            closesocket (h);

        // make sure any read process finishes before we delete the socket
        CriticalSection::ScopedLockType lock(readLock);
        connected = false;
       #else
        if (connected)
        {
            connected = false;

            if (isListener)
            {
                // need to do this to interrupt the accept() function..
                StreamingSocket temp;
                temp.connect (IPAddress::local().toString(), portNumber, 1000);
            }
        }

        if (h != -1)
        {
            // unblock any pending read requests
            ::shutdown (h, SHUT_RDWR);
            {
                // see man-page of recv on linux about a race condition where the
                // shutdown command is lost if the receiving thread does not have
                // a chance to process before close is called. On Mac OS X shutdown
                // does not unblock a select call, so using a lock here will dead-lock
                // both threads.
               #if JUCE_LINUX
                CriticalSection::ScopedLockType lock (readLock);
                ::close (h);
               #else
                ::close (h);
                CriticalSection::ScopedLockType lock (readLock);
              #endif
            }
        }
       #endif
    }

    static bool bindSocket (const SocketHandle handle, const int port, const String& address) noexcept
    {
        if (handle <= 0 || port < 0)
            return false;

        struct sockaddr_in servTmpAddr;
        zerostruct (servTmpAddr); // (can't use "= { 0 }" on this object because it's typedef'ed as a C struct)
        servTmpAddr.sin_family = PF_INET;
        servTmpAddr.sin_addr.s_addr = htonl (INADDR_ANY);
        servTmpAddr.sin_port = htons ((uint16) port);

        if (address.isNotEmpty())
            servTmpAddr.sin_addr.s_addr = ::inet_addr (address.toUTF8());

        return bind (handle, (struct sockaddr*) &servTmpAddr, sizeof (struct sockaddr_in)) >= 0;
    }

    static int getBoundPort (const SocketHandle handle) noexcept
    {
        if (handle <= 0)
            return -1;

        struct sockaddr_in sin_addr;
        socklen_t len = sizeof (sin_addr);

        if (getsockname (handle, (struct sockaddr*) &sin_addr, &len) == 0)
            return ntohs (sin_addr.sin_port);

        return -1;
    }

    static int readSocket (const SocketHandle handle,
                           void* const destBuffer, const int maxBytesToRead,
                           bool volatile& connected,
                           const bool blockUntilSpecifiedAmountHasArrived,
                           CriticalSection& readLock,
                           String* senderIP = nullptr,
                           int* senderPort = nullptr) noexcept
    {
        int bytesRead = 0;

        while (bytesRead < maxBytesToRead)
        {
            long bytesThisTime = -1;
            char* const buffer = static_cast<char*> (destBuffer) + bytesRead;
            const juce_socklen_t numToRead = (juce_socklen_t) (maxBytesToRead - bytesRead);

            {
                // avoid race-condition
                CriticalSection::ScopedTryLockType lock (readLock);

                if (lock.isLocked())
                {
                    if (senderIP == nullptr || senderPort == nullptr)
                    {
                        bytesThisTime = ::recv (handle, buffer, numToRead, 0);
                    }
                    else
                    {
                        sockaddr_in client;
                        socklen_t clientLen = sizeof (sockaddr);

                        bytesThisTime = ::recvfrom (handle, buffer, numToRead, 0, (sockaddr*) &client, &clientLen);

                        *senderIP = String::fromUTF8 (inet_ntoa (client.sin_addr), 16);
                        *senderPort = ntohs (client.sin_port);
                    }
                }
            }

            if (bytesThisTime <= 0 || ! connected)
            {
                if (bytesRead == 0)
                    bytesRead = -1;

                break;
            }

            bytesRead += bytesThisTime;

            if (! blockUntilSpecifiedAmountHasArrived)
                break;
        }

        return (int) bytesRead;
    }

    static int waitForReadiness (const volatile int& handle, CriticalSection& readLock,
                                 const bool forReading, const int timeoutMsecs) noexcept
    {
        // avoid race-condition
        CriticalSection::ScopedTryLockType lock (readLock);

        if (! lock.isLocked())
            return -1;

        int h = handle;

        struct timeval timeout;
        struct timeval* timeoutp;

        if (timeoutMsecs >= 0)
        {
            timeout.tv_sec = timeoutMsecs / 1000;
            timeout.tv_usec = (timeoutMsecs % 1000) * 1000;
            timeoutp = &timeout;
        }
        else
        {
            timeoutp = 0;
        }

        fd_set rset, wset;
        FD_ZERO (&rset);
        FD_SET (h, &rset);
        FD_ZERO (&wset);
        FD_SET (h, &wset);

        fd_set* const prset = forReading ? &rset : nullptr;
        fd_set* const pwset = forReading ? nullptr : &wset;

       #if JUCE_WINDOWS
        if (select ((int) h + 1, prset, pwset, 0, timeoutp) < 0)
            return -1;
       #else
        {
            int result;
            while ((result = select (h + 1, prset, pwset, 0, timeoutp)) < 0
                    && errno == EINTR)
            {
            }

            if (result < 0)
                return -1;
        }
       #endif

        // we are closing
        if (handle < 0)
            return -1;

        {
            int opt;
            juce_socklen_t len = sizeof (opt);

            if (getsockopt (h, SOL_SOCKET, SO_ERROR, (char*) &opt, &len) < 0
                 || opt != 0)
                return -1;
        }

        return FD_ISSET (h, forReading ? &rset : &wset) ? 1 : 0;
    }

    static bool setSocketBlockingState (const SocketHandle handle, const bool shouldBlock) noexcept
    {
       #if JUCE_WINDOWS
        u_long nonBlocking = shouldBlock ? 0 : (u_long) 1;
        return ioctlsocket (handle, FIONBIO, &nonBlocking) == 0;
       #else
        int socketFlags = fcntl (handle, F_GETFL, 0);

        if (socketFlags == -1)
            return false;

        if (shouldBlock)
            socketFlags &= ~O_NONBLOCK;
        else
            socketFlags |= O_NONBLOCK;

        return fcntl (handle, F_SETFL, socketFlags) == 0;
       #endif
    }

    static addrinfo* getAddressInfo (const bool isDatagram, const String& hostName, int portNumber)
    {
        struct addrinfo hints;
        zerostruct (hints);

        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = isDatagram ? SOCK_DGRAM : SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV;

        struct addrinfo* info = nullptr;
        if (getaddrinfo (hostName.toUTF8(), String (portNumber).toUTF8(), &hints, &info) == 0
             && info != nullptr)
            return info;

        return nullptr;
    }

    static bool connectSocket (int volatile& handle,
                               CriticalSection& readLock,
                               const String& hostName,
                               const int portNumber,
                               const int timeOutMillisecs) noexcept
    {
        struct addrinfo* info = getAddressInfo (false, hostName, portNumber);

        if (info == nullptr)
            return false;

        if (handle < 0)
            handle = (int) socket (info->ai_family, info->ai_socktype, 0);

        if (handle < 0)
        {
            freeaddrinfo (info);
            return false;
        }

        setSocketBlockingState (handle, false);
        const int result = ::connect (handle, info->ai_addr, (socklen_t) info->ai_addrlen);
        freeaddrinfo (info);

        bool retval = (result >= 0);

        if (result < 0)
        {
           #if JUCE_WINDOWS
            if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
           #else
            if (errno == EINPROGRESS)
           #endif
            {
                if (waitForReadiness (handle, readLock, false, timeOutMillisecs) == 1)
                    retval = true;
            }
        }

        setSocketBlockingState (handle, true);

        if (retval)
            resetSocketOptions (handle, false, false);

        return retval;
    }

    static void makeReusable (int handle) noexcept
    {
        const int reuse = 1;
        setsockopt (handle, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuse, sizeof (reuse));
    }

    static bool multicast (int handle, const String& multicastIPAddress,
                           const String& interfaceIPAddress, bool join) noexcept
    {
        struct ip_mreq mreq;

        zerostruct (mreq);
        mreq.imr_multiaddr.s_addr = inet_addr (multicastIPAddress.toUTF8());
        mreq.imr_interface.s_addr = INADDR_ANY;

        if (interfaceIPAddress.isNotEmpty())
            mreq.imr_interface.s_addr = inet_addr (interfaceIPAddress.toUTF8());

        int joinCmd = join ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;

        return setsockopt (handle, IPPROTO_IP, joinCmd, (const char*) &mreq, sizeof (mreq)) == 0;
    }
}

//==============================================================================
StreamingSocket::StreamingSocket()
    : portNumber (0),
      handle (-1),
      connected (false),
      isListener (false)
{
    SocketHelpers::initSockets();
}

StreamingSocket::StreamingSocket (const String& host, int portNum, int h)
    : hostName (host),
      portNumber (portNum),
      handle (h),
      connected (true),
      isListener (false)
{
    SocketHelpers::initSockets();
    SocketHelpers::resetSocketOptions (h, false, false);
}

StreamingSocket::~StreamingSocket()
{
    close();
}

//==============================================================================
int StreamingSocket::read (void* destBuffer, const int maxBytesToRead, bool shouldBlock)
{
    return (connected && ! isListener) ? SocketHelpers::readSocket (handle, destBuffer, maxBytesToRead,
                                                                    connected, shouldBlock, readLock)
                                       : -1;
}

int StreamingSocket::write (const void* sourceBuffer, const int numBytesToWrite)
{
    if (isListener || ! connected)
        return -1;

    return (int) ::send (handle, (const char*) sourceBuffer, (juce_socklen_t) numBytesToWrite, 0);
}

//==============================================================================
int StreamingSocket::waitUntilReady (const bool readyForReading,
                                     const int timeoutMsecs) const
{
    return connected ? SocketHelpers::waitForReadiness (handle, readLock, readyForReading, timeoutMsecs)
                     : -1;
}

//==============================================================================
bool StreamingSocket::bindToPort (const int port)
{
    return bindToPort (port, String());
}

bool StreamingSocket::bindToPort (const int port, const String& addr)
{
    return SocketHelpers::bindSocket (handle, port, addr);
}

int StreamingSocket::getBoundPort() const noexcept
{
    return SocketHelpers::getBoundPort (handle);
}

bool StreamingSocket::connect (const String& remoteHostName,
                               const int remotePortNumber,
                               const int timeOutMillisecs)
{
    if (isListener)
    {
        jassertfalse;    // a listener socket can't connect to another one!
        return false;
    }

    if (connected)
        close();

    hostName = remoteHostName;
    portNumber = remotePortNumber;
    isListener = false;

    connected = SocketHelpers::connectSocket (handle, readLock, remoteHostName,
                                              remotePortNumber, timeOutMillisecs);

    if (! (connected && SocketHelpers::resetSocketOptions (handle, false, false)))
    {
        close();
        return false;
    }

    return true;
}

void StreamingSocket::close()
{
    SocketHelpers::closeSocket (handle, readLock, isListener, portNumber, connected);

    hostName.clear();
    portNumber = 0;
    handle = -1;
    isListener = false;
}

//==============================================================================
bool StreamingSocket::createListener (const int newPortNumber, const String& localHostName)
{
    if (connected)
        close();

    hostName = "listener";
    portNumber = newPortNumber;
    isListener = true;

    struct sockaddr_in servTmpAddr;
    zerostruct (servTmpAddr);

    servTmpAddr.sin_family = PF_INET;
    servTmpAddr.sin_addr.s_addr = htonl (INADDR_ANY);

    if (localHostName.isNotEmpty())
        servTmpAddr.sin_addr.s_addr = ::inet_addr (localHostName.toUTF8());

    servTmpAddr.sin_port = htons ((uint16) portNumber);

    handle = (int) socket (AF_INET, SOCK_STREAM, 0);

    if (handle < 0)
        return false;

   #if ! JUCE_WINDOWS // on windows, adding this option produces behaviour different to posix
    SocketHelpers::makeReusable (handle);
   #endif

    if (bind (handle, (struct sockaddr*) &servTmpAddr, sizeof (struct sockaddr_in)) < 0
         || listen (handle, SOMAXCONN) < 0)
    {
        close();
        return false;
    }

    connected = true;
    return true;
}

StreamingSocket* StreamingSocket::waitForNextConnection() const
{
    // To call this method, you first have to use createListener() to
    // prepare this socket as a listener.
    jassert (isListener || ! connected);

    if (connected && isListener)
    {
        struct sockaddr_storage address;
        juce_socklen_t len = sizeof (address);
        const int newSocket = (int) accept (handle, (struct sockaddr*) &address, &len);

        if (newSocket >= 0 && connected)
            return new StreamingSocket (inet_ntoa (((struct sockaddr_in*) &address)->sin_addr),
                                        portNumber, newSocket);
    }

    return nullptr;
}

bool StreamingSocket::isLocal() const noexcept
{
    return hostName == "127.0.0.1";
}


//==============================================================================
//==============================================================================
DatagramSocket::DatagramSocket (const bool canBroadcast)
    : handle (-1),
      isBound (false),
      lastServerPort (-1),
      lastServerAddress (nullptr)
{
    SocketHelpers::initSockets();

    handle = (int) socket (AF_INET, SOCK_DGRAM, 0);
    if (handle >= 0)
    {
        SocketHelpers::resetSocketOptions (handle, true, canBroadcast);
        SocketHelpers::makeReusable (handle);
    }
}

DatagramSocket::~DatagramSocket()
{
    if (lastServerAddress != nullptr)
        freeaddrinfo (static_cast <struct addrinfo*> (lastServerAddress));

    shutdown();
}

void DatagramSocket::shutdown()
{
    if (handle < 0)
        return;

    int copyOfHandle = handle;
    handle = -1;
    bool connected = false;
    SocketHelpers::closeSocket (copyOfHandle, readLock, false, 0, connected);
}

bool DatagramSocket::bindToPort (const int port)
{
    return bindToPort (port, String());
}

bool DatagramSocket::bindToPort (const int port, const String& addr)
{
    if (handle < 0)
        return false;

    if (SocketHelpers::bindSocket (handle, port, addr))
    {
        isBound = true;
        lastBindAddress = addr;

        return true;
    }

    return false;
}

int DatagramSocket::getBoundPort() const noexcept
{
    if (handle < 0)
        return -1;

    return isBound ? SocketHelpers::getBoundPort (handle) : -1;
}

//==============================================================================
int DatagramSocket::waitUntilReady (const bool readyForReading,
                                    const int timeoutMsecs) const
{
    if (handle < 0)
        return -1;

    return SocketHelpers::waitForReadiness (handle, readLock, readyForReading, timeoutMsecs);
}

int DatagramSocket::read (void* destBuffer, int maxBytesToRead, bool shouldBlock)
{
    if (handle < 0)
        return -1;

    bool connected = true;
    return isBound ? SocketHelpers::readSocket (handle, destBuffer, maxBytesToRead,
                                                connected, shouldBlock, readLock) : -1;
}

int DatagramSocket::read (void* destBuffer, int maxBytesToRead, bool shouldBlock, String& senderIPAddress, int& senderPort)
{
    if (handle < 0)
        return -1;

    bool connected = true;
    return isBound ? SocketHelpers::readSocket (handle, destBuffer, maxBytesToRead, connected,
                                                shouldBlock, readLock, &senderIPAddress, &senderPort) : -1;
}

int DatagramSocket::write (const String& remoteHostname, int remotePortNumber,
                           const void* sourceBuffer, int numBytesToWrite)
{
    if (handle < 0)
        return -1;

    struct addrinfo*& info = reinterpret_cast <struct addrinfo*&> (lastServerAddress);

    // getaddrinfo can be quite slow so cache the result of the address lookup
    if (info == nullptr || remoteHostname != lastServerHost || remotePortNumber != lastServerPort)
    {
        if (info != nullptr)
            freeaddrinfo (info);

        if ((info = SocketHelpers::getAddressInfo (true, remoteHostname, remotePortNumber)) == nullptr)
            return -1;

        lastServerHost = remoteHostname;
        lastServerPort = remotePortNumber;
    }

    return (int) ::sendto (handle, (const char*) sourceBuffer,
                           (juce_socklen_t) numBytesToWrite, 0,
                           info->ai_addr, (socklen_t) info->ai_addrlen);
}

bool DatagramSocket::joinMulticast (const String& multicastIPAddress)
{
    if (! isBound || handle < 0)
        return false;

    return SocketHelpers::multicast (handle, multicastIPAddress, lastBindAddress, true);
}

bool DatagramSocket::leaveMulticast (const String& multicastIPAddress)
{
    if (! isBound || handle < 0)
        return false;

    return SocketHelpers::multicast (handle, multicastIPAddress, lastBindAddress, false);
}

bool DatagramSocket::setEnablePortReuse (bool enabled)
{
   #if JUCE_ANDROID
    return false;
   #else
    const int reuse = enabled ? 1 : 0;

   #if JUCE_WINDOWS
    // port re-use is implied by addr re-use on windows
    const int optname = SO_REUSEADDR;
   #else
    const int optname = SO_REUSEPORT;
   #endif


    if (handle >= 0)
        return (setsockopt (handle, SOL_SOCKET, optname,
                            (const char*) &reuse, sizeof (reuse)) == 0);

    return false;
   #endif
}

#if JUCE_MSVC
 #pragma warning (pop)
#endif
