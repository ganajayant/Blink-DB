/**
 * @file command_parser.hpp
 * @brief Command Parser for command-line interface to interact with Database engine.
 * @details This class provides methods to parse and validate commands such as
 *          SET, GET, DEL, and help commands.
 * @author Gana Jayant Sigadam
 * @date March 2025
 * @version 1.0
 */
#ifndef COMMAND_PARSER_HPP
#define COMMAND_PARSER_HPP

#include "engine/lsm.hpp"
#include "resp_encoder.hpp"
#include <string>
#include <utility>
#include <vector>

/**
 * @class ValidationResult
 * @brief Holds the result of command validation.
 *
 * The `ValidationResult` class encapsulates the outcome of a validation
 * process, including whether it was successful, an error message (if any),
 * and optional key-value pairs related to the validation.
 */
class ValidationResult {
public:
    /**
     * @brief Indicates whether the validation was successful.
     */
    bool success;
    /**
     * @brief Error message if validation failed.
     */
    std::string errmsg;
    /**
     * @brief Key associated with the command.
     */
    std::string key;
    /**
     * @brief Value associated with the command.
     */
    std::string value;
    /**
     * @brief Default constructor initializes the result to failure.
     */
    ValidationResult() : success(false), errmsg(""), key(""), value("") {}
};

/**
 * @class CommandParser
 * @brief Parses and validates commands for a command-line interface.
 *
 * The `CommandParser` class provides methods to parse user input,
 * validate commands, and interact with the underlying database engine.
 */
class CommandParser {
private:
    /**
     * @brief Splits a string into tokens based on whitespace and quotes.
     *
     * This method tokenizes the input string, handling quoted strings
     * as single tokens and ignoring whitespace outside of quotes.
     *
     * @param str The input string to tokenize.
     * @return A vector of tokens extracted from the input string.
     */
    static std::vector<std::string> split(const std::string &str) {
        std::vector<std::string> tokens;
        std::string token;
        bool inQuotes = false;
        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (std::isspace(c) && !inQuotes) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
        return tokens;
    }

    /**
     * @brief Validates the SET command.
     *
     * This method checks the number of arguments and the validity of the key
     * and value for the SET command.
     *
     * @param tokens The tokens extracted from the input string.
     * @param result The ValidationResult object to store validation results.
     * @return True if validation is successful, false otherwise.
     */
    static bool validate_set(const std::vector<std::string> &tokens, ValidationResult &result) {
        if (tokens.size() != 3) {
            result.success = false;
            result.errmsg = "wrong number of arguments for 'set' command";
            return false;
        }
        if (tokens[1].empty()) {
            result.success = false;
            result.errmsg = "invalid key";
            return false;
        }
        result.key = tokens[1];
        result.value = tokens[2];
        result.success = true;
        return true;
    }

    /**
     * @brief Validates the GET command.
     *
     * This method checks the number of arguments and the validity of the key
     * for the GET command.
     *
     * @param tokens The tokens extracted from the input string.
     * @param result The ValidationResult object to store validation results.
     * @return True if validation is successful, false otherwise.
     */
    static bool validate_get(const std::vector<std::string> &tokens, ValidationResult &result) {
        if (tokens.size() != 2) {
            result.success = false;
            result.errmsg = "wrong number of arguments for 'get' command";
            return false;
        }
        if (tokens[1].empty()) {
            result.success = false;
            result.errmsg = "invalid key";
            return false;
        }
        result.key = tokens[1];
        result.success = true;
        return true;
    }

    /**
     * @brief Validates the DEL command.
     *
     * This method checks the number of arguments and the validity of the key
     * for the DEL command.
     *
     * @param tokens The tokens extracted from the input string.
     * @param result The ValidationResult object to store validation results.
     * @return True if validation is successful, false otherwise.
     */
    static bool validate_del(const std::vector<std::string> &tokens, ValidationResult &result) {
        if (tokens.size() != 2) {
            result.success = false;
            result.errmsg = "wrong number of arguments for 'del' command";
            return false;
        }
        if (tokens[1].empty()) {
            result.success = false;
            result.errmsg = "invalid key";
            return false;
        }
        result.key = tokens[1];
        result.success = true;
        return true;
    }
    /**
     * @brief Provides a help message for available commands.
     *
     * This method returns a string containing a list of available commands
     * and their descriptions.
     *
     * @return A string representing the help message.
     */
    std::string
    help() {
        return "Available commands:\n"
               " SET <key> <value> - Set key to hold the string value\n"
               " GET <key>         - Get the value of key\n"
               " DEL <key>         - Delete a key\n"
               " help              - Show this help menu\n"
               " exit              - Exit the program\n"
               " clear             - Clear the screen\n";
    }

public:
    /**
     * @brief Database engine instance.
     *
     * This instance is used to interact with the underlying database engine
     * for executing commands.
     */
    LSMTree db;

    /**
     * @brief Default constructor for CommandParser.
     *
     * This constructor initializes the CommandParser instance.
     */
    CommandParser() = default;

    /**
     * @brief Parses and executes a command.
     *
     * This method takes a command string, splits it into tokens,
     * validates the command, and executes it using the database engine.
     *
     * @param line The input command string.
     * @return A string representing the result of the command execution.
     */
    std::string parser(const std::string &line) {
        try {
            std::vector<std::string> tokens = split(line);
            if (tokens.empty()) {
                return "";
            }

            ValidationResult result;
            std::string command = tokens[0];
            std::transform(command.begin(), command.end(), command.begin(), ::tolower);

            if (command == "set") {
                if (!validate_set(tokens, result)) {
                    return RespEncoder::error(result.errmsg);
                }
                db.put(result.key, result.value);
                return RespEncoder::simpleString("OK");
            } else if (command == "get") {
                if (!validate_get(tokens, result)) {
                    return RespEncoder::error(result.errmsg);
                }
                std::pair<bool, std::string> found_value = db.get(result.key);
                if (!found_value.first) {
                    return "NULL\n";
                }
                return found_value.second + "\n";
            } else if (command == "del") {
                if (!validate_del(tokens, result)) {
                    return RespEncoder::error(result.errmsg);
                }
                std::pair<bool, std::string> found_value = db.get(result.key);
                if (!found_value.first) {
                    return "key \"" + result.key + "\" not found\n";
                }
                db.remove(result.key);
                return RespEncoder::integer(1);
            } else if (command == "help") {
                return help();
            } else if (command == "exit") {
                exit(0);
            } else if (command == "clear") {
                return "\033[2J\033[1;1H";
            } else {
                return RespEncoder::error("unknown command '" + command + "'");
            }
        } catch (const std::exception &e) {
            return RespEncoder::error(e.what());
        }
        return "";
    }
};

#endif
