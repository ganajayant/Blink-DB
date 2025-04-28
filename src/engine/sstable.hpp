/**
 * @file sstable.hpp
 * @brief SSTable implementation
 * @author Gana Jayant Sigadam
 * @version 1.0
 * @date March 2025
 */
#ifndef SS_TABLE
#define SS_TABLE

#include "constants.hpp"
#include "memtable.hpp"
#include <cstddef>
#include <fstream>
#include <string>

/**
 * @class SS_Table
 * @brief Represents an SSTable (Sorted String Table) used in the LSM Tree.
 *
 * This class handles persistent storage of key-value pairs by writing data in a structured format.
 * It provides functions for loading indexes, retrieving values, and managing SSTable files.
 */
class SS_Table {
private:
    std::string index_filename;                          ///< Filename for the index file.
    std::string data_filename;                           ///< Filename for the data file.
    std::vector<std::pair<std::string, uint64_t>> index; ///< In-memory index mapping keys to file offsets.
    bool indexLoaded = false;                            ///< Flag indicating if the index is loaded.

    static constexpr size_t KEYS_PER_INDEX_ENTRY = 10; ///< Number of keys per index entry.

    /**
     * @brief Finds the start offset for a given key in the data file using binary search.
     * @param key The key to search for.
     * @return Optional offset if the key is found, otherwise std::nullopt.
     */
    std::optional<uint64_t> findStartOffset(const std::string &key) {
        if (index.empty()) {
            return std::nullopt;
        }

        if (key < index[0].first) {
            return index[0].second;
        }

        size_t left = 0, right = index.size() - 1;
        while (left < right) {
            size_t mid = left + (right - left + 1) / 2;
            if (index[mid].first <= key) {
                left = mid;
            } else {
                right = mid - 1;
            }
        }

        return index[left].second;
    }

public:
    /**
     * @brief Constructs an SS_Table from a given filename (excluding extensions).
     * @param filename Base name for the SSTable (without extensions).
     */
    SS_Table(const std::string &filename) : index_filename(filename + INDEX_EXTENSION), data_filename(filename + DATA_EXTENSION) {
        indexLoaded = loadIndex();
    }

    /**
     * @brief Constructs an SS_Table from existing index and data filenames.
     * @param index_filename Path to the index file.
     * @param data_filename Path to the data file.
     */
    SS_Table(const std::string &index_filename, const std::string &data_filename) : index_filename(index_filename), data_filename(data_filename) {
        indexLoaded = loadIndex();
    }

    /**
     * @brief Creates an SSTable from a given MemTable.
     * @param filename Base filename for the new SSTable (without extensions).
     * @param memTable Pointer to the MemTable containing data.
     * @return True if creation is successful, otherwise false.
     */
    static bool createFromMemTable(const std::string &filename, MemTable *memTable) {
        /*
            data file format: [Key Size] [Key] [Value Size] [Value]
            [Key Size] [Key] [Value Size] [Value]
            [4 bytes]  [N bytes] [4 bytes] [M bytes]
            index file format: [Key Size] [Key] [Offset]
        */
        std::ofstream indexFile(filename + INDEX_EXTENSION, std::ios::binary);
        std::ofstream dataFile(filename + DATA_EXTENSION, std::ios::binary);
        if (!indexFile.is_open() || !dataFile.is_open()) {
            return false;
        }

        std::vector<KeyValuePair> entries;
        for (auto it = memTable->begin(); it != memTable->end(); ++it) {
            entries.push_back(*it);
        }

        uint64_t entries_count = entries.size();
        uint64_t sparse_index_count = (entries_count + KEYS_PER_INDEX_ENTRY - 1) / KEYS_PER_INDEX_ENTRY;
        indexFile.write(reinterpret_cast<const char *>(&sparse_index_count), sizeof(sparse_index_count));
        uint64_t offset = 0;

        for (size_t i = 0; i < entries.size(); i++) {
            const KeyValuePair &entry = entries[i];
            uint32_t key_size = entry.getKey().size();
            uint32_t value_size = entry.getValue().size();

            if (i % KEYS_PER_INDEX_ENTRY == 0) {
                indexFile.write(reinterpret_cast<const char *>(&key_size), sizeof(key_size));
                indexFile.write(entry.getKey().c_str(), key_size);
                indexFile.write(reinterpret_cast<const char *>(&offset), sizeof(offset));
            }

            dataFile.write(reinterpret_cast<const char *>(&key_size), sizeof(key_size));
            dataFile.write(entry.getKey().c_str(), key_size);
            dataFile.write(reinterpret_cast<const char *>(&value_size), sizeof(value_size));
            dataFile.write(entry.getValue().c_str(), value_size);

            offset += sizeof(key_size) + key_size + sizeof(value_size) + value_size;
        }

        indexFile.close();
        dataFile.close();
        return true;
    }

    /**
     * @brief Loads the SSTable index from the index file.
     * @return True if the index is successfully loaded, otherwise false.
     */
    bool loadIndex() {
        std::ifstream indexFile(index_filename, std::ios::binary);
        if (!indexFile.is_open()) {
            return false;
        }

        index.clear();

        uint64_t entries_count;
        indexFile.read(reinterpret_cast<char *>(&entries_count), sizeof(entries_count));

        index.reserve(entries_count);
        for (uint64_t i = 0; i < entries_count; i++) {
            uint32_t key_size;
            indexFile.read(reinterpret_cast<char *>(&key_size), sizeof(key_size));
            std::string key(key_size, '\0');
            indexFile.read(&key[0], key_size);
            uint64_t offset;
            indexFile.read(reinterpret_cast<char *>(&offset), sizeof(offset));
            index.push_back({std::move(key), offset});
        }
        indexFile.close();
        return true;
    }

    /**
     * @brief Retrieves the value associated with a key from the SSTable.
     * @param key The key to look up.
     * @return A pair containing a boolean (indicating success) and the value.
     */
    std::pair<bool, std::string> getValue(std::string key) {
        if (!indexLoaded) {
            return {false, ""};
        }
        std::optional<uint64_t> start_offset = findStartOffset(key);
        if (!start_offset.has_value()) {
            return {false, ""};
        }

        std::ifstream dataFile(data_filename, std::ios::binary);
        if (!dataFile.is_open()) {
            return {false, ""};
        }
        dataFile.seekg(start_offset.value());

        while (!dataFile.eof()) {
            uint32_t key_size;
            dataFile.read(reinterpret_cast<char *>(&key_size), sizeof(key_size));
            if (dataFile.eof()) {
                break;
            }

            std::string stored_key(key_size, 0);
            dataFile.read(&stored_key[0], key_size);
            if (stored_key > key) {
                return {false, ""};
            }

            uint32_t value_size;
            dataFile.read(reinterpret_cast<char *>(&value_size), sizeof(value_size));
            std::string value(value_size, 0);
            dataFile.read(&value[0], value_size);

            if (stored_key == key) {
                dataFile.close();
                if (value == TOMBSTONE) {
                    return {false, value};
                }
                return {true, value};
            }
        }
        dataFile.close();
        return {false, ""};
    }

    /**
     * @brief Gets the filename of the SSTable's index file.
     * @return The index file's name.
     */
    std::string getIndexFile() const {
        return index_filename;
    }

    /**
     * @brief Gets the filename of the SSTable's data file.
     * @return The data file's name.
     */
    std::string getDataFilename() const {
        return data_filename;
    }
};

#endif