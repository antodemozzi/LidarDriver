#ifndef DATASOURCE_H
#define DATASOURCE_H

#include "FileUtils.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class ScanSource
{
public:
    virtual ~ScanSource() = default;
    virtual Scan next() = 0;
};

class FileScanSource final : public ScanSource
{
public:
    FileScanSource(const std::string& path, double startAngle, double resolution,
                   bool repeat);
    Scan next() override;

private:
    std::vector<Scan> scans;
    std::size_t nextIndex = 0;
    bool repeat;
};

class UdpScanSource final : public ScanSource
{
public:
    UdpScanSource(unsigned short port, double startAngle, double resolution);
    ~UdpScanSource() override;
    Scan next() override;
    void setTimeoutMilliseconds(std::uint32_t timeoutMilliseconds);

private:
    std::intptr_t socketHandle;
    double startAngle;
    double resolution;
};

std::unique_ptr<ScanSource> createSource(const std::string& filePath,
                                         int udpPort, double startAngle,
                                         double resolution, bool repeat);

#endif
