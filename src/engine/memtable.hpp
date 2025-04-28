/**
 * @file memtable.hpp
 * @author Gana Jayant Sigadam
 * @brief MemTable Class
 * @details This class implements a MemTable using a skip list as In-Memory storage for LSM-Tree.
 * @version 1.0
 * @date March 2025
 */
#ifndef MEM_TABLE
#define MEM_TABLE

#include "constants.hpp"
#include "skiplist.hpp"

#include <cstddef>
#include <string>

/**
 * @class MemTable
 * @brief MemTable Class
 *  @details This class implements a MemTable using a skip list as In-Memory storage for LSM-Tree.
 */
class MemTable {
    SkipList *list;

public:
    using Iterator = SkipList::Iterator;

    MemTable() : list(new SkipList()) {
    }

    /**
     * @brief Put a key-value pair into the MemTable.
     *
     * @param key The key to be inserted.
     * @param value The value associated with the key.
     */
    void put(const std::string &key, const std::string &value) {
        list->put(key, value);
    }
    /**
     * @brief Get the value associated with a key.
     *
     * @param key
     * @return std::pair<bool, std::string>
     *         A pair where the first element indicates if the key was found,
     *         and the second element is the value associated with the key.
     *         If the key was not found, the first element will be false and the second element will be an empty string.
     *         If the key was found but marked as TOMBSTONE, the first element will be false and the second element will be TOMBSTONE.
     */
    std::pair<bool, std::string> get(const std::string &key) {
        std::pair<bool, std::string> result = list->get(key);
        if (!result.first) {
            return {false, ""};
        }
        if (result.second == TOMBSTONE) {
            return {false, result.second};
        }
        return {true, result.second};
    }

    /**
     * @brief Remove a key from the MemTable.
     *
     * @param key The key to be removed.
     */
    void remove(const std::string &key) {
        list->put(key, TOMBSTONE);
        return;
    }

    /**
     * @brief Get the size of the MemTable.
     *
     * @return size_t The size of the MemTable in bytes.
     */
    size_t getSize() {
        return list->getSize();
    }

    Iterator begin() {
        return list->begin();
    }

    Iterator end() {
        return list->end();
    }

    Iterator find(const std::string &key) {
        return list->find(key);
    }

    ~MemTable() {
        delete list;
    }
};

#endif