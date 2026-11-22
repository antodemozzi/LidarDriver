#ifndef LIDAR_LOGGER_H
#define LIDAR_LOGGER_H

#include <iostream>
#include <string>

namespace Logger
{
inline void info(const std::string& message)
{
    std::cout << "[info] " << message << '\n';
}

inline void warning(const std::string& message)
{
    std::cerr << "[warning] " << message << '\n';
}

inline void error(const std::string& message)
{
    std::cerr << "[error] " << message << '\n';
}
}

#endif
