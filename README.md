// Copyright Spinochi Andreea 322CA

Homework 2 - TCP & UDP Client-Server Application

## General description

The program is a easy implementation of a messaging application with a broker server and clients that could enroll in topics and get hold of messages posted on the ones topics. The server listens on sockets: a UDP socket for receiving messages from publishers and a TCP socket for handling client connections.

Clients connect with the server the use of a TCP socket and may ship commands to the server to subscribe or unsubscribe from subjects. Clients can also get a hold of messages from the server for when they're offline. The server broadcasts messages to all clients subscribed to a topic the use of UDP sockets.

When a publisher sends a message on a subject, the server gets the message on its UDP socket and announces it to all customers subscribed to that topic. If a client has the &quot;keep and forward&quot; flag set, the server will shop the message and ahead it to the consumer while it reconnects.

The program is split into two elements: subscriber.cpp and server.cpp. Subscriber.cpp carries the code for the client(how the client is run, how it connects to the server, how it sends and receives messages). Server.cpp carries the code for the server(how the server is run, how it listens for connections, how it receives and sends messages, how it broadcasts messages).

## Implementation

### Server

The server is implemented in server.cpp. It has a TCP socket for listening for connections from clients and a UDP socket for receiving messages from publishers. The server has a vector of clients, a vector of topics and a vector of messages. The server also has a map of topics and a map of messages. The maps are used for storing the topics and messages in a more efficient way.

The server has a function for parsing the input arguments and one for parsing the commands received from clients. The server also has a function for handling the TCP connections from clients and one for handling the UDP messages from publishers.

The server has a function for sending messages to clients. The function takes as arguments the socket of the client, the message to be sent and the length of the message. The function sends the message to the client and checks if the message was sent successfully. If the message was not sent successfully, the function tries to send the message again.

The server has a function for broadcasting messages to clients. The function takes as arguments the topic of the message and the message to be sent. The function iterates through the vector of clients and sends the message to all clients subscribed to the topic. The function also checks if the message was sent successfully. If the message was not sent successfully, the function tries to send the message again.

The server has a function for sending messages to clients when they reconnect. The function takes as arguments the socket of the client and the topic of the message. The function iterates through the vector of messages and sends the messages to the client. The function also checks if the message was sent successfully. If the message was not sent successfully, the function tries to send the message again.

### Main functions: ###

```read_udp_message``` -  function is a helper function used to read a message from a UDP client and store it in a udp_message struct. The function takes in the UDP socket file descriptor sockfd_udp as a parameter.

```send_message``` -  function to send a message to a client over a socket connection. The function takes two arguments: the message to be sent as a string and the socket file descriptor (sockfd) for the client connection. the function enters a while loop to send the message payload to the client in chunks. The function uses the send() function to send as much of the message as possible in each iteration of the while loop. If the send() function is interrupted or blocked (with errno equal to EINTR or EAGAIN), the function continues with the next iteration of the loop. If the send() function returns an error or if it returns 0 (indicating that the connection has been closed by the peer), the function throws a runtime error.
The sent_len variable keeps track of the length of the message that has been sent so far. The loop continues until the entire message has been sent to the client.

```send_to_subscribers``` - This function sends a message to all subscribers of a given topic. It first retrieves the subscribers of the topic from the subscribers_DB map. If there are no subscribers, it returns. Otherwise, it creates a TCP message from the udp_message argument using the create_tcp_message function. It then iterates through the subscribers vector and checks whether each subscriber is connected or has the SF flag set. If the subscriber is connected, it sends the message using the send_message function. If the subscriber is not connected but has the SF flag set, it stores the message in the unsent_messages queue. 

```check_store_and_forward``` - This function checks if there are any messages to store and forward for a new client that has just connected to the server. It iterates through all topics and their subscribers to find the subscriber that matches the new client's ID. Once it finds the matching subscriber, it updates the subscriber's sockfd and connected flag to indicate that it is now connected. If the subscriber has the SF flag set, it sends all stored messages to the new client.

```handle_tcp_socket``` - This function handles the TCP socket for new client connections. It accepts a new client socket and receives a message from the client indicating the ID of the subscriber. If the client is already connected, it sends an "EXIT" message to the client and closes the socket. If the client is new, it adds the client to the list of connected clients and updates the set of file descriptors to be monitored. It also prints a message indicating the client's connection details. Finally, it checks if there are any messages to store and forward to the new client.

```handle_existing_tcp_socket``` - This function handles an existing TCP socket connection. It first receives the message length from the client, and then searches for the client's information in the `connected_clients` map using the socket file descriptor `i`. If the `n` value of the message length received is 0, it means that the connection was closed and the function calls `close_tcp_connection` to handle the disconnection. If the `n` value is greater than 0, the function proceeds to receive the actual message and parses it to determine if it's a "subscribe" or "unsubscribe" command. It then calls `subscribe_to_topic` or `remove_subscriber_from_topic` functions to handle the subscription or unsubscription request, respectively.

```exec``` - This function is responsible for running the server. It first creates and binds the UDP and TCP sockets, and initializes the descriptor sets. Then, it enters an infinite loop where it waits for activity on any of the descriptors using the select function. If activity is detected on the standard input descriptor, the function reads input from the user. If activity is detected on the UDP or TCP sockets, the function handles the connection or message accordingly by calling the appropriate handler functions. Finally, the function closes the sockets before exiting.


### Subscriber

The subscriber is implemented in subscriber.cpp. It has a TCP socket for connecting to the server and a UDP socket for receiving messages from the server. The subscriber has a vector of topics and a vector of messages. The subscriber also has a map of topics and a map of messages. The maps are used for storing the topics and messages in a more efficient way.

The subscriber has a function for parsing the input arguments and one for parsing the commands received from the server. The subscriber also has a function for handling the TCP connection to the server and one for handling the UDP messages from the server.

The subscriber has a function for sending messages to the server. The function takes as arguments the socket of the server, the message to be sent and the length of the message. The function sends the message to the server and checks if the message was sent successfully. If the message was not sent successfully, the function tries to send the message again.

The subscriber has a function for receiving messages from the server. The function takes as arguments the socket of the server, the message to be received and the length of the message. The function receives the message from the server and checks if the message was received successfully. If the message was not received successfully, the function tries to receive the message again.

### Main functions: ###

```send_message``` - This is a function that sends a message to a server through a socket connection. The function takes in a string message and a socket file descriptor (sockfd) as input. It then calculates the length of the message and sends it to the server as a 32-bit unsigned integer. If there is an error sending the message length, a runtime_error is thrown.
Next, the function sends the message payload itself by passing the message string, its length and a flag of 0 to the send() function. If there is an error sending the message payload, a runtime_error is thrown as well.

```exec``` - This function is the main function of the client, which connects to the server and then reads input from stdin and sends it to the server, as well as reading messages from the server and printing them to the console.
First, the function initializes the descriptors, creates the socket, deactivates the Nagle algorithm, and sets the server address. Then, the client sends its ID to the server using the `send_message` function.
Next, the function enters an infinite loop that uses `select` to wait for input from either stdin or the server. If stdin is set, it reads the input and sends it to the server using `send_message`. If the socket is set, it reads the message from the server using `recv`, checks if the message is the exit command, and prints the message to the console.
If the exit command is received, the function closes the socket and exits the program.

Resources used:
- course slides for the select() function (I found it easier to use the select() function instead of the poll()/ epoll() functions)
- labs for the skeleton code for the server and subscriber

Note: Really nice homework, I enjoyed doing it. I learnt a lot of new things. :)