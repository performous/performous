#include "network.hh"

#include <cstring>
#include <memory>
#include <stdexcept>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif


std::vector<std::string> getLocalIPAddresses(bool noLocalHost, IPFilter filter) {
    auto ipAddresses = std::vector<std::string>{};

#ifdef WIN32
    ULONG bufferSize = 0;
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, nullptr, &bufferSize) != ERROR_BUFFER_OVERFLOW) {
        throw std::runtime_error("GetAdaptersAddresses failed to get buffer size");
    }

    std::unique_ptr<char[]> buffer(new char[bufferSize]);
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get()), &bufferSize) != ERROR_SUCCESS) {
        throw std::runtime_error("GetAdaptersAddresses failed");
    }

    PIP_ADAPTER_ADDRESSES adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get());
    while (adapterAddresses) {
        if (adapterAddresses->FirstUnicastAddress) {
            sockaddr* sockaddrPtr = adapterAddresses->FirstUnicastAddress->Address.lpSockaddr;

            if (sockaddrPtr->sa_family == AF_INET && (filter == IPFilter::IPV4 || filter == IPFilter::Both)) {
                char ipAddress[NI_MAXHOST];
                if (getnameinfo(sockaddrPtr, sizeof(sockaddr), ipAddress, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) == 0) {
                    std::string ipAddr(ipAddress);

                    // Überprüfen, ob lokale Host-IP-Adressen ausgelassen werden sollen
                    if (!noLocalHost || ipAddr != "127.0.0.1") {
                        ipAddresses.push_back(ipAddr);
                    }
                }
            }
            else if (sockaddrPtr->sa_family == AF_INET6 && (filter == IPFilter::IPV6 || filter == IPFilter::Both)) {
                char ipAddress[NI_MAXHOST];
                if (getnameinfo(sockaddrPtr, sizeof(sockaddr), ipAddress, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) == 0) {
                    std::string ipAddr(ipAddress);

                    // Überprüfen, ob lokale Host-IP-Adressen ausgelassen werden sollen
                    if (!noLocalHost || ipAddr != "::1") {
                        ipAddresses.push_back(ipAddr);
                    }
                }
            }
        }
        adapterAddresses = adapterAddresses->Next;
    }
#else
    struct ifaddrs* ifaddr, * ifa;
    if (getifaddrs(&ifaddr) == -1) {
        throw std::runtime_error("getifaddrs failed");
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr)
            continue;

        if (ifa->ifa_addr->sa_family != AF_INET && ifa->ifa_addr->sa_family != AF_INET6)
            continue;

        if ((filter == IPFilter::IPV4 && ifa->ifa_addr->sa_family != AF_INET) ||
            (filter == IPFilter::IPV6 && ifa->ifa_addr->sa_family != AF_INET6))
            continue;

        void* addr;

        // Konvertiere die Netzwerkadresse in eine lesbare Zeichenfolge
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            addr = &(addr_in->sin_addr);
        }
        else {
            struct sockaddr_in6* addr_in6 = reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_addr);
            addr = &(addr_in6->sin6_addr);
        }

        char ipBuffer[INET6_ADDRSTRLEN];
        if (inet_ntop(ifa->ifa_addr->sa_family, addr, ipBuffer, sizeof(ipBuffer)) == nullptr) {
            throw std::runtime_error("inet_ntop failed");
        }

        auto const ipAddr = std::string{ ipBuffer };

        if (noLocalHost && (ipAddr == "127.0.0.1" || ipAddr == "::1"))
            continue;

        ipAddresses.emplace_back(ipAddr);
    }

    freeifaddrs(ifaddr);

#endif // WIN32
    return ipAddresses;
}

std::vector<std::string> getLocalIPAddresses(IPFilter filter, bool noLocalHost) {
    return getLocalIPAddresses(noLocalHost, filter);
}

