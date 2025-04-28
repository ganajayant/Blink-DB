/**
 * @file resp_decoder.hpp
 * @brief Redis Serialization Protocol (RESP) Decoder
 * @details This file contains the implementation of a RESP decoder thats
 *          responsible for parsing RESP commands and extracting the operation,
 *          key, and value from the input buffer. It also handles error cases
 *          and provides a structured response object.
 * @author Gana Jayant Sigadam
 * @version 1.0
 * @date March 2025
 */
#ifndef RESP_DECODER
#define RESP_DECODER

#include <string>

#define CRLF "\r\n"

/**
 * @brief Enum for RESP operations
 * @details This enum defines the possible RESP operations that can be parsed
 *          from the input buffer. It includes SET, GET, DEL, and UNKNOWN.
 */
enum Operation {
    SET,
    GET,
    DEL,
    UNKNOWN
};

/**
 * @brief Response object for RESP commands
 * @details This class encapsulates the parsed information from a RESP command,
 *          including the operation type, key, value, success status, and error message.
 *          It is used to represent the result of decoding a RESP command.
 */
class Resp {
public:
    Operation operation;
    std::string key;
    std::string value;
    bool success;
    std::string error;
    Resp() : operation(UNKNOWN), success(false) {}
};

/**
 * @brief RESP Decoder class
 * @details This class is responsible for decoding RESP commands from a buffer.
 *          It parses the input buffer to extract the operation, key, and value,
 *          and handles error cases. The decode method returns a Resp object
 *          containing the parsed information.
 */
class RespDecoder {
public:
    /**
     * @brief Decode a RESP command from a buffer
     * @param buffer The input buffer containing the RESP command
     * @param length The length of the input buffer
     * @return Resp object containing the parsed information
     */
    static Resp decode(const char *buffer, size_t length) {
        Resp resp;

        std::string_view input(buffer, length);

        if (input.empty() || input[0] != '*') {
            resp.error = "Invalid request: missing array marker";
            return resp;
        }

        size_t pos = input.find(CRLF);
        if (pos == std::string_view::npos) {
            resp.error = "Invalid request: malformed array header";
            return resp;
        }

        int num_args;
        try {
            num_args = std::stoi(std::string(input.substr(1, pos - 1)));
        } catch (std::exception &) {
            resp.error = "Invalid request: invalid argument count";
            return resp;
        }

        if (num_args < 2 || num_args > 3) {
            resp.error = "Invalid request: unexpected argument count";
            return resp;
        }

        input.remove_prefix(pos + 2);
        if (!parseOperation(input, resp)) {
            return resp;
        }

        if (!parseKey(input, resp)) {
            return resp;
        }

        if (resp.operation == SET) {
            if (num_args != 3) {
                resp.error = "Invalid request: SET requires a value";
                return resp;
            }

            if (!parseValue(input, resp)) {
                return resp;
            }
        } else if (num_args > 2) {
            resp.error = "Invalid request: too many arguments";
            return resp;
        }

        if (!input.empty() && input != CRLF) {
            resp.error = "Invalid request: extra data after command";
            return resp;
        }

        resp.success = true;
        return resp;
    }

    /**
     * @brief Decode a RESP command from a null-terminated string
     * @param buffer The input buffer containing the RESP command
     * @return Resp object containing the parsed information
     */
    static Resp decode(const char *buffer) {
        return decode(buffer, std::strlen(buffer));
    }

private:
    /**
     * @brief Parse the operation from the input buffer
     * @param input The input buffer containing the RESP command
     * @param resp The Resp object to store the parsed information
     * @return true if parsing is successful, false otherwise
     */
    static bool parseOperation(std::string_view &input, Resp &resp) {
        if (input.empty() || input[0] != '$') {
            resp.error = "Invalid request: missing operation string marker";
            return false;
        }

        size_t pos = input.find(CRLF);
        if (pos == std::string_view::npos) {
            resp.error = "Invalid request: malformed operation length";
            return false;
        }

        int op_len;
        try {
            op_len = std::stoi(std::string(input.substr(1, pos - 1)));
        } catch (std::exception &) {
            resp.error = "Invalid request: invalid operation length";
            return false;
        }

        input.remove_prefix(pos + 2);
        if (input.size() < static_cast<size_t>(op_len) + 2) {
            resp.error = "Invalid request: truncated operation";
            return false;
        }

        std::string_view op = input.substr(0, op_len);

        if (op == "DEL") {
            resp.operation = DEL;
        } else if (op == "GET") {
            resp.operation = GET;
        } else if (op == "SET") {
            resp.operation = SET;
        } else {
            resp.error = "Invalid request: unknown operation";
            return false;
        }

        input.remove_prefix(op_len + 2);
        return true;
    }

    /**
     * @brief Parse the key from the input buffer
     * @param input The input buffer containing the RESP command
     * @param resp The Resp object to store the parsed information
     * @return true if parsing is successful, false otherwise
     */
    static bool parseKey(std::string_view &input, Resp &resp) {
        if (input.empty() || input[0] != '$') {
            resp.error = "Invalid request: missing key string marker";
            return false;
        }

        size_t pos = input.find(CRLF);
        if (pos == std::string_view::npos) {
            resp.error = "Invalid request: malformed key length";
            return false;
        }

        int key_len;
        try {
            key_len = std::stoi(std::string(input.substr(1, pos - 1)));
        } catch (std::exception &) {
            resp.error = "Invalid request: invalid key length";
            return false;
        }

        input.remove_prefix(pos + 2);

        if (input.size() < static_cast<size_t>(key_len) + 2) {
            resp.error = "Invalid request: truncated key";
            return false;
        }

        resp.key = std::string(input.substr(0, key_len));
        input.remove_prefix(key_len + 2);
        return true;
    }

    /**
     * @brief Parse the value from the input buffer
     * @param input The input buffer containing the RESP command
     * @param resp The Resp object to store the parsed information
     * @return true if parsing is successful, false otherwise
     */
    static bool parseValue(std::string_view &input, Resp &resp) {
        if (input.empty() || input[0] != '$') {
            resp.error = "Invalid request: missing value string marker";
            return false;
        }

        size_t pos = input.find(CRLF);
        if (pos == std::string_view::npos) {
            resp.error = "Invalid request: malformed value length";
            return false;
        }

        int value_len;
        try {
            value_len = std::stoi(std::string(input.substr(1, pos - 1)));
        } catch (std::exception &) {
            resp.error = "Invalid request: invalid value length";
            return false;
        }

        input.remove_prefix(pos + 2);

        if (input.size() < static_cast<size_t>(value_len) + 2) {
            resp.error = "Invalid request: truncated value";
            return false;
        }

        resp.value = std::string(input.substr(0, value_len));
        input.remove_prefix(value_len + 2);
        return true;
    }
};
#endif