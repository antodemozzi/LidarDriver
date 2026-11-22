#ifndef LIDAR_EXCEPTIONS_H
#define LIDAR_EXCEPTIONS_H

#include <stdexcept>
#include <string>

class LidarError : public std::runtime_error
{
public:
    explicit LidarError(const std::string& message) : std::runtime_error(message) {}
};

class ParseError : public LidarError
{
public:
    explicit ParseError(const std::string& message) : LidarError(message) {}
};

class BufferError : public LidarError
{
public:
    explicit BufferError(const std::string& message) : LidarError(message) {}
};

class DataSourceError : public LidarError
{
public:
    explicit DataSourceError(const std::string& message) : LidarError(message) {}
};

class FileSourceError : public DataSourceError
{
public:
    explicit FileSourceError(const std::string& message) : DataSourceError(message) {}
};

class UdpSourceError : public DataSourceError
{
public:
    explicit UdpSourceError(const std::string& message) : DataSourceError(message) {}
};

class UdpTimeoutError : public UdpSourceError
{
public:
    explicit UdpTimeoutError(const std::string& message = "UDP receive timed out")
        : UdpSourceError(message) {}
};

#endif
