#ifndef _UTILS_SERVER_H
#define _UTILS_SERVER_H 1

#include <iostream>
#include<queue>
#include <poll.h>
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
#include "utils.h"
using namespace std;

#define TOPIC_LEN 50
#define MAX_BACKLOG 128
#define TYPE_INT "INT"
#define TYPE_SHORT_REAL "SHORT_REAL"
#define TYPE_FLOAT "FLOAT"
#define TYPE_STRING "STRING"
#define ARG_NO          2
#define SUBSCRIBED      "Subscribed to topic."
#define UNSUBSCRIBED    "Unsubscribed from topic."
#define INVALID_CMD     "invalid command\n"
#define INVALID_SF      "SF has invalid value\n"

enum Type {INT, SHORT_REAL, FLOAT, STRING};

// Struct representing an incoming UDP message
struct udp_message {
    string udp_client_ip;
    int udp_client_port;
    string topic;
    string data_type;
    string payload;
    uint32_t payload_size;
};

// Struct representing a subscriber to a certain topic
struct subscriber {
    int sockfd; // Socket file descriptor
    string ID;  // Unique ID of the subscriber
    bool SF;    // Store and forward flag
    bool connected; // Whether the subscriber is currently connected
    uint8_t SF_value; // Value of the SF flag
    queue<string> unsent_messages; // Queue of unsent messages for the subscriber
};

// Struct representing a client that has connected to the server
struct client {
    int sockfd; // Socket file descriptor
    string ID;  // Unique ID of the client

    // Operator used for sorting clients in a set
    bool operator <(const client& cl) const {
        return ID < cl.ID;
    }
};

// Server variables
int sockfd_udp;              // UDP socket file descriptor
int sockfd_tcp;              // TCP socket file descriptor
int sockfd_newcli;           // new client socket file descriptor
int ret, n;
struct sockaddr_in servaddr; // server address
struct sockaddr_in cliaddr;  // client address
socklen_t clilen;
fd_set read_fds, tmp_fds;             // file descriptor sets used in poll()
int fdmax;			         // maximum file descriptor value in read_fds
udp_message udp_msg;
char buffer[BUF_LEN];
unordered_map<string, vector<subscriber>> subscribers_DB;   // subscribers data base
set<client> connected_clients;                              // set containing connected clients

#endif
