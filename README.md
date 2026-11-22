# LidarDriver

LidarDriver is a C++17 application that acquires LiDAR scans from CSV files or
UDP datagrams and stores them in a configurable circular FIFO buffer. The
project provides a lightweight foundation for integrating real sensors while
keeping acquisition, validation, and driver logic separate.

## Features

- CSV file and UDP datagram sources;
- scan timestamps, start angles, resolutions, sequence numbers, and sensor IDs;
- missing or invalid measurements represented as `Measurement.valid == false`,
  never as `0.0` (zero remains a valid measurement);
- scan normalization to the configured dimension, padding missing values and
  truncating oversized scans;
- configurable circular FIFO buffer;
- nearest-supported-angle distance queries;
- dedicated parsing, buffer, file, and UDP exceptions;
- structured `info`, `warning`, and `error` logging;
- text and binary `LD1` UDP frame formats;
- automatic tests through CTest.

## Data format

Each file row or UDP datagram can contain a CSV scan:

```text
1.03,0.17,4.89,,nan,3.08
```

Values must be finite and non-negative. An empty field or `nan` represents an
unavailable measurement. Scans may contain different numbers of measurements;
the driver adapts them to the configured resolution.

Text UDP frames use the following format:

```text
LIDAR1;seq=42;timestamp_ms=1700000000000;start=0;resolution=0.5;sensor=lidar-1|1.03,0.17,4.89
```

The binary UDP format starts with the `LD1` magic value, followed by:

- sequence number: 32-bit little-endian integer;
- timestamp: 64-bit little-endian integer, in milliseconds;
- start angle: 64-bit little-endian `double`;
- resolution: 64-bit little-endian `double`;
- measurement count: 32-bit little-endian integer;
- measurements: one 64-bit little-endian `double` per value.

The binary frame has a 35-byte header. Negative, infinite, or otherwise invalid
measurement values are converted to unavailable measurements.

## Build

Requirements:

- CMake 3.16 or newer;
- a C++17 compiler.

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The Windows build automatically links `ws2_32` for UDP support.

## Usage

Use the CSV file source and stop at the end of the file:

```powershell
.\build\LidarDriver.exe --file data\data.txt --resolution 0.5 --capacity 10 --no-repeat
```

Reuse file scans cyclically:

```powershell
.\build\LidarDriver.exe --file data\data.txt --resolution 0.5 --repeat
```

Listen for UDP datagrams:

```powershell
.\build\LidarDriver.exe --udp 9000 --resolution 0.5 --start-angle 0
```

Set the UDP receive timeout:

```powershell
.\build\LidarDriver.exe --udp 9000 --udp-timeout-ms 500
```

A sensor or external process can send CSV, text-framed, or binary-framed data
to the configured UDP port. The receive operation is limited by the configured
timeout.

## Command-line options

| Option | Description |
|---|---|
| `--file PATH` | Use a CSV file as the source |
| `--udp PORT` | Listen for UDP datagrams on a port |
| `--resolution DEGREES` | Resolution from `0.1` to `1.0` |
| `--start-angle DEGREES` | Initial scan angle |
| `--capacity N` | Maximum number of scans in the buffer |
| `--no-repeat` | Stop when a file reaches its end |
| `--repeat` | Restart a file from the beginning when it ends |
| `--udp-timeout-ms N` | UDP receive timeout in milliseconds |

## Interactive commands

| Command | Behavior |
|---|---|
| `1` | Acquire and insert a scan |
| `2` | Print the latest scan |
| `3` | Extract the oldest scan |
| `4` | Query a distance by angle |
| `5` | Clear the buffer |
| `6` | Exit |

If no valid measurement exists at an angle, the program reports
`No valid measurement at this angle` instead of interpreting it as a distance
of zero.

## Project structure

```text
LidarDriver/
├── .github/workflows/ci.yml
├── CMakeLists.txt
├── data/data.txt
├── include/
│   ├── DataSource.h
│   ├── Exceptions.h
│   ├── FileUtils.h
│   ├── LidarDriver.h
│   └── Logger.h
├── src/
│   ├── DataSource.cpp
│   ├── FileUtils.cpp
│   ├── LidarDriver.cpp
│   └── main.cpp
└── tests/LidarDriverTests.cpp
```

## Current limitations

UDP does not provide reliable delivery. Duplicate, out-of-order, or missing
packets must be handled by the sensor or a dedicated protocol adapter.
Noise filtering, sensor synchronization, and graphical visualization are not
included in this prototype.

## Authors

- Larisa Apostol - ID 2101805
- Anna Cazzadore - ID 2101825
- Giada Ruggero - ID 2101765
