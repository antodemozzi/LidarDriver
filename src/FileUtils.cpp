#include "../include/FileUtils.h"
#include "../include/Exceptions.h"

#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
std::string trim(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos)
    {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}
}

Scan parseScanCsv(const std::string& line, double startAngle, double resolution)
{
    Scan scan;
    scan.timestamp = std::chrono::system_clock::now();
    scan.start_angle = startAngle;
    scan.resolution = resolution;

    std::stringstream row(line);
    std::string token;
    std::size_t position = 0;
    while (std::getline(row, token, ','))
    {
        token = trim(token);
        if (token.empty() || token == "nan" || token == "NaN")
        {
            scan.distances.push_back(Measurement{0.0, false});
        }
        else
        {
            try
            {
                std::size_t parsed = 0;
                const double value = std::stod(token, &parsed);
                if (parsed != token.size() || !std::isfinite(value) || value < 0.0)
                {
                    throw std::invalid_argument("value must be finite and non-negative");
                }
                scan.distances.push_back(Measurement{value, true});
            }
            catch (const std::exception& error)
            {
                throw ParseError(
                    "Invalid value at column " + std::to_string(position + 1) +
                    ": " + error.what());
            }
        }
        ++position;
    }

    if (scan.distances.empty())
    {
        throw ParseError("A scan cannot be empty");
    }
    return scan;
}

std::vector<Scan> fileReading(const std::string& fileLink, double startAngle,
                              double resolution)
{
    std::ifstream dataFile(fileLink);
    if (!dataFile)
    {
        throw FileSourceError("Unable to open file " + fileLink);
    }

    std::vector<Scan> data;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(dataFile, line))
    {
        ++lineNumber;
        if (trim(line).empty())
        {
            continue;
        }
        try
        {
            data.push_back(parseScanCsv(line, startAngle, resolution));
        }
        catch (const std::exception& error)
        {
            throw ParseError("Invalid scan at line " +
                             std::to_string(lineNumber) + ": " +
                             error.what());
        }
    }
    return data;
}

namespace
{
std::uint32_t readLe32(const std::string& data, std::size_t offset)
{
    return static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 3])) << 24);
}

std::uint64_t readLe64(const std::string& data, std::size_t offset)
{
    std::uint64_t result = 0;
    for (std::size_t i = 0; i < 8; ++i)
    {
        result |= static_cast<std::uint64_t>(
                      static_cast<unsigned char>(data[offset + i])) << (8 * i);
    }
    return result;
}

double readLeDouble(const std::string& data, std::size_t offset)
{
    const std::uint64_t bits = readLe64(data, offset);
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
}

Scan parseUdpDatagram(const std::string& datagram, double defaultStartAngle,
                      double defaultResolution)
{
    if (datagram.size() >= 3 && datagram.compare(0, 3, "LD1") == 0)
    {
        constexpr std::size_t headerSize = 35;
        if (datagram.size() < headerSize)
        {
            throw ParseError("Truncated binary UDP frame");
        }
        const std::uint32_t count = readLe32(datagram, 31);
        if (count > (datagram.size() - headerSize) / 8 ||
            datagram.size() != headerSize + static_cast<std::size_t>(count) * 8)
        {
            throw ParseError("Binary UDP frame size does not match measurement count");
        }

        Scan scan;
        scan.sequence = readLe32(datagram, 3);
        const std::uint64_t timestampMs = readLe64(datagram, 7);
        scan.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestampMs));
        scan.start_angle = readLeDouble(datagram, 15);
        scan.resolution = readLeDouble(datagram, 23);
        if (!std::isfinite(scan.start_angle) || !std::isfinite(scan.resolution))
        {
            throw ParseError("Invalid angles or resolution in binary UDP frame");
        }
        for (std::uint32_t i = 0; i < count; ++i)
        {
            const double value = readLeDouble(datagram, headerSize + i * 8);
            if (!std::isfinite(value) || value < 0.0)
            {
                scan.distances.push_back(Measurement{0.0, false});
            }
            else
            {
                scan.distances.push_back(Measurement{value, true});
            }
        }
        if (scan.distances.empty())
        {
            throw ParseError("Binary UDP frame cannot be empty");
        }
        return scan;
    }

    if (datagram.rfind("LIDAR1;", 0) == 0)
    {
        const std::size_t separator = datagram.find('|');
        if (separator == std::string::npos)
        {
            throw ParseError("Missing text UDP frame header separator");
        }

        Scan scan = parseScanCsv(datagram.substr(separator + 1),
                                 defaultStartAngle, defaultResolution);
        const std::string header = datagram.substr(7, separator - 7);
        std::stringstream fields(header);
        std::string field;
        while (std::getline(fields, field, ';'))
        {
            if (field.rfind("seq=", 0) == 0)
            {
                scan.sequence = std::stoull(field.substr(4));
            }
            else if (field.rfind("timestamp_ms=", 0) == 0)
            {
                const std::uint64_t milliseconds = std::stoull(field.substr(13));
                scan.timestamp = std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(milliseconds));
            }
            else if (field.rfind("start=", 0) == 0)
            {
                scan.start_angle = std::stod(field.substr(6));
            }
            else if (field.rfind("resolution=", 0) == 0)
            {
                scan.resolution = std::stod(field.substr(11));
            }
            else if (field.rfind("sensor=", 0) == 0)
            {
                scan.sensor_id = field.substr(7);
            }
        }
        return scan;
    }

    return parseScanCsv(datagram, defaultStartAngle, defaultResolution);
}
