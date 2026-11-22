#include "../include/LidarDriver.h"
#include "../include/Exceptions.h"

#include <cmath>
#include <iomanip>
#include <stdexcept>

namespace
{
double validateResolution(double value)
{
    if (!std::isfinite(value) || value < LidarDriver::RES_MIN ||
        value > LidarDriver::RES_MAX)
    {
        throw BufferError("Resolution must be finite and between 0.1 and 1.0");
    }
    return value;
}
}

LidarDriver::LidarDriver(double resolutionValue, std::size_t capacityValue)
    : resolution(validateResolution(resolutionValue)),
      scanDimension(static_cast<std::size_t>(
          std::llround(180.0 / validateResolution(resolutionValue))) + 1)
{
    if (capacityValue == 0)
    {
        throw BufferError("Buffer capacity must be greater than zero");
    }
    buffer.resize(capacityValue);
}

Scan LidarDriver::normalize(const Scan& scan) const
{
    Scan normalized = scan;
    normalized.resolution = resolution;
    normalized.distances.resize(scanDimension);
    return normalized;
}

void LidarDriver::new_scan(const Scan& scan)
{
    if (scan.distances.empty())
    {
        throw BufferError("A scan cannot be empty");
    }
    buffer[tail] = normalize(scan);
    tail = (tail + 1) % buffer.size();
    if (count < buffer.size())
    {
        ++count;
    }
    else
    {
        head = (head + 1) % buffer.size();
    }
}

Scan LidarDriver::get_scan()
{
    if (count == 0)
    {
        throw BufferError("No scans available in the buffer");
    }
    Scan result = std::move(buffer[head]);
    buffer[head] = Scan{};
    head = (head + 1) % buffer.size();
    --count;
    return result;
}

void LidarDriver::clear_buffer()
{
    for (Scan& scan : buffer)
    {
        scan = Scan{};
    }
    head = tail = count = 0;
}

Measurement LidarDriver::get_distance(double angle) const
{
    if (count == 0)
    {
        throw BufferError("No scans available in the buffer");
    }
    if (!std::isfinite(angle) || angle < MIN_ANGLE || angle > MAX_ANGLE)
    {
        throw BufferError("Angle must be finite and between 0 and 180 degrees");
    }

    const Scan& scan = latest_scan();
    const std::size_t index = static_cast<std::size_t>(
        std::llround((angle - scan.start_angle) / resolution));
    if (index >= scan.distances.size() || !scan.distances[index].valid)
    {
        return Measurement{0.0, false};
    }
    return scan.distances[index];
}

std::size_t LidarDriver::size() const noexcept
{
    return count;
}

std::size_t LidarDriver::capacity() const noexcept
{
    return buffer.size();
}

const Scan& LidarDriver::latest_scan() const
{
    if (count == 0)
    {
        throw BufferError("No scans available in the buffer");
    }
    return buffer[(tail + buffer.size() - 1) % buffer.size()];
}

std::ostream& operator<<(std::ostream& os, const LidarDriver& lidar)
{
    if (lidar.size() == 0)
    {
        return os << "No scans available in the buffer.";
    }

    const Scan& scan = lidar.latest_scan();
    os << "Last scan (" << scan.distances.size() << " measurements): [ ";
    for (const auto& value : scan.distances)
    {
        if (value.valid)
        {
            os << std::setprecision(6) << value.value;
        }
        else
        {
            os << "invalid";
        }
        os << ' ';
    }
    return os << ']';
}
