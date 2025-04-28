/**
 * @file key_value.hpp
 * @author Gana Jayant Sigadam
 * @brief Key-Value Pair Class
 * @details This class represents a key-value pair with methods to set and get
 * @version 1.0
 * @date March 2025
 */
#ifndef KEY_VALUE_HPP
#define KEY_VALUE_HPP

#include <string>

/**
 * @brief  Key-Value Pair Class
 * @details This class represents a key-value pair with methods to set and get
 *
 */
class KeyValuePair {
private:
    std::string key;
    std::string value;

public:
    /**
     * @brief Construct a new Key Value Pair object
     *
     * @param key key string
     * @param value value string
     */
    KeyValuePair(const std::string &key, const std::string &value)
        : key(key), value(value) {}

    KeyValuePair(const std::string &keyBytes)
        : key(keyBytes) {}
    KeyValuePair()
        : key(""), value("") {}

    /**
     * @brief Set the Value object
     *
     * @param newValue
     */
    void setValue(const std::string &newValue) { value = newValue; }

    /**
     * @brief Get the Key object
     *
     * @return const std::string& key string
     */
    const std::string &getKey() const { return key; }

    /**
     * @brief Get the Value object
     *
     * @return const std::string& value string
     */
    const std::string &getValue() const { return value; }

    /**
     * @brief Get the Size of the Key Value Pair object
     * @details The key value pair size is calculated as the sum of the size of
     *          the key and value strings, plus the size of the string object itself.
     * @return size_t size of the key-value pair
     */
    size_t size() const {
        return sizeof(std::string) * 2 +
               key.capacity() +
               value.capacity();
    }
};
#endif