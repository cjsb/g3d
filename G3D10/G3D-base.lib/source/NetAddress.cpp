/**
  \file G3D-base.lib/source/NetAddress.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#include "G3D-base/platform.h"
#include "G3D-base/NetAddress.h"
#include "G3D-base/BinaryInput.h"
#include "G3D-base/BinaryOutput.h"
#include "G3D-base/Array.h"
#include "G3D-base/stringutils.h"
#include "G3D-base/System.h"
#include "G3D-base/NetworkDevice.h"
#include "G3D-base/Log.h"

#if defined(G3D_LINUX) || defined(G3D_OSX)
    #include <unistd.h>
    #include <errno.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/tcp.h>
    #define _alloca alloca
    #define SOCKET_ERROR -1

#   ifndef SOCKADDR_IN
#       define SOCKADDR_IN struct sockaddr_in
#   endif
#   ifndef SOCKET
#       define SOCKET int
#   endif

// SOCKADDR_IN is supposed to be defined in NetAddress.h
#ifndef SOCKADDR_IN
#    error Network headers included in wrong order
#endif
#endif


namespace G3D {

    
NetAddress::NetAddress() {
    System::memset(&addr, 0, sizeof(addr));
}

void NetAddress::init(uint32 host, uint16 port) {
    if ((host != 0) || (port != 0)) {
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(port);
        if (host == 0) {
            host = INADDR_ANY;
        }
        addr.sin_addr.s_addr = htonl(host);
    } else {
        System::memset(&addr, 0, sizeof(addr));
    }
}


NetAddress::NetAddress
   (const String&          hostname,
    uint16                      port) {
    init(hostname, port);
}


void NetAddress::init
   (const String&          hostname,
    uint16                      port) {

    uint32 addr;
    
    if (hostname == "") {
        addr = INADDR_NONE;
    } else {
        addr = inet_addr(hostname.c_str());
    }

    // The address wasn't in numeric form, resolve it
    if (addr == INADDR_NONE) {
        // Get the IP address of the server and store it in host
        struct hostent* host = gethostbyname(hostname.c_str());

        if (host == nullptr) {
            init(0, 0);
            return;
        }

        System::memcpy(&addr, host->h_addr_list[0], host->h_length);
    }

    if (addr != INADDR_NONE) {
        addr = ntohl(addr);
    }
    init(addr, port);
}


NetAddress::NetAddress(uint32 hostip, uint16 port) {
    init(hostip, port);
}


NetAddress NetAddress::broadcastAddress(uint16 port) {
    return NetAddress(NetworkDevice::instance()->broadcastAddressArray()[0], port);
}


NetAddress::NetAddress(const String& hostnameAndPort) {

    Array<String> part = stringSplit(hostnameAndPort, ':');

    debugAssert(part.length() == 2);
    init(part[0], atoi(part[1].c_str()));
}


NetAddress::NetAddress(const SOCKADDR_IN& a) {
    addr = a;
}


NetAddress::NetAddress(const struct in_addr& addr, uint16 port) {
    #ifdef G3D_WINDOWS
        init(ntohl(addr.S_un.S_addr), port);
    #else
        init(htonl(addr.s_addr), port);
    #endif
}

void NetAddress::localHostAddresses(Array<NetAddress>& array) {
    array.resize(0);

    char ac[256];

    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
        Log::common()->printf("Error while getting local host name\n");
        return;
    }

    struct hostent* phe = gethostbyname(ac);
    if (phe == 0) {
        Log::common()->printf("Error while getting local host address\n");
        return;
    }

    for (int i = 0; (phe->h_addr_list[i] != 0); ++i) {
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
        array.append(NetAddress(addr));
    }    
}

void NetAddress::serialize(class BinaryOutput& b) const {
    b.writeUInt32(ip());
    b.writeUInt16(port());
}


void NetAddress::deserialize(class BinaryInput& b) {
    uint32 i;
    uint16 p;

    i = b.readUInt32();
    p = b.readUInt16();

    init(i, p);
}


bool NetAddress::ok() const {
    return addr.sin_family != 0;
}


String NetAddress::ipString() const {
    return format("%s", inet_ntoa(*(in_addr*)&(addr.sin_addr)));
}


String NetAddress::toString() const {
    return ipString() + format(":%d", ntohs(addr.sin_port));
}

}
