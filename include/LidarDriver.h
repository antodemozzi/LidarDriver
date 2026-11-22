#ifndef LIDARDRIVER_H
#define LIDARDRIVER_H

#include "FileUtils.h"

#include <cstddef>
#include <iostream>
#include <vector>

class LidarDriver
{
public:
    static constexpr double RES_MIN = 0.1;
    static constexpr double RES_MAX = 1.0;
    static constexpr double MAX_ANGLE = 180.0;
    static constexpr double MIN_ANGLE = 0.0;

    explicit LidarDriver(double resolution, std::size_t capacity = 10);

    void new_scan(const Scan& scan);
    Scan get_scan();
    void clear_buffer();
    Measurement get_distance(double angle) const;
    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;
    const Scan& latest_scan() const;
    friend std::ostream& operator<<(std::ostream& os, const LidarDriver& lidar);

private:
    std::vector<Scan> buffer;
    std::size_t head = 0;
    std::size_t tail = 0;
    std::size_t count = 0;
    const double resolution;
    std::size_t scanDimension;

    Scan normalize(const Scan& scan) const;
};

#endif
