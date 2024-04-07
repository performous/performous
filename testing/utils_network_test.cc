#include "common.hh"

#include "utils/network.hh"

#if defined(WIN32) || defined(__MINGW32__)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#endif

TEST(UnitTest_getLocalIPAddresses, returns_non_empty) {
    auto const ips = getLocalIPAddresses();
    EXPECT_FALSE(ips.empty());
}

namespace {
    bool isValidIP(std::string const& ip) {
#ifdef _WIN32
        return (inet_pton(AF_INET, ip.c_str(), nullptr) == 1 || inet_pton(AF_INET6, ip.c_str(), nullptr) == 1);
#else
        struct sockaddr_in sa;
        auto const  result = inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1 || inet_pton(AF_INET6, ip.c_str(), &(sa.sin_addr)) == 1;
        return result;
#endif
    }
}

TEST(UnitTest_getLocalIPAddresses, ips_are_valid) {
    auto const ips = getLocalIPAddresses();
    for (const auto& ip : ips) {
        std::cout << "ip address: " << ip << std::endl;

        EXPECT_TRUE(isValidIP(ip));
    }
}

TEST(UnitTest_getLocalIPAddresses, ips_with_local_host) {
    auto const ips = getLocalIPAddresses(false);

    EXPECT_THAT(ips, Contains("127.0.0.1"));
}

TEST(UnitTest_getLocalIPAddresses, ips_without_local_host) {
    auto const ips = getLocalIPAddresses(true);

    EXPECT_THAT(ips, Not(Contains("127.0.0.1")));
    EXPECT_THAT(ips, Not(Contains("::1")));
    EXPECT_THAT(ips, Not(IsEmpty()));
}

TEST(UnitTest_getLocalIPAddresses, ipv4) {
    auto const ips = getLocalIPAddresses(false, IPFilter::IPV4);

    EXPECT_THAT(ips, Contains("127.0.0.1"));
    EXPECT_THAT(ips, Not(Contains("::1")));
}

// This test will not succeed in the github pipeline due to missing ipv6 addresses.
// So it is marked as developer test.
// Disabled until the ctest run in pipeline will filter for UnitTest.
TEST(DeveloperTest_getLocalIPAddresses, DISABLED_ipv6) {
    auto const ips = getLocalIPAddresses(false, IPFilter::IPV6);

    EXPECT_THAT(ips, Not(Contains("127.0.0.1")));
    EXPECT_THAT(ips, Contains("::1"));
}
