#include "../include/DataSource.h"
#include "../include/Exceptions.h"
#include "../include/LidarDriver.h"
#include "../include/Logger.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
double parseDouble(const std::string& value, const std::string& name)
{
    std::size_t parsed = 0;
    const double result = std::stod(value, &parsed);
    if (parsed != value.size() || !std::isfinite(result))
    {
        throw std::invalid_argument("Invalid " + name + ": " + value);
    }
    return result;
}

int parseInt(const std::string& value, const std::string& name)
{
    std::size_t parsed = 0;
    const int result = std::stoi(value, &parsed);
    if (parsed != value.size())
    {
        throw std::invalid_argument("Invalid " + name + ": " + value);
    }
    return result;
}

void printScan(const Scan& scan)
{
    std::cout << "Scan (" << scan.distances.size() << " measurements): [ ";
    for (const auto& value : scan.distances)
    {
        if (value.valid)
        {
            std::cout << value.value;
        }
        else
        {
            std::cout << "invalid";
        }
        std::cout << ' ';
    }
    std::cout << "]\n";
}
}

int main(int argc, char* argv[])
{
    try
    {
        std::string filePath = "data/data.txt";
        int udpPort = -1;
        double resolution = 0.5;
        double startAngle = 0.0;
        std::size_t capacity = 10;
        bool repeat = false;
        std::uint32_t udpTimeoutMilliseconds = 3000;

        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            if (argument == "--file" && i + 1 < argc)
            {
                filePath = argv[++i];
            }
            else if (argument == "--udp" && i + 1 < argc)
            {
                udpPort = parseInt(argv[++i], "UDP port");
                filePath.clear();
            }
            else if (argument == "--resolution" && i + 1 < argc)
            {
                resolution = parseDouble(argv[++i], "resolution");
            }
            else if (argument == "--start-angle" && i + 1 < argc)
            {
                startAngle = parseDouble(argv[++i], "start angle");
            }
            else if (argument == "--capacity" && i + 1 < argc)
            {
                const int parsed = parseInt(argv[++i], "capacity");
                if (parsed <= 0)
                {
                    throw std::invalid_argument("Capacity must be greater than zero");
                }
                capacity = static_cast<std::size_t>(parsed);
            }
            else if (argument == "--no-repeat")
            {
                repeat = false;
            }
            else if (argument == "--repeat")
            {
                repeat = true;
            }
            else if (argument == "--udp-timeout-ms" && i + 1 < argc)
            {
                const int parsed = parseInt(argv[++i], "UDP timeout");
                if (parsed < 1)
                {
                    throw std::invalid_argument("UDP timeout must be greater than zero");
                }
                udpTimeoutMilliseconds = static_cast<std::uint32_t>(parsed);
            }
            else
            {
                throw std::invalid_argument("Unknown or incomplete option: " + argument);
            }
        }

        if (udpPort < 0 && filePath == "data/data.txt")
        {
            std::ifstream rootData(filePath);
            if (!rootData)
            {
                filePath = "../data/data.txt";
            }
        }

        LidarDriver lidar(resolution, capacity);
        std::unique_ptr<ScanSource> source =
            createSource(filePath, udpPort, startAngle, resolution, repeat);
        UdpScanSource* udpSource =
            dynamic_cast<UdpScanSource*>(source.get());
        if (udpSource)
        {
            udpSource->setTimeoutMilliseconds(udpTimeoutMilliseconds);
        }

        Logger::info("LidarDriver ready. Type 1-6 for an operation.");
        int command = 0;
        while (command != 6)
        {
            std::cout << "\n1 Add scan\n2 Print latest scan\n3 Extract oldest scan\n"
                         "4 Query distance\n5 Clear buffer\n6 Exit\n> ";
            if (!(std::cin >> command))
            {
                if (std::cin.eof())
                {
                    return 0;
                }
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid command.\n";
                continue;
            }

            try
            {
                switch (command)
                {
                case 1:
                    lidar.new_scan(source->next());
                    std::cout << "Scan added. Buffer: " << lidar.size() << '/'
                              << lidar.capacity() << '\n';
                    break;
                case 2:
                    std::cout << lidar << '\n';
                    break;
                case 3:
                    printScan(lidar.get_scan());
                    break;
                case 4:
                {
                    double angle = 0.0;
                    std::cout << "Angle: ";
                    std::cin >> angle;
                    const Measurement distance = lidar.get_distance(angle);
                    if (distance.valid)
                    {
                        std::cout << "Distance: " << distance.value << " m\n";
                    }
                    else
                    {
                        std::cout << "No valid measurement at this angle.\n";
                    }
                    break;
                }
                case 5:
                    lidar.clear_buffer();
                    std::cout << "Buffer cleared.\n";
                    break;
                case 6:
                    break;
                default:
                    std::cout << "Unknown command.\n";
                    break;
                }
            }
            catch (const std::exception& error)
            {
                Logger::error(std::string("Operation failed: ") + error.what());
            }
        }
    }
    catch (const std::exception& error)
    {
        Logger::error(error.what());
        return 1;
    }
    return 0;
}
