/**
 * Basic UDP socket functionality
 *
 * @see http://gafferongames.com/networking-for-game-programmers/sending-and-receiving-packets/
 */
 
#ifndef _udp_socket_h_
#define _udp_socket_h_

#include <stdbool.h>
#include <stddef.h> // for size_t

/**
 * Represents a reference to a native socket.
 *
 * Typically, the contained socket_handle is not used directly but in an
 * abstract manner by calling the functions defined in this module.
 *
 * However, you may use the socket handle for calling operating system
 * functionality that is not covered by this module.
 */
struct UdpSocket
{
    int socket_handle;
};
typedef struct UdpSocket UdpSocket;

/**
 * Defines possible outcomes of calls to functions in this module.
 *
 * The value UDP_SOCKET_OK is equivalent to the integer <code>0</code>,
 * while all values defining errors are equivalent to negative integers.
 *
 * The enum is used in the <code>code</code> property of
 * <code>UdpSocketResultCode</code>.
 */
enum UdpSocketResultCode
{
    UDP_SOCKET_OK = 0,
    UDP_SOCKET_ERR_SOCKET_IS_NULL = -1,
    UDP_SOCKET_ERR_PLATFORM_INIT_FAILED = -2,
    UDP_SOCKET_ERR_SOCKET_CREATION_FAILED = -3,
    UDP_SOCKET_ERR_PORT_IN_USE = -4,
    UDP_SOCKET_ERR_SET_NONBLOCKING_FAILED = -5,
    UDP_SOCKET_ERR_RESOLVE_HOSTNAME_FAILED = -6,
    UDP_SOCKET_ERR_CONNECT_FAILED = -7,
    UDP_SOCKET_ERR_BAD_BROADCAST = -8,
    UDP_SOCKET_ERR_WOULDBLOCK = -9,
    UDP_SOCKET_ERR_NO_RECEIVER = -10,
    UDP_SOCKET_ERR_PACKET_TOO_BIG = -11,
    UDP_SOCKET_ERR_SEND_FAILED = -12,
    UDP_SOCKET_ERR_FREE_FAILED = -13,
    UDP_SOCKET_ERR_NOTHING_RECEIVED = -14,
    UDP_SOCKET_ERR_RECEIVE_FAILED = -15
};
typedef enum UdpSocketResultCode UdpSocketResultCode;

/**
 * The functions in this module return this structure to signal the outcome of
 * the call.
 *
 * The functions guarentee that in the successful case, <code>code</code> is set
 * to UDP_SOCKET_OK and <code>msg</code> is set to <code>NULL</code>. In
 * the error case, <code>code</code> will hold any of the
 * <code>UDP_SOCKET_ERR_*</code> values and msg is guarenteed to point to an
 * error string further describing the error in a human readable form.
 */
struct UdpSocketResult
{
    UdpSocketResultCode code;
    /** Error message when error, NULL on success */
    const char* msg;
};
typedef struct UdpSocketResult UdpSocketResult;

#if defined(ARDUINO_ARCH_ESP8266)
    #include <Arduino.h>
    #include <WiFiUdp.h>
    struct UdpEndpoint {
        IPAddress address;
        int port;
    };
#else
    #if defined(_WIN32) || defined(WIN32)
        #pragma comment( lib, "wsock32.lib" )
        #include <winsock2.h>
        typedef int socklen_t;
    #else
        #include <sys/socket.h>
    #endif
    struct UdpEndpoint {
        struct sockaddr_storage addr;
        socklen_t addr_len;
    };
#endif
typedef struct UdpEndpoint UdpEndpoint;

/**
 * Initializes the given UdpSocket data structure to reference a UDP socket on
 * a free port selected by the operating system.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure.
 *
 * If the <code>code</code> property of the result structure holds the value
 * <code>UDP_SOCKET_OK</code>, which is equivalent to the integer
 * <code>0</code>, the socket is successfully initialized and ready for receiving
 * data. To send data, <code>udp_socket_set_receiver</code> must be called first.
 *
 * After a successful initialization, it is up to the caller to ensure
 * <code>udp_socket_free</code> is called using the same data structure at a later
 * point in time to ensure the socket is properly closed and associated resources
 * are freed.
 *
 * If the initialization failed, the <code>code</code> property of the returned
 * result structure will further specify which error occurred. The value may be
 * any of the <code>UDP_SOCKET_ERR_*</code> values in the error case. The
 * <code>msg</code> property will point to a human readable error message or to
 * <code>NULL</code> in the successful case.
 *
 * In the error case, all resources acquired in the initialization process are
 * automatically freed, and the call will have no side effects other than zeroing
 * the given data structure (given that, <code>socket</code> it did not point to
 * <code>NULL</code>). Calling <code>udp_socket_free</code> in the error case is
 * not necessary.
 */
UdpSocketResult udp_socket_init(UdpSocket* socket);


/**
 * Initializes the given UdpSocket data structure to reference a UDP socket on
 * the given port.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure.
 *
 * If the <code>code</code> property of the result structure holds the value
 * <code>UDP_SOCKET_OK</code>, which is equivalent to the integer
 * <code>0</code>, the socket is successfully initialized and ready for receiving
 * data. To send data, <code>udp_socket_set_receiver</code> must be called first.
 *
 * If the port is already in use, the <code>code</code> property will instead
 * hold the value <code>UDP_SOCKET_ERR_PORT_IN_USE</code>, which is equivalent to
 * <code>-4</code>.
 *
 * After a successful initialization, it is up to the caller to ensure
 * <code>udp_socket_free</code> is called using the same data structure at a later
 * point in time to ensure the socket is properly closed and associated resources
 * are freed.
 *
 * If the initialization failed, the <code>code</code> property of the returned
 * result structure will further specify which error occurred. The value may be
 * any of the <code>UDP_SOCKET_ERR_*</code> values in the error case. The
 * <code>msg</code> property will point to a human readable error message or to
 * <code>NULL</code> in the successful case.
 *
 * In the error case, all resources acquired in the initialization process are
 * automatically freed, and the call will have no side effects other than zeroing
 * the given data structure (given that, <code>socket</code> it did not point to
 * <code>NULL</code>). Calling <code>udp_socket_free</code> in the error case is
 * not necessary.
 */
UdpSocketResult udp_socket_init_on_port(UdpSocket* socket, unsigned short port);

/**
 * Close the socket and free associated resources but not the passed UdpSocket
 * data structure.
 *
 * Note that the operating system might take some time before it makes the port
 * occuppied by this socket available again.
 *
 * It is up to the caller to free the memory used to allocate the UdpSocket
 * structure itself.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure. A successful completion will be signalled with the <code>code</code>
 * property being set to <code>UDP_SOCKET_OK</code>, which is equivalent to
 * the integer <code>0</code>. If the operation failed, the error will be
 * specified with <code>code</code> being set to any of the
 * <code>UDP_SOCKET_ERR_*</code> values and the <code>msg</code> property pointing
 * to a human readable error message.
 */
UdpSocketResult udp_socket_free(UdpSocket* socket);

/**
 * Sets the receiving hostname and port for subsequent calls to
 * <code>udp_socket_send</code>.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure. A successful completion will be signalled with the <code>code</code>
 * property being set to <code>UDP_SOCKET_OK</code>, which is equivalent to
 * the integer <code>0</code>. If the operation failed, the error will be
 * specified with <code>code</code> being set to any of the
 * <code>UDP_SOCKET_ERR_*</code> values and the <code>msg</code> property pointing
 * to a human readable error message.
 */
UdpSocketResult udp_socket_set_receiver(UdpSocket* socket, const char* hostname, unsigned short port);

/**
 * Sets the receiving hostname and port for subsequent calls to
 * <code>udp_socket_send</code>.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure. A successful completion will be signalled with the <code>code</code>
 * property being set to <code>UDP_SOCKET_OK</code>, which is equivalent to
 * the integer <code>0</code>. If the operation failed, the error will be
 * specified with <code>code</code> being set to any of the
 * <code>UDP_SOCKET_ERR_*</code> values and the <code>msg</code> property pointing
 * to a human readable error message.
 */
UdpSocketResult udp_socket_set_endpoint(UdpSocket* socket, UdpEndpoint* endpoint);

/**
 * Sends the packet given using the <code>packet_data</code> pointer and the byte
 * length in <code>packet_data_len</code> to the receiver set with the last
 * successful call to <code>udp_socket_set_receiver</code>. The data pointer
 * must always point to a valid address space and packet_data_len must be
 * greater than zero.
 *
 * The call does not block. That means that upon successful return, the UDP packet is
 * enqueued and will be sent later in the background.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure. A successful completion will be signalled with the <code>code</code>
 * property being set to <code>UDP_SOCKET_OK</code>, which is equivalent to
 * the integer <code>0</code>. If the operation failed, the error will be
 * specified with <code>code</code> being set to any of the
 * <code>UDP_SOCKET_ERR_*</code> values and the <code>msg</code> property pointing
 * to a human readable error message.
 */
UdpSocketResult udp_socket_send(UdpSocket* socket, void* packet_data, size_t packet_data_len);

/**
 * Sends the packet given using the <code>packet_data</code> pointer and the byte
 * length in <code>packet_data_len</code> to the receiver passed with the
 * <code>to</code> parameter. Any previous set_receiver or set_endpoint calls
 * will be maintained and <code>udp_socket_send</code> will have the same behavior
 * as before.
 *
 * The call does not block. That means that upon successful return, the UDP packet is
 * enqueued and will be sent later in the background.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure. A successful completion will be signalled with the <code>code</code>
 * property being set to <code>UDP_SOCKET_OK</code>, which is equivalent to
 * the integer <code>0</code>. If the operation failed, the error will be
 * specified with <code>code</code> being set to any of the
 * <code>UDP_SOCKET_ERR_*</code> values and the <code>msg</code> property pointing
 * to a human readable error message.
 */
 UdpSocketResult udp_socket_send_to(UdpSocket* socket, void* packet_data, size_t packet_data_len, UdpEndpoint* to);

/**
 * TODO document
 *
 * then_respond makes that sends always send to the partner last received from.
 *
 * The result of the operation will be signalled with the returned UdpSocketResult
 * structure. A successful completion will be signalled with the <code>code</code>
 * property being set to <code>UDP_SOCKET_OK</code>, which is equivalent to
 * the integer <code>0</code>. If the operation failed, the error will be
 * specified with <code>code</code> being set to any of the
 * <code>UDP_SOCKET_ERR_*</code> values and the <code>msg</code> property pointing
 * to a human readable error message.
 */
UdpSocketResult udp_socket_receive_from(UdpSocket* socket, void* packet_buffer, size_t packet_buffer_capacity, size_t* received_byte_count, UdpEndpoint* sender);

UdpSocketResult udp_socket_receive(UdpSocket* socket, void* packet_buffer, size_t packet_buffer_capacity, size_t* received_byte_count, bool set_sender_as_receiver);

bool udp_endpoint_equal(UdpEndpoint* a, UdpEndpoint* b);

#endif /* _udp_socket_h_ */
