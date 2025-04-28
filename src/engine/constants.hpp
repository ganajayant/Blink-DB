/**
 * @file constants.hpp
 * @brief Constants for the database engine.
 * @details This file contains various constants used throughout the database engine,
 *          including file extensions, directory names, and memory limits.
 * @author Gana Jayant Sigadam
 * @date March 2025
 * @version 1.0
 */
#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>

/**
 * @brief TOMOBSTONE constant.
 * @details This constant is used to represent a deleted key in the database.
 *          It is a special value that indicates the key has been marked for deletion.
 *          The value is set to 0xFFFFFFFF, which is a common representation for
 *          a deleted or invalid value in many systems.
 */
#define TOMBSTONE "\xFF\xFF\xFF\xFF"

/**
 * @brief Index Extenstion Constant.
 * @details This constant is used to define the file extension for index files
 *           in the database. It is set to ".index", this index file is used to store sparse indexes of keys
 */
const std::string INDEX_EXTENSION = ".index";

/**
 * @brief Data Extension Constant.
 * @details This constant is used to define the file extension for data files
 *           in the database. It is set to ".data", this data file is used to store the actual key-value pairs
 *
 */
const std::string DATA_EXTENSION = ".data";

/**
 * @brief Data Directory Constant.
 * @details This constant is used to define the directory where sstable files are stored
 */
const std::string DATA_DIR = "data/";

/**
 * @brief Constants for memory limit in bytes. (Default: 32MB)
 * @details If the memtable size exceeds this limit, the memtable will be flushed to disk.
 */
const size_t MAX_MEMTABLE_SIZE = 32 * 1024 * 1024;

/**
 * @brief Constants for the maximum number of sstables.
 * @details This constant is used as threshold for sstable count after which compaction is triggered.
 *          The default value is set to 100, which means that if the number of sstable files exceeds this limit,
 *          a compaction process will be initiated to merge and reduce the number of sstable files.
 */
const size_t MAX_SSTABLE_COUNT = 100;
#endif