// If not compiled for ESP8266 must be either windows or posix sockets
#ifndef ARDUINO_ARCH_ESP8266

#include <unistd.h> // for close
#include <stdio.h> // for sprintf
#include <stdbool.h>
#include <string.h> // for memcpy and memset
#include <errno.h>
#include "../test/assert.h"
#include "sockets_headers.h"
#include "udp_socket_messages.h"
#include "udp_socket.h"
#include "udp_socket_results_internal.h"

/** Free resources associated with the socket allocated by the operating system */
static UdpSocketResult udp_socket_close(UdpSocket* socket);
static UdpSocketResult udp_socket_initialize_socket_support(UdpSocket* socket);
static UdpSocketResult udp_socket_create_socket(UdpSocket* sock, unsigned short port);
static UdpSocketResult udp_socket_set_socket_nonblocking(UdpSocket* socket);

#if defined(_WIN32) || defined(WIN32)
    WSADATA WsaData;
#endif

UdpSocketResult udp_socket_init_on_port(UdpSocket* socket, unsigned short port)
{
    if(socket == NULL)
    {
        return make_err_result(
            UDP_SOCKET_ERR_SOCKET_IS_NULL,
            msg_socket_is_null
        );
    }

    // Intialize everything to zero, false, by standard is always 0
    memset(socket, 0, sizeof(UdpSocket));


    UdpSocketResult result;

    result = udp_socket_initialize_socket_support(socket);
    if(result.code != UDP_SOCKET_OK)
    {
        return result;
    }

    result = udp_socket_create_socket(socket, port);
    if(result.code != UDP_SOCKET_OK)
    {
        return result;
    }

    result = udp_socket_set_socket_nonblocking(socket);
    if(result.code != UDP_SOCKET_OK)
    {
        close(socket->socket_handle);

        return result;
    }

    return make_success_result();
}

static UdpSocketResult udp_socket_create_socket(UdpSocket* sock, unsigned short port)
{
#ifdef UDP_SOCKET_IPV4_ONLY

    sock->socket_handle = socket(PF_INET,
                                 SOCK_DGRAM,
                                 IPPROTO_UDP);

    if(sock->socket_handle <= 0)
    {
        return make_err_result(
            UDP_SOCKET_ERR_SOCKET_CREATION_FAILED,
            msg_socket_creation_failed
        );
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    // Port 0 -> Let system select a free port
    address.sin_port = htons((unsigned short) port);

#else

    sock->socket_handle = socket(PF_INET6,
                                 SOCK_DGRAM,
                                 IPPROTO_UDP);

    if(sock->socket_handle <= 0)
    {
        return make_err_result(
            UDP_SOCKET_ERR_SOCKET_CREATION_FAILED,
            msg_socket_creation_failed
        );
    }

    struct sockaddr_in6 address;
    address.sin6_family = AF_INET6;
    // Port 0 -> Let system select a free port
    address.sin6_port = htons((unsigned short) port);
    address.sin6_addr = in6addr_any;

    int mode = 0;
    int setopt_err = setsockopt(sock->socket_handle, IPPROTO_IPV6, IPV6_V6ONLY, (const void*)&mode, sizeof(mode));
    assert(setopt_err == 0);

#endif

    int bind_err = bind(sock->socket_handle,
                        (const struct sockaddr*) &address,
                        sizeof(address));

    if (bind_err < 0)
    {
        return make_err_result(
            UDP_SOCKET_ERR_PORT_IN_USE,
            msg_port_in_use
        );
    }

    return make_success_result();
}

static UdpSocketResult udp_socket_set_socket_nonblocking(UdpSocket* socket)
{
    #if defined(_WIN32) || defined(WIN32)

        DWORD nonBlocking = 1;
        if(ioctlsocket(socket->socket_handle,
                       FIONBIO,
                       &nonBlocking) != 0)
        {
            return make_err_result(
                UDP_SOCKET_ERR_SET_NONBLOCKING_FAILED,
                msg_set_nonblocking_failed
            );
        }

    #else

        int nonBlocking = 1;
        if(fcntl(socket->socket_handle,
                    F_SETFL,
                    O_NONBLOCK,
                    nonBlocking) == -1)
        {
            return make_err_result(
                UDP_SOCKET_ERR_SET_NONBLOCKING_FAILED,
                msg_set_nonblocking_failed
            );
        }

    #endif

    return make_success_result();
}

/**
 * Does platform-specific intialization, if needed
 */
static UdpSocketResult udp_socket_initialize_socket_support(UdpSocket* socket)
{
    #if defined(_WIN32) || defined(WIN32)
        static bool initialized = false;

        if(!initialized)
        {
            initialized = WSAStartup(MAKEWORD(2,2),
                                     WsaData)
                          == NO_ERROR;

            if(!initialized)
            {
                return make_err_result(
                    UDP_SOCKET_ERR_PLATFORM_INIT_FAILED,
                    msg_platform_init_failed
                );
            }
        }

        // REVIEW: Do I have to call WSACleanup() on Windows;
    #endif

    return make_success_result();
}

UdpSocketResult udp_socket_free(UdpSocket* socket)
{
    if(socket == NULL)
    {
        return make_err_result(
            UDP_SOCKET_ERR_SOCKET_IS_NULL,
            msg_socket_is_null
        );
    }

    UdpSocketResult result = udp_socket_close(socket);
    if(result.code != UDP_SOCKET_OK)
    {
        return result;
    }

    // Overwrite the handle since it is not valid anymore
    socket->socket_handle = -1;

    return make_success_result();
}

static UdpSocketResult udp_socket_close(UdpSocket* socket)
{
    #if defined(_WIN32) || defined(WIN32)
        int error = closesocket(socket->socket_handle);
        if(error != 0)
        {
            return make_err_result(
                UDP_SOCKET_ERR_FREE_FAILED,
                "Closing the socket failed"
            );
        }
    #else
        int error = close(socket->socket_handle);
        if(error == -1)
        {
            return make_err_result(
                UDP_SOCKET_ERR_FREE_FAILED,
                strerror(errno)
            );
        }
    #endif

    return make_success_result();
}

UdpSocketResult udp_socket_set_receiver(UdpSocket* socket, const char* hostname, unsigned short port_short)
{
    if(socket == NULL)
    {
        return make_err_result(
            UDP_SOCKET_ERR_SOCKET_IS_NULL,
            msg_socket_is_null
        );
    }

    // A two-byte number can be a maximum of five characters in a string plus one \0
    char port[6];
    sprintf(port, "%hu", port_short);

    struct addrinfo* first_result = NULL;

    struct addrinfo criteria;
    memset(&criteria, 0, sizeof criteria);
    // IPv6 only, map to ipv4 if necessary
#ifndef UDP_SOCKET_IPV4_ONLY
    criteria.ai_family = AF_INET6;
#else
    criteria.ai_family = AF_UNSPEC;
#endif
    // packet oriented rather than connection oriented
    // is preferred when looking up name
    criteria.ai_socktype = SOCK_DGRAM;
    // specificially, prefer UDP not UDP lite or RTP
    criteria.ai_protocol = IPPROTO_UDP;
    // AI_ADDRCONFIG means to only use IPv6 if it is configured in the local system
    //               and to only use IPv4 if that is configured
    // AI_V4MAPPED means to return IPv4 if IPv6 is preferred but not available
    criteria.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;

    int error = getaddrinfo(
        hostname,
        port,
        &criteria,
        &first_result
    );

    if (error != 0)
    {
        if (error == EAI_SYSTEM)
        {
            return make_err_result(
                UDP_SOCKET_ERR_RESOLVE_HOSTNAME_FAILED,
                strerror(errno)
            );
        }
        else
        {
            return make_err_result(
                UDP_SOCKET_ERR_RESOLVE_HOSTNAME_FAILED,
                gai_strerror(error)
            );
        }
    }
    else
    {
        struct addrinfo* curr_result = first_result;

        assert(first_result != NULL);

        do
        {
            assert(curr_result->ai_socktype == SOCK_DGRAM);
            assert(curr_result->ai_protocol == IPPROTO_UDP);

            error = connect(
                socket->socket_handle,
                curr_result->ai_addr,
                curr_result->ai_addrlen
            );

            curr_result = curr_result->ai_next;
        }
        while(error != 0 && curr_result != NULL);

        freeaddrinfo(first_result);

        if (error != 0)
        {
            return make_err_result(
                UDP_SOCKET_ERR_CONNECT_FAILED,
                strerror(errno)
            );
        }
    }

    return make_success_result();
}

UdpSocketResult udp_socket_set_endpoint(UdpSocket* socket, UdpEndpoint* endpoint)
{
    if(socket == NULL)
    {
        return make_err_result(
            UDP_SOCKET_ERR_SOCKET_IS_NULL,
            msg_socket_is_null
        );
    }

    int error;
    if(endpoint) {
        error = connect(
            socket->socket_handle,
            (struct sockaddr*) &endpoint->addr,
            endpoint->addr_len
        );
    } else {
        // When null pointer given, disconnect
        struct sockaddr addr;
        memset(&addr, 0, sizeof(addr));
        addr.sa_family = AF_UNSPEC;
        error = connect(
            socket->socket_handle,
            &addr,
            sizeof(addr)
        );
    }

    if(error == 0) {
        return make_success_result();
    } else {
        return make_err_result(
            UDP_SOCKET_ERR_CONNECT_FAILED,
            strerror(errno)
        );
    }
}

UdpSocketResult udp_socket_receive_from(UdpSocket* socket, void* packet_data, size_t max_packet_size, size_t* received_byte_count, UdpEndpoint* sender)
{
    assert(packet_data != NULL);
    assert(max_packet_size > 0);

    ssize_t received_bytes;

    if(sender) {
        // Initialize to capacity for recvfrom
        sender->addr_len = sizeof(sender->addr);

        received_bytes = recvfrom(
            socket->socket_handle,
            (char*) packet_data,
            max_packet_size,
            0,
            (struct sockaddr*) &sender->addr,
            &sender->addr_len
        );
    } else {
        received_bytes = recvfrom(
            socket->socket_handle,
            (char*) packet_data,
            max_packet_size,
            0,
            NULL,
            NULL
        );
    }

    if(received_bytes <= 0)
    {
        if(received_byte_count)
        {
            *received_byte_count = 0;
        }

        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return make_err_result(
                UDP_SOCKET_ERR_NOTHING_RECEIVED,
                msg_nothing_received
            );
        }
        else
        {
            return make_err_result(
                UDP_SOCKET_ERR_RECEIVE_FAILED,
                strerror(errno)
            );
        }
    }
    else
    {
        if(received_byte_count)
        {
            *received_byte_count = (size_t) received_bytes;
        }

        return make_success_result();
    }
}

UdpSocketResult udp_socket_send_to(UdpSocket* socket, void* packet_data, size_t packet_data_len, UdpEndpoint* to)
{
    assert(packet_data != NULL);
    assert(packet_data_len > 0);

    if(socket == NULL)
    {
        return make_err_result(
            UDP_SOCKET_ERR_SOCKET_IS_NULL,
            msg_socket_is_null
        );
    }

    ssize_t sent_bytes;
    if(to)
    {
        sent_bytes = sendto(socket->socket_handle,
                            packet_data,
                            packet_data_len,
                            0,
                            (const struct sockaddr*) &to->addr,
                            to->addr_len);
    }
    else
    {
        sent_bytes = send(socket->socket_handle,
                          packet_data,
                          packet_data_len,
                          0);
    }

    if(sent_bytes == -1)
    {
        if(errno == EACCES)
        {
            return make_err_result(
                UDP_SOCKET_ERR_BAD_BROADCAST,
                msg_bad_braodcast
            );
        }
        else if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return make_err_result(
                UDP_SOCKET_ERR_WOULDBLOCK,
                msg_wouldblock
            );
        }
        else if(errno == EDESTADDRREQ)
        {
            return make_err_result(
                UDP_SOCKET_ERR_NO_RECEIVER,
                msg_no_receiver
            );
        }
        else if(errno == EMSGSIZE)
        {
            return make_err_result(
                UDP_SOCKET_ERR_PACKET_TOO_BIG,
                msg_packet_too_big
            );
        }
        else
        {
            return make_err_result(
                UDP_SOCKET_ERR_SEND_FAILED,
                strerror(errno)
            );
        }
    }

    assert(((size_t) sent_bytes) == packet_data_len);

    return make_success_result();
}

bool udp_endpoint_equal(UdpEndpoint* a, UdpEndpoint* b)
{
    if(a->addr_len != b->addr_len)
    {
        return false;
    }

    const size_t len = a->addr_len;
    return 0 == memcmp(&a->addr, &b->addr, len);
}

#endif
