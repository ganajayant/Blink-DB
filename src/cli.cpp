/**
 * @file cli.cpp
 * @brief A simple command-line program that processes user input.
 *
 * This program provides a command-line interface where the user can input
 * commands. It uses a `CommandParser` object to process each line of input.
 * The program is designed to handle SIGINT (Ctrl + C) gracefully and provides
 * an interactive prompt for the user to enter commands.
 *
 * @details
 * When the user presses Ctrl + C (SIGINT), the program will display a message
 * reminding the user to exit using the "exit" command or Ctrl + D. The program
 * clears the screen on startup and repeatedly prompts for user input until
 * the program is terminated.
 *
 * @note The command-line input is passed to a `CommandParser` class for parsing
 *       and processing.
 *
 * @author Gana Jayant Sigadam
 * @date March 2025
 * @version 1.0
 */

#include "command_parser.hpp"
#include <iostream>
#include <signal.h>
#include <string>

/**
 * @brief Returns the prompt header string for the user input.
 *
 * This function provides a string that is displayed as a prompt
 * to the user when accepting input.
 *
 * @return A string representing the prompt header (e.g., "User> ").
 */
std::string getHeader() {
    return "User> ";
}

/**
 * @brief Signal handler for SIGINT (Ctrl + C) interrupt.
 *
 * This function is triggered when the user presses Ctrl + C.
 * It displays a message prompting the user to use the exit command
 * or Ctrl + D to exit the program.
 *
 * @param signum The signal number (not used in this case).
 */
void handle_sigint(int) {
    std::cout << "\nuse exit command to exit or use Ctrl + d\n"
              << getHeader();
    std::cout.flush();
}

/**
 * @brief Main function that processes user input.
 *
 * This function initializes signal handling, clears the terminal screen,
 * and enters a loop where it continually prompts the user for input.
 * The input is passed to a CommandParser instance, which processes the command.
 *
 * @param argc The number of command-line arguments (not used in this case).
 * @param argv The command-line arguments (not used in this case).
 * @return An integer indicating the status of the program execution (0 for success).
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // Set up the SIGINT signal handler
    signal(SIGINT, handle_sigint);

    // Clear the terminal screen
    std::cout << "\033[2J\033[1;1H";

    // Create an instance of the CommandParser class
    CommandParser parser;

    // Continuously prompt the user and process the input
    for (std::string line; std::cout << getHeader() && getline(std::cin, line);) {
        // Only parse and print results if the line is not empty
        if (!line.empty()) {
            std::cout << parser.parser(line);
        }
    }

    return 0;
}
