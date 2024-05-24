#include <iostream>
#include <iomanip>
#include <cmath>
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
#include <algorithm>
#include "commons.h"
#include "utils.h"


using namespace std;

// helper function to set the payload of the udp_message for an integer
void set_int_payload(udp_message& msg, uint8_t sign_int, uint32_t number_int) {
    std::ostringstream payload_stream; // stream to store payload
    if (sign_int == 1) {
        payload_stream << '-'; // add sign to payload
    }
    payload_stream << ntohl(number_int); // add number to payload

    msg.payload.append(payload_stream.str()); // set payload as string
    msg.data_type = TYPE_INT; // set type identifier
}

// helper function to set the payload of the udp_message for a short real
void set_short_real_payload(udp_message& msg, uint16_t number_short) {
    std::ostringstream payload_stream; // stream to store payload
    float correct_short = float(ntohs(number_short)) / 100; // convert number to short real

    if (correct_short == int(correct_short)) { // check if number is integer
        payload_stream << int(correct_short); // add number to payload
    } else { // number is not integer
        payload_stream.precision(2); // set precision to 2 decimals
        payload_stream << fixed << correct_short; // add number to payload
    }

    msg.payload.append(payload_stream.str()); // set payload as string
    msg.data_type = TYPE_SHORT_REAL; // set type identifier
}

// helper function to set the payload of the udp_message for a float
void set_float_payload(udp_message& msg, uint8_t sign_float, uint32_t number_float, uint8_t power) {
    // calculate correct float value by dividing number by 10^power
    float correct_float = float(ntohl(number_float)) / pow(10, int(power));

    // set payload
    if (sign_float == 1) { // number is negative
        msg.payload.push_back('-'); // add sign to payload
    }
    
    std::ostringstream ss; // stream to store payload
    ss << std::setprecision(int(power)) << std::fixed << correct_float; // add number to payload
    msg.payload.append(ss.str()); // set payload as string

    // set type identifier
    msg.data_type = TYPE_FLOAT;
}

// helper function to set the payload of the udp_message for a string
void set_string_payload(udp_message& msg, const char* buffer) {
    msg.payload.append(buffer + TOPIC_LEN + 1); // copy the payload data into the msg.payload string

    // set type identifier
    msg.data_type = TYPE_STRING;
}

// function to read a udp_message from the UDP client and store it in a udp_message struct
udp_message read_udp_message(int sockfd_udp) {
    udp_message msg;
    ostringstream number_stream;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    // clear buffer
    std::fill(buffer, buffer + BUF_LEN, 0);

    // receive udp_message from UDP client and store it in buffer
    n = recvfrom(sockfd_udp, buffer, BUF_LEN, 0, (struct sockaddr *)&cliaddr, &len);
    TRY_CATCH({
        throw std::runtime_error("recvfrom failed");
    });
    

    // set payload
    switch (Type(buffer[TOPIC_LEN])) {
        case INT:
            uint32_t number_int;
            uint8_t sign_int;

            // get sign and number
            std::memcpy(&sign_int, &buffer[TOPIC_LEN + 1], sizeof(uint8_t));
            std::memcpy(&number_int, &buffer[TOPIC_LEN + 2], sizeof(uint32_t));

            set_int_payload(msg, sign_int, number_int);
            break;

        case SHORT_REAL:
            uint16_t number_short;

            // get number
            std::memcpy(&number_short, &buffer[TOPIC_LEN + 1], sizeof(uint16_t));

            // set payload
            set_short_real_payload(msg, number_short);

            // set type identifier
            msg.data_type = TYPE_SHORT_REAL;

            break;

        case FLOAT:
            uint32_t number_float;

            // get sign and number
            std::memcpy(&sign_int, &buffer[TOPIC_LEN + 1], sizeof(uint8_t));
            std::memcpy(&number_float, &buffer[TOPIC_LEN + 2], sizeof(uint32_t));

            // get power
            uint8_t power;
            std::memcpy(&power, &buffer[TOPIC_LEN + 6], sizeof(uint8_t));

            // set payload
            set_float_payload(msg, sign_int, number_float, power);

            // set type identifier
            msg.data_type = TYPE_FLOAT;

            break;

        case STRING:
            // copy the payload data into the msg.payload string
            msg.payload.assign(buffer + TOPIC_LEN + 1, n - TOPIC_LEN - 1);

            // set type identifier
            msg.data_type = TYPE_STRING;
            break;

        default:
            break;
    }

    // set topic
    buffer[TOPIC_LEN] = '\0'; // set null terminator at the end of the topic
    msg.topic = buffer; // set udp message topic

    return msg;
}

// function to create a TCP message from a udp_message
string create_tcp_message(udp_message msg) {
    return msg.udp_client_ip + ":" + to_string(msg.udp_client_port) + " - " +
           msg.topic + " - " + msg.data_type + " - " + msg.payload;
}

// function to send a message to a client
void send_message(string message, int sockfd) {
    const size_t MSG_LEN_SIZE = sizeof(uint32_t); // size of message length

    uint32_t msg_len = message.length(); // length of message
    n = send(sockfd, &msg_len, MSG_LEN_SIZE, 0); // send message length

    TRY_CATCH({
        throw std::runtime_error("send failed");
    });

    size_t sent_len = 0; // length of sent message
    while (sent_len < msg_len) { // while there are still bytes to send
        n = send(sockfd, message.c_str() + sent_len, msg_len - sent_len, 0); // send message payload
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            throw std::runtime_error("send failed");
        } else if (n == 0) {
            throw std::runtime_error("send failed: connection closed by peer");
        }
        sent_len += n; // update sent_len
    }
}

// function to send a message to all subscribers of a topic
void send_to_subscribers(udp_message msg) {
    auto& subscribers = subscribers_DB[msg.topic]; // get subscribers of topic

    if (subscribers.empty()) {
        return; // if there are no subscribers, return
    }

    string message = create_tcp_message(msg); // create TCP message from udp_message

    // send message to all subscribers by iterating through the subscribers vector
    std::for_each(subscribers.begin(), subscribers.end(), [&](subscriber& subscriber) {
        if (subscriber.connected) { // if subscriber is connected, send message
            send_message(message, subscriber.sockfd); // send message to subscriber
        } else if (subscriber.SF) { // if subscriber is not connected but has SF flag set, store message
            subscriber.unsent_messages.push(message); // store message
        }
    });
}

// function to check if there are any messages to store and forward
void check_store_and_forward(int sockfd, char* id) {
    // iterate through all topics
    for (auto& entry : subscribers_DB) {
        auto& subscribers = entry.second; // get subscribers of topic
        for (auto& sub : subscribers) { // iterate through subscribers
            if (sub.ID == id) { // if subscriber is the new client
                sub.sockfd = sockfd; // update sockfd
                sub.connected = true; // set connected flag
                
                if (sub.SF) { // if subscriber has SF flag set
                    while (!sub.unsent_messages.empty()) { // send all stored messages
                        send_message(sub.unsent_messages.front(), sockfd); // send message
                        sub.unsent_messages.pop(); // remove message from queue
                    }
                }
                break;
            }
        }
    }
}

// function to check if a client is connected
bool client_is_connected(const char* id) {
    return std::any_of(connected_clients.begin(), connected_clients.end(),
                       [&](const client& cl) { return strcmp(cl.ID.c_str(), id) == 0; });
}

// function to disconnect a client
void disconnect_client(set<client>::iterator cl) {
    // set subscriber as 'not connected'
    // iterate through all topics
    for (auto& [topic, subscribers] : subscribers_DB) {
        auto it_subscriber = find_if(subscribers.begin(), subscribers.end(), [&](const subscriber& sub) { // find subscriber
            return sub.ID == cl->ID; // return true if subscriber is found
        });
        if (it_subscriber != subscribers.end()) { // if subscriber is found
            it_subscriber->connected = false; // set connected flag to false
        }
    }

    // delete client from connected_clients
    connected_clients.erase(cl);
}

// function to get the command and arguments from the buffer
bool get_command(string& cmd, string& arg1, string& arg2) {
    istringstream iss(buffer);
    iss >> cmd >> arg1;

    if (cmd != "subscribe" && cmd != "unsubscribe") {
        cerr << "Invalid command.\n";
        return false;
    }

    if (cmd == "subscribe") {
        iss >> arg2;

        if (arg2 != "0" && arg2 != "1") {
            cerr << "Invalid second argument.\n";
            return false;
        }
    }

    return true;
}

// function to check if a client is subscribed to a topic
bool client_is_subscribed(const string& topic, const string& id) {
    const auto& topic_subscribers = subscribers_DB.find(topic);
    if (topic_subscribers == subscribers_DB.end()) {
        return false;
    }
    
    return any_of(topic_subscribers->second.begin(), topic_subscribers->second.end(),
                  [&](const subscriber& sub) { return sub.ID == id; });
}

// function to update a subscriber
void update_subscriber(string topic, string id, bool SF) {
    for (auto& sub : subscribers_DB[topic]) {
        if (sub.ID == id) {
            sub.SF = SF;
            return;
        }
    }
}

// function to add a subscriber
void add_subscriber(string topic, subscriber sub) {
    subscribers_DB[topic].emplace_back(std::move(sub));
}

// function to remove a subscriber
void remove_subscriber(string topic, string id) {
    auto& subscribers = subscribers_DB[topic];
    subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
        [&](const subscriber& sub) { return sub.ID == id; }), subscribers.end());
}

// function to create a UDP socket
void create_udp_socket() {
    // create UDP socket file descriptor
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    TRY_CATCH({
        throw std::runtime_error("socket creation failed");
    });
}

// set server address
void set_server_add(char* port) {
    std::fill((char *) &servaddr, (char *) &servaddr + sizeof(servaddr), 0); // <=> memset((char *) &servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(port));
    servaddr.sin_addr.s_addr = INADDR_ANY;
}

// bind socket to server address
void bind_udp_socket(char *port) {
    // set server address
    set_server_add(port);

    // bind socket to server address
    ret = bind(sockfd_udp, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    TRY_CATCH({
        throw std::runtime_error("bind failed");
    });
}

// create TCP socket file descriptor
void create_tcp_socket() {
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    TRY_CATCH({
        throw std::runtime_error("socket creation failed");
    });
}

// deactivate Nagle algorithm
void deactivate_nagle_algorithm() {
    int nagle_flag = 1;
    ret = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *) &nagle_flag, sizeof(int));
    TRY_CATCH({
        throw std::runtime_error("Nagle deactivation failed");
    });
}

// bind TCP socket to server address
void bind_tcp_socket() {
    ret = bind(sockfd_tcp, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    TRY_CATCH({
        throw std::runtime_error("bind failed");
    });
}

// listen on TCP socket
void listen_on_tcp_socket() {
    ret = listen(sockfd_tcp, MAX_BACKLOG);
    TRY_CATCH({
        throw std::runtime_error("socket creation failed");
    });
}

// function to add descriptors to sets
void add_descriptors_to_sets() {
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    FD_SET(sockfd_tcp, &read_fds);
    fdmax = max(sockfd_udp, sockfd_tcp);
}

// function to read stdin input
void read_stdin_input() {
    const std::unordered_map<std::string, std::function<void()>> commands {
        {"exit", [&]() {
            for (auto& cl : connected_clients) {
                send_message(EXIT, cl.sockfd);
                close(cl.sockfd);
            }
            connected_clients.clear();
            throw std::runtime_error(EXIT);
        }}
    };

    string input;
    getline(cin, input);

    auto it = commands.find(input);
    if (it == commands.end()) {
        cerr << "(" << __FILE__ << ", " << __LINE__ << "): " << INVALID_CMD << endl;
        return;
    }

    it->second();
}

// function to handle UDP socket
void handle_udp_socket() {
    udp_msg = read_udp_message(sockfd_udp);
    send_to_subscribers(udp_msg);
}

// function to handle TCP socket
void handle_tcp_socket() {
    clilen = sizeof(cliaddr); // set client address length

    // get new client socket
    sockfd_newcli = accept(sockfd_tcp, (struct sockaddr *) &cliaddr, &clilen);
    TRY_CATCH({
        throw std::runtime_error("accept failed");
    });

    // get message length
    uint32_t msg_len;
    n = recv(sockfd_newcli, &msg_len, sizeof(uint32_t), 0);
    TRY_CATCH({
        throw std::runtime_error("recv failed");
    });

    // get actual message
    std::fill(buffer, buffer + BUF_LEN, 0); // memset(buffer, 0, BUF_LEN);

    n = recv(sockfd_newcli, buffer, msg_len, 0); // receive message
    TRY_CATCH({
        throw std::runtime_error("recv failed");
    });

    // check if client is already connected
    if (client_is_connected(buffer)) {
        cout << "Client " << buffer << " already connected.\n";

        // send EXIT message to client
        send_message(EXIT, sockfd_newcli);

        // close socket
        close(sockfd_newcli);
        return;
    }
    // add new client to connected_clients               
    connected_clients.insert({sockfd_newcli, buffer});

    // add new TCP socket to read_fds and update fdmax
    FD_SET(sockfd_newcli, &read_fds);
    fdmax = max(fdmax, sockfd_newcli);

    // print client connection message
    char client_ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip_str, INET_ADDRSTRLEN) == NULL) {
        perror("Error: failed to convert client IP to string");
    } else {
        cout << "New client " << buffer << " connected from " << client_ip_str << ":" << ntohs(cliaddr.sin_port) << ".\n";
    }

    // check if there are any messages to store and forward
    check_store_and_forward(sockfd_newcli, buffer);
}

// function to close TCP connection
void close_tcp_connection(int i, set<client>::iterator it_client) {
    disconnect_client(it_client);
    close(i);
    FD_CLR(i, &read_fds);

    cout << "Client " << it_client->ID << " disconnected.\n";
}

// function to subscribe to a topic
void subscribe_to_topic(string topic, string id, bool SF, int sockfd) {
    if (client_is_subscribed(topic, id)) {
        update_subscriber(topic, id, SF);
        return;
    }
                        
    add_subscriber(topic, {sockfd, id, SF, true});

    send_message(SUBSCRIBED, sockfd);
}

// function to unsubscribe from a topic
void remove_subscriber_from_topic(string topic, string id, int i) {
    if (!client_is_subscribed(topic, id)) {
        return;
    }

    remove_subscriber(topic, id);

    send_message(UNSUBSCRIBED, i);
}

// function to check if a client has the same socket file descriptor as the one given as argument
bool is_socketfd_equal(const client& cl, int sockfd) {
    return cl.sockfd == sockfd;
}

// function to handle existing TCP socket
void handle_existing_tcp_socket(int i) {
    uint32_t msg_len; // message length
    n = recv(i, &msg_len, sizeof(uint32_t), 0); // receive message length

    TRY_CATCH({
    throw std::runtime_error("recv failed");
    });

    auto it_client = std::find_if(connected_clients.begin(), connected_clients.end(),
                                std::bind(is_socketfd_equal, std::placeholders::_1, i)); // find client

    if (n == 0) {   // closed connection
        close_tcp_connection(i, it_client);
    } else {    // receives message
        std::fill(buffer, buffer + BUF_LEN, 0); // memset(buffer, 0, BUF_LEN);
        n = recv(i, buffer, msg_len, 0); // receive message
        TRY_CATCH({
            throw std::runtime_error("recv failed");
        });

        string cmd, topic, SF;
        if (!get_command(cmd, topic, SF)) {
            return;
        }

        if (cmd == "subscribe") {
            subscribe_to_topic(topic, it_client->ID, SF == "1", i);
        } else {                           
            remove_subscriber_from_topic(topic, it_client->ID, i);
        }
    }
}

// function to run the server
void exec(char *port) {
    create_udp_socket();
    bind_udp_socket(port);
    create_tcp_socket();
    deactivate_nagle_algorithm();
    bind_tcp_socket();
    listen_on_tcp_socket();

    // initialize descriptor sets
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    // add descriptors to sets
    add_descriptors_to_sets();

    while (1) {
        tmp_fds = read_fds;   // copy read_fds to tmp_fds
        
        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL); // select descriptors
            
        TRY_CATCH({
            throw std::runtime_error("select failed");
        });

        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) { // read stdin input
            read_stdin_input();
        }

        // iterate through all descriptors
        for (int i = 1; i <= fdmax; i++) {
            if (!FD_ISSET(i, &tmp_fds)) {  // descriptor is not set
                continue;
            }

            if (i == sockfd_udp) { // new connection request on UDP socket
                handle_udp_socket();
            } else if (i == sockfd_tcp) {   // new connection request on TCP socket
                handle_tcp_socket();
            } else {    // new message from existing TCP connection
                handle_existing_tcp_socket(i);
            }
        }
    }

    close(sockfd_tcp); // close TCP socket
    close(sockfd_udp); // close UDP socket
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    TRY_CATCH({
        if (argc != ARG_NO) {
            throw std::runtime_error("invalid number of arguments");
        }
    });

    exec(argv[1]);

    return 0;
}