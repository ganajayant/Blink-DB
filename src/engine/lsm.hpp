/**
 * @file lsm.hpp
 * @brief LSM Tree implementation
 * @details This file contains the implementation of the Log-Structured Merge Tree (LSM Tree) for efficient key-value storage.
 * @author Gana Jayant Sigadam
 * @version 1.0
 * @date March 2025
 */
#ifndef LSM
#define LSM

#include "constants.hpp"
#include "memtable.hpp"
#include "sstable.hpp"

#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

/**
 * @class LSMTree
 * @brief LSM Tree implementation
 *  @details This class implements a Log-Structured Merge Tree (LSM Tree) for efficient key-value storage.
 *  @details The LSM Tree uses a combination of in-memory and on-disk data structures to provide fast
 *  @details read and write operations. The in-memory structure is a MemTable, which is periodically flushed
 *  @details to disk as an SSTable. The on-disk structure is a collection of SSTables, which are compacted
 *  @details periodically to reduce the number of files and improve read performance.
 */

class LSMTree {
private:
    std::string SS_TABLE_PATH;                       /**< Path to the directory storing SSTables. */
    std::unique_ptr<MemTable> activeMemTable;        /**< The active MemTable for write operations. */
    std::deque<std::unique_ptr<MemTable>> memTables; /**< Immutable MemTables awaiting flush to disk. */
    std::deque<std::unique_ptr<SS_Table>> sstables;  /**< Collection of SSTables on disk. */

    std::mutex active_memtable_mtx; /**< Mutex for synchronizing access to the active MemTable. */
    std::mutex memtables_mtx;       /**< Mutex for synchronizing access to the memTables queue. */
    std::mutex sstables_mtx;        /**< Mutex for synchronizing access to the SSTables. */
    std::mutex compaction_mtx;      /**< Mutex for synchronizing the compaction process. */

    std::condition_variable cv;            /**< Condition variable for MemTable flushing. */
    std::condition_variable compaction_cv; /**< Condition variable for compaction events. */

    std::atomic<bool> running;     /**< Flag to indicate whether the LSMTree service is running. */
    std::thread flush_thread;      /**< Background thread for flushing MemTables to SSTables. */
    std::thread compaction_thread; /**< Background thread for periodic SSTable compaction. */

    /**
     * @brief Rotates the active MemTable when it reaches its maximum size.
     * @details The current MemTable becomes immutable and is added to the flush queue.
     *          A new active MemTable is created for future writes.
     */
    void rotate_memtable() {
        std::unique_ptr<MemTable> new_memtable = std::make_unique<MemTable>();
        memTables.push_back(std::move(activeMemTable));
        activeMemTable = std::move(new_memtable);
        cv.notify_one();
    }

    /**
     * @brief Background worker thread that flushes MemTables to SSTables.
     * @details The worker waits until there is a MemTable to flush, then processes it.
     */
    void flush_worker() {
        while (running) {
            std::unique_ptr<MemTable> memtable_to_flush;
            {
                std::unique_lock<std::mutex> lock(memtables_mtx);
                cv.wait(lock, [this] { return !running || !memTables.empty(); });

                if (!running && memTables.empty()) {
                    break;
                }

                if (!memTables.empty()) {
                    memtable_to_flush = std::move(memTables.front());
                    memTables.erase(memTables.begin());
                }
            }

            if (memtable_to_flush) {
                flush_memtable(std::move(memtable_to_flush));
            }
        }
    }

    /**
     * @brief Writes an immutable MemTable to disk as an SSTable.
     * @param memtable The MemTable to flush to disk.
     */
    void flush_memtable(std::unique_ptr<MemTable> memtable) {
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count();
        std::string filename = SS_TABLE_PATH + "sstable_" + std::to_string(timestamp);
        bool success = SS_Table::createFromMemTable(filename, memtable.get());
        if (success) {
            std::lock_guard<std::mutex> lock(sstables_mtx);
            std::unique_ptr<SS_Table> sstable = std::make_unique<SS_Table>(filename);
            sstables.push_back(std::move(sstable));

            if (sstables.size() >= MAX_SSTABLE_COUNT) {
                compaction_cv.notify_one();
            }
        }
    }

    /**
     * @brief Background worker thread for periodic SSTable compaction.
     * @details Monitors the number of SSTables and triggers compaction when necessary.
     */
    void compaction_worker() {
        while (running) {
            {
                std::unique_lock<std::mutex> lock(compaction_mtx);
                compaction_cv.wait(lock, [this] {
                    std::lock_guard<std::mutex> sstables_lock(sstables_mtx);
                    return !running || sstables.size() >= MAX_SSTABLE_COUNT;
                });

                if (!running) {
                    break;
                }
                perform_compaction();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }

    /**
     * @brief Performs SSTable compaction to optimize storage and query performance.
     * @details Merges multiple SSTables, removes deleted keys, and writes a new compacted SSTable.
     */
    void perform_compaction() {
        std::vector<std::unique_ptr<SS_Table>> tables_to_compact;
        {
            std::lock_guard<std::mutex> lock(sstables_mtx);

            if (sstables.size() < MAX_SSTABLE_COUNT) {
                return;
            }

            size_t tables_to_take = MAX_SSTABLE_COUNT;

            for (size_t i = 0; i < tables_to_take && !sstables.empty(); ++i) {
                tables_to_compact.push_back(std::move(sstables.front()));
                sstables.erase(sstables.begin());
            }
        }

        if (tables_to_compact.empty()) {
            return;
        }

        std::sort(tables_to_compact.begin(), tables_to_compact.end(),
                  [](const std::unique_ptr<SS_Table> &a, const std::unique_ptr<SS_Table> &b) {
                      return a->getIndexFile() < b->getIndexFile();
                  });
        std::unique_ptr<MemTable> merged_memtable = std::make_unique<MemTable>();

        std::map<std::string, std::string> merged_data;
        for (std::unique_ptr<SS_Table> &sstable : tables_to_compact) {
            std::ifstream data_file(sstable->getDataFilename(), std::ios::binary);
            if (!data_file.is_open()) {
                continue;
            }
            while (!data_file.eof()) {
                uint32_t key_size;
                data_file.read(reinterpret_cast<char *>(&key_size), sizeof(key_size));
                if (data_file.eof()) {
                    break;
                }

                std::string key(key_size, '\0');
                data_file.read(&key[0], key_size);

                uint32_t value_size;
                data_file.read(reinterpret_cast<char *>(&value_size), sizeof(value_size));

                std::string value(value_size, '\0');
                data_file.read(&value[0], value_size);
                if (merged_data.find(key) == merged_data.end()) {
                    merged_data[key] = value;
                }
            }
        }

        for (std::pair<std::string, std::string> x : merged_data) {
            if (x.second != TOMBSTONE) {
                merged_memtable->put(x.first, x.second);
            }
        }

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now.time_since_epoch())
                                  .count();
        std::string filename = SS_TABLE_PATH + "sstable_" + std::to_string(timestamp);
        bool success = SS_Table::createFromMemTable(filename, merged_memtable.get());

        if (success) {
            for (auto &sstable : tables_to_compact) {
                std::string index_file = sstable->getIndexFile();
                std::string data_file = sstable->getDataFilename();

                sstable.reset();

                std::filesystem::remove(index_file);
                std::filesystem::remove(data_file);
            }

            std::lock_guard<std::mutex> lock(sstables_mtx);
            std::unique_ptr<SS_Table> new_sstable = std::make_unique<SS_Table>(filename);
            sstables.push_back(std::move(new_sstable));
        }
    }

    /**
     * @brief Loads existing SSTables from disk at startup.
     * @details Reads the directory containing SSTables, reconstructs indexes, and prepares the system.
     */
    void load_existing_sstables() {
        std::vector<std::string> files;
        for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(SS_TABLE_PATH)) {
            files.push_back(entry.path().string());
        }
        for (std::string const &file : files) {
            if (file.find(INDEX_EXTENSION) != std::string::npos) {
                std::string data_file = file.substr(0, file.find(INDEX_EXTENSION));
                std::unique_ptr<SS_Table> sstable = std::make_unique<SS_Table>(file, data_file + DATA_EXTENSION);
                if (sstable->loadIndex()) {
                    sstables.push_back(std::move(sstable));
                }
            }
        }
        // sort sstables by filename based on the creation time the filenames are in the format data/sstable_timestamp.index and data/sstable_timestamp.data
        std::sort(sstables.begin(), sstables.end(), [](const std::unique_ptr<SS_Table> &a, const std::unique_ptr<SS_Table> &b) {
            return a->getIndexFile() < b->getIndexFile();
        });
        if (sstables.size() >= MAX_SSTABLE_COUNT) {
            compaction_cv.notify_one();
        }
    }

public:
    /**
     * @brief Constructs an LSMTree instance and initializes background worker threads.
     */
    LSMTree()
        : SS_TABLE_PATH(DATA_DIR),
          running(true) {
        std::filesystem::create_directories(SS_TABLE_PATH);

        flush_thread = std::thread(&LSMTree::flush_worker, this);
        compaction_thread = std::thread(&LSMTree::compaction_worker, this);
        activeMemTable = std::make_unique<MemTable>();

        load_existing_sstables();
    }

    /**
     * @brief Destructor that gracefully shuts down the LSMTree, ensuring all background tasks complete.
     */
    ~LSMTree() {
        running = false;
        cv.notify_all();
        compaction_cv.notify_all();

        if (flush_thread.joinable()) {
            flush_thread.join();
        }
        if (compaction_thread.joinable()) {
            compaction_thread.join();
        }
    }

    /**
     * @brief Inserts a key-value pair into the LSM Tree.
     * @param key The key to insert.
     * @param value The associated value.
     */
    void put(const std::string &key, const std::string &value) {
        {
            std::lock_guard<std::mutex> lock(active_memtable_mtx);
            activeMemTable->put(key, value);
            if (activeMemTable->getSize() >= MAX_MEMTABLE_SIZE) {
                rotate_memtable();
            }
        }
    }

    /**
     * @brief Retrieves the value associated with a given key.
     * @param key The key to search for.
     * @return A pair containing a boolean indicating success and the associated value.
     */
    std::pair<bool, std::string> get(const std::string &key) {
        {
            std::lock_guard<std::mutex> lock(active_memtable_mtx);
            std::pair<bool, std::string> result = activeMemTable->get(key);
            if (result.first) {
                return result;
            } else if (!result.first && result.second == TOMBSTONE) {
                return {false, result.second};
            }
        }

        {
            std::lock_guard<std::mutex> lock(memtables_mtx);
            for (std::reverse_iterator it = memTables.rbegin(); it != memTables.rend(); ++it) {
                std::pair<bool, std::string> result = (*it)->get(key);
                if (result.first) {
                    return result;
                } else if (!result.first && result.second == TOMBSTONE) {
                    return {false, result.second};
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(sstables_mtx);
            for (std::reverse_iterator it = sstables.rbegin(); it != sstables.rend(); ++it) {
                std::pair<bool, std::string> result = (*it)->getValue(key);
                if (result.first) {
                    return result;
                } else if (!result.first && result.second == TOMBSTONE) {
                    return {false, result.second};
                }
            }
        }
        return {false, ""};
    }

    /**
     * @brief Marks a key as deleted by inserting a tombstone value.
     * @param key The key to remove.
     */
    void remove(const std::string &key) {
        std::lock_guard<std::mutex> lock(active_memtable_mtx);
        activeMemTable->put(key, TOMBSTONE);
    }
};

#endif