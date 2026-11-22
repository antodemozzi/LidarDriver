#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

struct Measurement
{
    double value = 0.0;
    bool valid = false;
};

struct Scan
{
    std::vector<Measurement> distances;
    std::chrono::system_clock::time_point timestamp;
    double start_angle = 0.0;
    double resolution = 0.0;
    std::uint64_t sequence = 0;
    std::string sensor_id;
};

Scan parseScanCsv(const std::string& line, double startAngle = 0.0,
                  double resolution = 0.0);
std::vector<Scan> fileReading(const std::string& fileLink,
                              double startAngle = 0.0,
                              double resolution = 0.0);
Scan parseUdpDatagram(const std::string& datagram,
                      double defaultStartAngle = 0.0,
                      double defaultResolution = 0.0);

#endif
