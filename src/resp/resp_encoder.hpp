/**
 * @file resp_encoder.hpp
 * @author Gana Jayant Sigadam
 * @brief Used to encode messages into RESP (REdis Serialization Protocol) format.
 * @details This class provides static methods to encode different types of RESP
 * messages, including simple strings, errors, integers, bulk strings, and null
 * responses. Each method returns a string formatted according to the RESP protocol.
 * The RESP protocol is used by Redis for communication between the client and server.
 * @version 1.0
 * @date March 2025
 * @copyright Copyright (c) 2025
 *
 */
#ifndef RESP_ENCODER_HPP
#define RESP_ENCODER_HPP

#include <string>

class RespEncoder {
public:
    /**
     * @brief Converts a simple string to RESP format.
     *
     * @param str the string to be sent
     * @return std::string respresentation of the string
     * @details The string is prefixed with a '+' character and suffixed with
     * a CRLF sequence. This is the standard format for simple strings in RESP.
     * For example, the string "Hello" would be converted to "+Hello\r\n".
     */
    static std::string simpleString(const std::string &str) {
        return "+" + str + "\r\n";
    }

    /**
     * @brief Converts an error message to RESP format.
     *
     * @param message the message to be sent
     * @return std::string respresentation of the error message
     * @details The message is prefixed with a '-' character and suffixed with
     * a CRLF sequence. This is the standard format for error messages in RESP.
     * For example, the message "Error occurred" would be converted to
     * "-Error occurred\r\n".
     */
    static std::string error(const std::string &message) {
        return "-ERR " + message + "\r\n";
    }

    /**
     * @brief Used to encode an integer to RESP format.
     *
     * @param value the integer to be sent
     * @return std::string respresentation of the integer
     * @details The integer is prefixed with a ':' character and suffixed with
     * a CRLF sequence. This is the standard format for integers in RESP.
     * For example, the integer 42 would be converted to ":42\r\n".
     */
    static std::string integer(int value) {
        return ":" + std::to_string(value) + "\r\n";
    }

    /**
     * @brief Converts a bulk string to RESP format.
     * @param str the string to be sent
     * @param isNull true if the string is null
     * @return std::string
     * @details The string is prefixed with a '$' character, followed by the
     * length of the string, and suffixed with a CRLF sequence. If the string
     * is null, it is represented as "$-1\r\n". If the string is empty, it is
     * represented as "$0\r\n\r\n". This is the standard format for bulk strings
     * in RESP. For example, the string "Hello" would be converted to
     * "$5\r\nHello\r\n", and a null string would be converted to "$-1\r\n".
     * @note The function handles both null and empty strings appropriately.
     * @note The function uses CRLF (Carriage Return Line Feed) as the line
     */
    static std::string bulkString(const std::string &str, const bool isNull) {
        if (isNull) {
            return "$-1\r\n";
        }
        if (str.empty()) {
            return "$0\r\n\r\n";
        }
        return "$" + std::to_string(str.length()) + "\r\n" + str + "\r\n";
    }
};

#endif
