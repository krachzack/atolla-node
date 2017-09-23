static const char* msg_socket_is_null = "The given UdpSocket is NULL";
#if defined(_WIN32) || defined(WIN32)
static const char* msg_platform_init_failed = "Initalization of the winsock2 API failed";
#endif
static const char* msg_socket_creation_failed = "Failed to create the socket";
static const char* msg_port_in_use = "Failed to bind the socket to the given port, is it already in use?";
static const char* msg_set_nonblocking_failed = "Failed to set a newly created socket to non-blocking mode";
static const char* msg_bad_braodcast = "An attempt was made to send to a network/broadcast address as though it was a unicast address";
static const char* msg_wouldblock = "The send call would block, but the socket is non-blocking, the most likely reason is that too much data was sent at once";
static const char* msg_no_receiver = "Tried to send data but no receiver is set";
static const char* msg_packet_too_big = "The given message is too large to send it in one piece";
static const char* msg_nothing_received = "No received data is available right now";
