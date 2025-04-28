# Blink-DB

## Overview
Blink-DB is a high-performance, write-optimized database system implemented in C++. It consists of two core components integrated into a single codebase:

- **Database Engine** – Uses **Log-Structured Merge-Trees (LSM-Trees)** for fast writes and efficient data retrieval
- **Server** – A non-blocking, event-driven server leveraging **kqueue** for high-performance RESP command processing

## Project Structure
```
.
├── benchmark.sh
├── Design Document.pdf
├── docs
│   ├── Doxyfile
│   └── html
├── images
│   └── LSM Tree.png
├── Makefile
├── README.md
├── report
│   ├── main.tex
│   └── pictures
└── src
    ├── cli.cpp
    ├── command_parser.hpp
    ├── engine
    ├── kqueue_server.hpp
    ├── main.cpp
    ├── resp
    └── resp_encoder.hpp
```

## DB Architecture

![DB Architecture](./images/LSM%20Tree.png "DB Architecture")

## Features
- **Write-Major Architecture**: Optimized for fast writes, ensuring quick insert operations
- **LSM-Tree Based Storage**: Efficiently manages and compacts data for optimized read and write performance
- **Thread-Safe Execution**: Supports concurrent operations using internal synchronization mechanisms
- **Non-blocking** kqueue-based server for high throughput
- **RESP command processing** (`GET`, `SET`, `DEL`)
- Multiple interfaces:
  - Command-line interface for direct interaction
  - Server interface for networked applications

## Compilation and Execution

### Build and Run the Server
```sh
make run
```
- Compiles `src/main.cpp` with C++17 standard
- Executes the database server

### Build and Run the CLI
```sh
make cli
```
- Compiles `src/cli.cpp` with C++17 standard
- Executes the command-line interface

### Benchmarking
```sh
make benchmark
```
This runs `redis-benchmark` with different loads and stores results in the `result/` directory. The benchmarking script tests with:

| Data Size | Key Size | Concurrent Requests | Parallel Connections |
|-----------|---------|---------------------|----------------------|
| 512 bytes | 1024    | 10,000, 100,000, 1,000,000 | 10, 100, 1000 |

### Generate Documentation
```sh
make docs
```
- Generates documentation using Doxygen

### Clean Up Binaries
```sh
make prune
```
- Removes compiled binaries and data files

## Usage Instructions

### CLI Mode

#### Supported Commands

##### Data Operations

###### Set a key-value pair
```sh
SET <key> <value>
```
Stores the key-value pair in the database.

###### Retrieve a value
```sh
GET <key>
```
Fetches the value associated with the given key.

###### Delete a key
```sh
DEL <key>
```
Removes the key-value pair from the database.

##### Utility Commands

###### Show available commands
```sh
help
```
Displays the list of supported commands.

###### Clear the screen
```sh
clear
```
Clears the CLI screen.

###### Exit the application
```sh
exit
```
Terminates the Blink-DB CLI session.

##### Example CLI Usage

```sh
SET name "BlinkDB"
GET name
DEL name
GET name  # Returns NULL
```

### Server Mode

The server listens on port `9001` by default and uses the Redis Serialization Protocol (RESP) for communication.

#### Telnet Testing
You can interact with the server using telnet on port `9001`.

##### Store a key-value pair (`SET`)
```sh
telnet 127.0.0.1 9001
*3
$3
SET
$3
foo
$3
bar
```

##### Retrieve a value (`GET`)
```sh
telnet 127.0.0.1 9001
*2
$3
GET
$3
foo
```

##### Delete a key (`DEL`)
```sh
telnet 127.0.0.1 9001
*2
$3
DEL
$3
foo
```

## Benchmark Results

The benchmark is run with the following parameters:
- **Number of requests**: 10000, 100000, 1000000
- **Number of parallel connections**: 10, 100, 1000
- **Data size**: 512 bytes
- **Random key range**: 1024 keys
- **Operations**: SET and GET

### Performance Results

| Concurrent Requests | Parallel Connections | Operation | Throughput (requests/s) | Avg. Latency (ms) |
|---------------------|----------------------|-----------|-------------------------|-------------------|
| 10000               | 10                   | SET       | 71942.45                | 0.132             |
| 10000               | 10                   | GET       | 86956.52                | 0.103             |
| 10000               | 100                  | SET       | 75187.97                | 1.311             |
| 10000               | 100                  | GET       | 103092.78               | 0.899             |
| 10000               | 1000                 | SET       | 73529.41                | 12.478            |
| 10000               | 1000                 | GET       | 90909.09                | 9.853             |
| 100000              | 10                   | SET       | 74460.16                | 0.128             |
| 100000              | 10                   | GET       | 91157.70                | 0.095             |
| 100000              | 100                  | SET       | 76923.08                | 1.293             |
| 100000              | 100                  | GET       | 94517.96                | 1.000             |
| 100000              | 1000                 | SET       | 76687.12                | 12.899            |
| 100000              | 1000                 | GET       | 91659.03                | 10.739            |
| 1000000             | 10                   | SET       | 73735.44                | 0.129             |
| 1000000             | 10                   | GET       | 80250.38                | 0.109             |
| 1000000             | 100                  | SET       | 70766.40                | 1.400             |
| 1000000             | 100                  | GET       | 86933.84                | 1.084             |
| 1000000             | 1000                 | SET       | 68465.02                | 14.572            |
| 1000000             | 1000                 | GET       | 84118.45                | 11.790            |

## Additional Notes

- Blink-DB is designed for high-speed writes, making it ideal for write-heavy workloads
- Reads are optimized through efficient indexing and LSM-Tree compaction
- The server implementation is optimized for high-throughput network operations
- Future versions will include additional features such as range queries, snapshots, and replication

For more details, refer to the documentation in the `docs/` directory.

---

**Author:** Gana Jayant Sigadam
**Date:** March 2025
**Version:** 1.0