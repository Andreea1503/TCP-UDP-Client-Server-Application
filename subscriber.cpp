#include <iostream>
#include <iomanip>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "commons.h"
#include "helper.h"

using namespace std;

// send message to server
void send_message(string message, int sockfd) {
    const size_t MSG_LEN_SIZE = sizeof(uint32_t); // size of message length

    uint32_t msg_len = message.length(); // length of message
    n = send(sockfd, &msg_len, MSG_LEN_SIZE, 0); // send message length

    TRY_CATCH({
     throw runtime_error("send failed");
    });

    n = send(sockfd, message.c_str(), message.length(), 0); // send message payload
    TRY_CATCH({
     throw runtime_error("send failed");
    });
}

// connect client to server
void connect_client_to_server(int sockfd, struct sockaddr_in servaddr) {
    ret = connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)); // connect to server
    TRY_CATCH({
     throw runtime_error("connect failed");
    });
}

// initialize descriptors
void initialize_descriptors() {
    FD_SET(STDIN_FILENO, &read_fds); // add stdin to read descriptors
    FD_SET(sockfd, &read_fds); // add socket to read descriptors
}

// create socket
void create_socket() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    TRY_CATCH({
     throw runtime_error("socket creation failed");
    });
}

void deactivate_nagle_algorithm() {
    // deactivate Nagle algorithm
    int nagle_flag = 1;
    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &nagle_flag, sizeof(int));
    TRY_CATCH({
        throw std::runtime_error("Nagle deactivation failed");
    });
}

// set server address
void set_server_address(int address, char* port) {
    std::fill((char *) &servaddr, (char *) &servaddr + sizeof(servaddr), 0); // clear servaddr

    servaddr.sin_family = AF_INET; // set family
    servaddr.sin_port = htons(address); // set port
    ret = inet_aton(port, &servaddr.sin_addr); // set address
    TRY_CATCH({
     throw runtime_error("inet_aton failed");
    });
}

// read input from stdin
std::string read_stdin_input() {
    std::string command; // command to be executed
    getline(std::cin, command); // read command
    return command; // return command
}

// run client, connect to server, send ID, read input from stdin and send it to server
void exec(char *argv[]) {
    initialize_descriptors();
    create_socket();
    deactivate_nagle_algorithm();
    set_server_address(atoi(argv[3]), argv[2]);
    connect_client_to_server(sockfd, servaddr);

    // send ID
    send_message(argv[1], sockfd);

    // add stdin and socket to read descriptors
    FD_SET(sockfd, &read_fds);

    while (1) {
        tmp_fds = read_fds; // copy read descriptors

        int nfds = sockfd + 1; // number of descriptors

        int ret = select(nfds, &tmp_fds, NULL, NULL, NULL); // select descriptors
        TRY_CATCH({
            if (ret < 0) {
                throw std::runtime_error("select failed");
            }
        });

        // if stdin is set, read input and send it to server
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            std::string command = read_stdin_input();

            // if command is exit, close socket and exit
            if (command == EXIT) {
                close(sockfd);
                exit(0);
            }

            send_message(command, sockfd);
        }

        // if socket is set, read message from server
        if (FD_ISSET(sockfd, &tmp_fds)) {
            uint32_t msg_len;
            // read message length
            recv(sockfd, &msg_len, sizeof(uint32_t), 0);

            // read message payload
            std::fill(buffer, buffer + BUF_LEN, 0);
            recv(sockfd, buffer, msg_len, 0);

            // if message is exit, close socket and exit
            if (strncmp(buffer, EXIT, strlen(EXIT)) == 0) {
                close(sockfd);
                exit(0);
            }

            // print message
            std::cout << buffer << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    // disable buffering for stdout
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    TRY_CATCH({
        if (argc != ARG_NO) {
            throw std::runtime_error("invalid number of arguments");
        }
    });

    // run client
    exec(argv);

    return 0;
}