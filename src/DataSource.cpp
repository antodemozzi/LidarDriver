#include "../include/DataSource.h"
#include "../include/Exceptions.h"
#include "../include/Logger.h"

#include <array>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>
#endif

FileScanSource::FileScanSource(const std::string& path, double startAngle,
                               double resolution, bool shouldRepeat)
    : scans(fileReading(path, startAngle, resolution)), repeat(shouldRepeat)
{
    if (scans.empty())
    {
        throw FileSourceError("Data file contains no scans");
    }
}

Scan FileScanSource::next()
{
    if (nextIndex >= scans.size())
    {
        if (!repeat)
        {
            throw FileSourceError("No more scans available in the data file");
        }
        nextIndex = 0;
    }
    Scan result = scans[nextIndex++];
    result.timestamp = std::chrono::system_clock::now();
    return result;
}

UdpScanSource::UdpScanSource(unsigned short port, double startAngleValue,
                             double resolutionValue)
    : socketHandle(-1), startAngle(startAngleValue), resolution(resolutionValue)
{
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
    {
        throw UdpSourceError("Unable to initialize Winsock");
    }
    socketHandle = static_cast<std::intptr_t>(
        socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
#else
    socketHandle = socket(AF_INET, SOCK_DGRAM, 0);
#endif
    if (socketHandle < 0)
    {
#ifdef _WIN32
        WSACleanup();
#endif
        throw UdpSourceError("Unable to create UDP socket");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(socketHandle, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    {
#ifdef _WIN32
        closesocket(static_cast<SOCKET>(socketHandle));
        WSACleanup();
#else
        close(socketHandle);
#endif
        socketHandle = -1;
        throw UdpSourceError("Unable to bind UDP port " + std::to_string(port));
    }
}

UdpScanSource::~UdpScanSource()
{
    if (socketHandle >= 0)
    {
#ifdef _WIN32
        closesocket(static_cast<SOCKET>(socketHandle));
        WSACleanup();
#else
        close(socketHandle);
#endif
    }
}

Scan UdpScanSource::next()
{
    std::array<char, 65536> buffer{};
#ifdef _WIN32
    const int received = recvfrom(static_cast<SOCKET>(socketHandle), buffer.data(),
                                  static_cast<int>(buffer.size() - 1), 0, nullptr, nullptr);
#else
    const int received = static_cast<int>(
        recvfrom(socketHandle, buffer.data(), buffer.size() - 1, 0, nullptr, nullptr));
#endif
    if (received <= 0)
    {
        if (received < 0)
        {
#ifdef _WIN32
            const int error = WSAGetLastError();
            if (error == WSAETIMEDOUT)
            {
                throw UdpTimeoutError();
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                throw UdpTimeoutError();
            }
#endif
        }
        throw UdpSourceError("UDP receive failed");
    }
    buffer[static_cast<std::size_t>(received)] = '\0';
    return parseUdpDatagram(buffer.data(), startAngle, resolution);
}

void UdpScanSource::setTimeoutMilliseconds(std::uint32_t timeoutMilliseconds)
{
    if (socketHandle < 0)
    {
        throw UdpSourceError("UDP socket is not available");
    }
#ifdef _WIN32
    const DWORD timeout = timeoutMilliseconds;
    if (setsockopt(static_cast<SOCKET>(socketHandle), SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout)) < 0)
    {
        throw UdpSourceError("Unable to configure UDP receive timeout");
    }
#else
    timeval timeout{};
    timeout.tv_sec = timeoutMilliseconds / 1000;
    timeout.tv_usec = static_cast<suseconds_t>((timeoutMilliseconds % 1000) * 1000);
    if (setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0)
    {
        throw UdpSourceError("Unable to configure UDP receive timeout");
    }
#endif
}

std::unique_ptr<ScanSource> createSource(const std::string& filePath, int udpPort,
                                         double startAngle, double resolution,
                                         bool repeat)
{
    if (startAngle < 0.0 || startAngle > 180.0)
    {
        throw DataSourceError("Start angle must be between 0 and 180 degrees");
    }
    if (!filePath.empty() && udpPort >= 0)
    {
        throw DataSourceError("Choose either --file or --udp, not both");
    }
    if (udpPort >= 0)
    {
        if (udpPort > 65535)
        {
            throw UdpSourceError("UDP port must be between 0 and 65535");
        }
        return std::unique_ptr<ScanSource>(new UdpScanSource(
            static_cast<unsigned short>(udpPort), startAngle, resolution));
    }
    return std::unique_ptr<ScanSource>(
        new FileScanSource(filePath, startAngle, resolution, repeat));
}
