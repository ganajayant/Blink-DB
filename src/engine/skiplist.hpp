/**
 * @file skiplist.hpp
 * @brief Skip List Implementation
 * @details This file contains the implementation of a skip list data structure
 *          for efficient key-value storage and retrieval. The skip list is used as a Memtable (In-Memory) for the LSM-Tree
 * @author Gana Jayant Sigadam
 * @date March 2025
 * @version 1.0
 */
#ifndef SKIP_LIST
#define SKIP_LIST

#include "key_value.hpp"
#include <random>

/**
 * @brief To represent the type of node in the skip list.
 *  @details This enum class defines the types of nodes in the skip list.
 *           It includes three types: NEGATIVE_INFINITY, NORMAL, and POSITIVE_INFINITY.
 *           NEGATIVE_INFINITY is used to represent the head of the skip list,
 *           NORMAL is used for regular nodes, and POSITIVE_INFINITY is used to represent the tail of the skip list.
 */
enum class SentinelType {
    NEGATIVE_INFINITY,
    NORMAL,
    POSITIVE_INFINITY,
};

/**
 * @class Node
 * @brief Skip List Node Class
 * @details This class represents a node in the skip list.
 *          Each node contains a key-value pair, pointers to the next and previous nodes,
 *          and pointers to the upper and lower levels of the skip list.
 *          The node type is represented by the SentinelType enum.
 */
class Node {
public:
    SentinelType type;
    KeyValuePair data;
    Node *prev;
    Node *next;
    Node *up;
    Node *down;

    Node(SentinelType type, const KeyValuePair &data)
        : type(type), data(data), prev(nullptr), next(nullptr), up(nullptr), down(nullptr) {}
};

/**
 * @class SkipList
 * @brief Skip List Class
 * @details This class implements a skip list data structure for efficient key-value storage and retrieval.
 */
class SkipList {
private:
    static constexpr int MAX_LEVEL = 16;
    static constexpr double P = 0.5;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    Node *head;
    Node *tail;
    int current_level;
    size_t totalSize;

public:
    SkipList() : gen(std::random_device()()), dis(0, 1), current_level(0), totalSize(0) {
        head = new Node(SentinelType::NEGATIVE_INFINITY, KeyValuePair());
        tail = new Node(SentinelType::POSITIVE_INFINITY, KeyValuePair());
        head->next = tail;
        tail->prev = head;
    }

    /**
     * @brief Search for a key in the skip list.
     * @details This method traverses the skip list to find the node with the specified key.
     *          It starts from the head and moves down to the lower levels until it finds the key or reaches the end.
     *          The search is performed in a forward direction, skipping nodes that are not relevant. if the key is not found,
     *          it returns the node where the key should be inserted.
     * @param key The key to search for.
     * @return Node* A pointer to the node containing the key or the node where the key should be inserted.
     */
    Node *search(const std::string &key) {
        Node *cur = head;
        while (true) {
            while (cur->next->type != SentinelType::POSITIVE_INFINITY &&
                   cur->next->data.getKey() <= key) {
                cur = cur->next;
            }
            if (cur->down != nullptr) {
                cur = cur->down;
            } else {
                break;
            }
        }
        return cur;
    }

    /**
     * @brief This method retrieves the value associated with a given key in the skip list.
     *
     * @param key The key to search for.
     * @return std::pair<bool, std::string> A pair containing a boolean indicating whether the key was found
     *         and the corresponding value if found, or an empty string if not found.
     */
    std::pair<bool, std::string> get(const std::string &key) {
        Node *cur = search(key);
        if (cur->data.getKey() == key) {
            return {true, cur->data.getValue()};
        }
        return {false, ""};
    }

    /**
     * @brief This method inserts a key-value pair into the skip list.
     * @details As the skip-list is a probabilistic data structure, the insertion
     *          may cause the skip-list to grow in height. The method handles this
     *          by creating new levels as needed. The insertion is done in a way that
     *          maintains the skip-list properties.
     * @param key The key to insert.
     * @param value The value to associate with the key.
     */
    void put(const std::string &key, const std::string &value) {
        Node *current = search(key);
        if (current->data.getKey() == key) {
            current->data.setValue(value);
            return;
        }
        KeyValuePair kv(key, value);
        totalSize += kv.size();
        Node *newNode = new Node(SentinelType::NORMAL, kv);
        newNode->prev = current;
        newNode->next = current->next;
        current->next->prev = newNode;
        current->next = newNode;

        int i = 0;
        while (i < MAX_LEVEL && dis(gen) < P) {
            if (i >= current_level) {
                current_level++;
                Node *newHead = new Node(SentinelType::NEGATIVE_INFINITY, KeyValuePair());
                Node *newTail = new Node(SentinelType::POSITIVE_INFINITY, KeyValuePair());
                newHead->next = newTail;
                newTail->prev = newHead;
                newHead->down = head;
                newTail->down = tail;
                head->up = newHead;
                tail->up = newTail;
                head = newHead;
                tail = newTail;
            }
            while (current->up == nullptr) {
                current = current->prev;
            }
            current = current->up;
            Node *newNodeUp = new Node(SentinelType::NORMAL, KeyValuePair(key, ""));
            newNodeUp->prev = current;
            newNodeUp->next = current->next;
            current->next->prev = newNodeUp;
            current->next = newNodeUp;
            newNode->up = newNodeUp;
            newNodeUp->down = newNode;
            newNode = newNodeUp;
            i++;
        }
    }
    /**
     * @brief Get the size of the skip list.
     *
     * @return size_t The total size of the skip list in bytes.
     */
    size_t getSize() {
        return totalSize;
    }

    /**
     * @class Iterator
     * @brief A Iterator class for the skip list.
     * @details This class provides an iterator for traversing the skip list.
     */
    class Iterator {
    private:
        Node *current;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = KeyValuePair;
        using difference_type = std::ptrdiff_t;
        using pointer = KeyValuePair *;
        using reference = KeyValuePair &;

        Iterator(Node *node = nullptr) : current(node) {}

        reference operator*() const {
            return current->data;
        }
        pointer operator->() const {
            return &(current->data);
        }

        Iterator &operator++() {
            if (current) {
                current = current->next;
                if (current && current->type == SentinelType::POSITIVE_INFINITY) {
                    current = nullptr;
                }
            }
            return *this;
        }
        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }
        bool operator==(const Iterator &other) const {
            return current == other.current;
        }
        bool operator!=(const Iterator &other) const {
            return !(*this == other);
        }
    };

    Iterator begin() {
        Node *current = head;
        while (current->down != nullptr) {
            current = current->down;
        }
        current = current->next;

        if (current->type == SentinelType::POSITIVE_INFINITY) {
            return end();
        }

        return Iterator(current);
    }

    Iterator end() {
        return Iterator(nullptr);
    }

    Iterator find(const std::string &key) {
        Node *node = search(key);
        if (node->data.getKey() == key && node->type == SentinelType::NORMAL) {
            return Iterator(node);
        }
        return end();
    }

    ~SkipList() {
        Node *cur = head;
        while (cur != nullptr) {
            Node *next = cur->down;
            Node *tmp = cur;
            while (tmp != nullptr) {
                Node *tmpNext = tmp->next;
                delete tmp;
                tmp = tmpNext;
            }
            cur = next;
        }
    }
};

#endif