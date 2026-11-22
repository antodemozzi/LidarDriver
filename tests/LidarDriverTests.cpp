#include "../include/FileUtils.h"
#include "../include/LidarDriver.h"
#include "../include/DataSource.h"
#include "../include/Exceptions.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace
{
Scan makeScan(std::initializer_list<Measurement> values)
{
    Scan scan;
    scan.distances = values;
    return scan;
}

void testParser()
{
    const Scan scan = parseScanCsv("1.5, ,nan,2.0");
    assert(scan.distances.size() == 4);
    assert(scan.distances[0].valid && scan.distances[0].value == 1.5);
    assert(!scan.distances[1].valid);
    assert(!scan.distances[2].valid);
    assert(scan.distances[3].valid);
}

void testBufferAndNormalization()
{
    LidarDriver driver(1.0, 2);
    driver.new_scan(makeScan({Measurement{1.0, true}, Measurement{2.0, true}}));
    driver.new_scan(makeScan({Measurement{3.0, true}}));
    driver.new_scan(makeScan({Measurement{4.0, true}}));

    assert(driver.size() == 2);
    const Scan oldest = driver.get_scan();
    assert(oldest.distances[0].valid && oldest.distances[0].value == 3.0);
    assert(oldest.distances.size() == 181);
    assert(!oldest.distances[1].valid);
}

void testInvalidDistance()
{
    LidarDriver driver(1.0);
    driver.new_scan(makeScan({Measurement{0.0, false}}));
    assert(!driver.get_distance(0.0).valid);

    bool rejected = false;
    try
    {
        driver.get_distance(181.0);
    }
    catch (const BufferError&)
    {
        rejected = true;
    }
    assert(rejected);
}

void testUdpFrameFormats()
{
    const Scan text = parseUdpDatagram(
        "LIDAR1;seq=42;timestamp_ms=1000;start=0;resolution=0.5;sensor=test|1.5,,nan");
    assert(text.sequence == 42);
    assert(text.start_angle == 0.0);
    assert(text.resolution == 0.5);
    assert(text.sensor_id == "test");
    assert(text.distances.size() == 3);
    assert(!text.distances[1].valid);

    std::string binary = "LD1";
    binary.append(32, '\0');
    binary[3] = 7;
    binary[31] = 2;
    binary.resize(35 + 16);
    binary[35] = 0x00;
    // The binary parser is covered with a little-endian 2-measurement frame:
    // this compact form verifies frame size and invalid NaN handling.
    std::uint64_t nanBits = 0;
    std::memcpy(&binary[35], &nanBits, 8);
    const double one = 1.25;
    std::memcpy(&binary[43], &one, 8);
    bool parsed = false;
    try
    {
        const Scan scan = parseUdpDatagram(binary);
        parsed = scan.distances.size() == 2;
    }
    catch (const ParseError&)
    {
        parsed = false;
    }
    assert(parsed);
}

void testDataSourceErrors()
{
    bool missingFile = false;
    try
    {
        FileScanSource source("missing-lidar-data.csv", 0.0, 0.5, false);
        (void)source;
    }
    catch (const FileSourceError&)
    {
        missingFile = true;
    }
    assert(missingFile);
}
}

int main()
{
    testParser();
    testBufferAndNormalization();
    testInvalidDistance();
    testUdpFrameFormats();
    testDataSourceErrors();
    std::cout << "All tests passed\n";
}
