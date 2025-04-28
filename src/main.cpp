/**
 * @file main.cpp
 * @author Gana Jayant Sigadam
 * @brief
 * @version 1.0
 * @date March 2025
 */

#include "kqueue_server.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

/*
 * @brief Main function for the KqueueServer.
 *
 * This function initializes the server, sets up signal handling,
 * and starts the server loop to accept and handle client connections.
 *
 * @param argc The number of command-line arguments.
 * @param argv The command-line arguments.
 * @return int Exit status of the program.
 */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    constexpr const char *ADDR = "127.0.0.1";
    constexpr int PORT = 9001;

    std::cout << "\033[2J\033[1;1H";

    try {
        KqueueServer server(ADDR, PORT);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}