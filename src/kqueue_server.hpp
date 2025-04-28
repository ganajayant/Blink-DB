/**
 * @file kqueue_server.hpp
 * @brief KqueueServer Class
 * @details This class implements a Kqueue-based server that handles client connections
 *          and processes RESP commands. It uses the LSMTree engine for data storage
 *          and retrieval. The server listens for incoming connections, decodes RESP commands,
 *          and sends responses back to the clients.
 * @author Gana Jayant Sigadam
 * @date 2025-03-28
 * @version 1.0
 */
#ifndef KQUEUE_SERVER_HPP
#define KQUEUE_SERVER_HPP
#include "./resp/resp_decoder.hpp"
#include "./resp/resp_encoder.hpp"
#include "engine/lsm.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <sys/event.h>
#include <unistd.h>

/**
 * @brief Client Data
 *  @details This struct holds the data for each client connection,
 *          including the buffer for incoming data and the total number of bytes received.
 */
struct ClientData {
    std::vector<char> buffer;
    size_t total_bytes = 0;
};

/**
 * @class KqueueServer
 * @brief Kqueue-based server class
 * @details This class implements a Kqueue-based server that handles client connections
 *          and processes RESP commands. It uses the LSMTree engine for data storage
 *          and retrieval. The server listens for incoming connections, decodes RESP commands,
 *          and sends responses back to the clients.
 */
class KqueueServer {
private:
    static constexpr size_t INITIAL_BUFFER_SIZE = 4 * 1024;
    static constexpr size_t INITIAL_EVENT_LIST_SIZE = 512;
    const size_t CHUNK_SIZE = 4096;

    int server_socket;
    int kq;
    std::vector<struct kevent> event_list;
    std::unordered_map<int, ClientData> client_buffers;
    LSMTree lsm;

    /**
     * @brief Create a Server Socket object
     * @param addr Address to bind the server socket to
     * @param port Port number to bind the server socket to
     * @return int File descriptor for the server socket
     * @details This function creates a server socket, binds it to the specified address and port,
     *          sets the socket options, and starts listening for incoming connections.
     *          It returns the file descriptor for the server socket.
     */
    int createServerSocket(const std::string &addr, int port) {
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, addr.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << addr << std::endl;
            return -1;
        }

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            std::cerr << "Failed to create socket" << std::endl;
            return -1;
        }

        int opt = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "Socket option setting failed" << std::endl;
            close(sock);
            return -1;
        }

        if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0 ||
            bind(sock, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0 ||
            listen(sock, SOMAXCONN) < 0) {
            std::cerr << "Socket setup failed" << std::endl;
            close(sock);
            return -1;
        }

        if (listen(sock, SOMAXCONN) == -1) {
            std::cerr << "Failed to listen on socket" << std::endl;
            close(sock);
            return -1;
        }
        return sock;
    }

    /**
     * @brief Create a Kqueue object
     * @return int File descriptor for the kqueue
     */
    int createKqueue() {
        int kq = kqueue();
        if (kq == -1) {
            std::cerr << "Failed to create kqueue" << std::endl;
            return -1;
        }
        return kq;
    }

    /**
     * @brief Add a file descriptor to the kqueue
     * @param fd File descriptor to add
     * @param filter Event filter (e.g., EVFILT_READ)
     * @param flags Event flags (e.g., EV_ADD)
     * @return bool True if successful, false otherwise
     */
    bool addToKqueue(int fd, int filter, int flags) {
        struct kevent change_event;
        EV_SET(&change_event, fd, filter, flags, 0, 0, NULL);
        if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1) {
            std::cerr << "Failed to add to kqueue for fd " << fd << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Handle new client connections
     * @details This function accepts new client connections and adds them to the kqueue for monitoring.
     */
    void handleNewConnection() {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        while (true) {
            int client_socket = accept(server_socket, (sockaddr *)&client_addr, &client_len);
            if (client_socket == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    std::cerr << "Connection failed: " << strerror(errno) << std::endl;
                    break;
                }
            }

            if (fcntl(client_socket, F_SETFL, O_NONBLOCK) < 0) {
                std::cerr << "Failed to set non-blocking on client socket" << std::endl;
                close(client_socket);
                continue;
            }

            if (addToKqueue(client_socket, EVFILT_READ, EV_ADD)) {
                client_buffers[client_socket] = ClientData();
                client_buffers[client_socket].buffer.reserve(INITIAL_BUFFER_SIZE);
            } else {
                close(client_socket);
            }
        }
    }

    /**
     * @brief Handle RESP operations
     * @param resp The RESP object containing the operation to perform
     * @param client_fd The file descriptor of the client socket
     * @details This function processes the RESP command and sends the appropriate response back to the client.
     */
    void handle_op(Resp &resp, int client_fd) {
        try {
            std::ostringstream response;
            switch (resp.operation) {
            case GET: {
                std::pair<bool, std::string> result = lsm.get(resp.key);
                response << RespEncoder::bulkString(result.second, !result.first);
                break;
            }
            case SET:
                lsm.put(resp.key, resp.value);
                response << RespEncoder::simpleString("OK");
                break;
            case DEL:
                lsm.remove(resp.key);
                response << RespEncoder::integer(1);
                break;
            default:
                response << RespEncoder::error("Unknown operation");
                break;
            }
            send(client_fd, response.str().c_str(), response.str().size(), 0);
        } catch (const std::exception &e) {
            std::cerr << "Exception in handle_op: " << e.what() << std::endl;
            std::string message = RespEncoder::error("Internal server error");
            send(client_fd, message.c_str(), message.size(), 0);
        }
    }

    /**
     * @brief Handle client messages
     * @param client_fd The file descriptor of the client socket
     * @details This function reads data from the client socket, decodes the RESP command,
     *          and processes it. It handles errors and disconnections appropriately.
     */
    void handleClientMessage(int client_fd) {
        ClientData &client_data = client_buffers[client_fd];

        char temp_buffer[CHUNK_SIZE];

        while (true) {
            ssize_t bytes_read = recv(client_fd, temp_buffer, CHUNK_SIZE, 0);

            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    std::cerr << "recv error for client " << client_fd << ": " << strerror(errno) << std::endl;
                    closeConnection(client_fd);
                    return;
                }
            } else if (bytes_read == 0) {
                closeConnection(client_fd);
                return;
            }

            client_data.buffer.insert(client_data.buffer.end(), temp_buffer, temp_buffer + bytes_read);
            client_data.total_bytes += bytes_read;

            if (client_data.buffer.size() + CHUNK_SIZE > client_data.buffer.capacity()) {
                client_data.buffer.reserve(client_data.buffer.capacity() * 2);
            }

            // size_t current_size = client_data.buffer.size();
            // client_data.buffer.resize(current_size + bytes_read);
            // memcpy(client_data.buffer.data() + current_size, temp_buffer, bytes_read);
            // client_data.total_bytes += bytes_read;

            // if (client_data.buffer.capacity() - client_data.buffer.size() < CHUNK_SIZE) {
            //     client_data.buffer.reserve(client_data.buffer.capacity() * 2);
            // }
        }

        if (client_data.total_bytes > 0) {
            client_data.buffer.push_back('\0');
            Resp resp = RespDecoder::decode(client_data.buffer.data());
            client_data.buffer.clear();
            client_data.total_bytes = 0;

            if (!resp.success) {
                std::string message = RespEncoder::error(resp.error);
                std::cerr << "Error: " << resp.error << std::endl;
                send(client_fd, message.c_str(), message.size(), 0);
                return;
            }
            handle_op(resp, client_fd);
        }
    }

    /**
     * @brief Close the client connection
     * @param client_fd The file descriptor of the client socket
     * @details This function removes the client from the kqueue and closes the socket.
     */
    void closeConnection(int client_fd) {
        addToKqueue(client_fd, EVFILT_READ, EV_DELETE);
        client_buffers.erase(client_fd);
        close(client_fd);
    }

public:
    /**
     * @brief KqueueServer constructor
     * @param addr Address to bind the server socket to
     * @param port Port number to bind the server socket to
     * @details This constructor initializes the server socket, kqueue, and event list.
     */
    KqueueServer(const std::string &addr, int port) {
        server_socket = createServerSocket(addr, port);
        if (server_socket == -1) {
            throw std::runtime_error("Failed to create server socket");
        }

        kq = createKqueue();
        if (kq == -1) {
            close(server_socket);
            throw std::runtime_error("Failed to create kqueue");
        }

        event_list.resize(INITIAL_EVENT_LIST_SIZE);

        if (!addToKqueue(server_socket, EVFILT_READ, EV_ADD)) {
            close(server_socket);
            close(kq);
            throw std::runtime_error("Failed to add server socket to kqueue");
        }

        std::cout << "Server is listening on " << addr << ":" << port << std::endl;
    }

    /**
     * @brief Run the server loop
     * @details This function enters the main event loop, waiting for events on the kqueue
     *          and handling them accordingly.
     */
    void run() {
        while (true) {
            int new_events = kevent(kq, NULL, 0, event_list.data(), event_list.size(), NULL);
            if (new_events == -1) {
                if (errno == EINTR) {
                    continue;
                }
                std::cerr << "kevent error: " << strerror(errno) << std::endl;
                break;
            }

            if (new_events == static_cast<int>(event_list.size())) {
                event_list.resize(event_list.size() * 2);
            }

            for (int i = 0; i < new_events; i++) {
                int event_fd = event_list[i].ident;

                if (event_list[i].flags & (EV_ERROR | EV_EOF)) {
                    if (event_fd != server_socket) {
                        closeConnection(event_fd);
                    }
                    continue;
                }
                event_fd == server_socket ? handleNewConnection() : handleClientMessage(event_fd);
            }
        }
    }

    ~KqueueServer() {
        for (const auto &client : client_buffers) {
            close(client.first);
        }
        close(server_socket);
        close(kq);
    }
};
#endif