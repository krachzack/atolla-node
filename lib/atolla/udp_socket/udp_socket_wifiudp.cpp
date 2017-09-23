#if defined(ARDUINO_ARCH_ESP8266)

#include <Arduino.h>
#include <WifiUdp.h>
#include "../test/assert.h"
#include "udp_socket_results_internal.h"
#include "udp_socket_messages.h"

static WiFiUDP Udp;
static bool has_receiver;
static uint16_t receiver_port;
static IPAddress receiver;

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

    Udp.begin(port);
    receiver_port = 0;
    has_receiver = false;

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

    Udp.stop();

    // Overwrite the handle since it is not valid anymore
    socket->socket_handle = -1;

    return make_success_result();
}

UdpSocketResult udp_socket_set_receiver(UdpSocket* socket, const char* hostname, unsigned short port)
{
    assert(false); // Not implemented yet
}

UdpSocketResult udp_socket_set_endpoint(UdpSocket* socket, UdpEndpoint* endpoint)
{
    if(endpoint)
    {
        receiver = endpoint->address;
        receiver_port = endpoint->port;
        has_receiver = true;
    }
    else
    {
        has_receiver = false;
    }
    
    return make_success_result();
}

/*UdpSocketResult udp_socket_get_endpoint(UdpSocket* socket, UdpEndpoint* endpoint)
{
    if(endpoint)
    {
        if(has_receiver)
        {
            endpoint->address = receiver;
            endpoint->port = receiver_port;
        }
        else
        {
            return make_err_result(
                UDP_SOCKET_ERR_NO_RECEIVER,
                msg_no_receiver
            );
        }
    }

    return make_success_result();
}
*/
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

    if(to == NULL)
    {
        if(has_receiver)
        {
            Udp.beginPacket(receiver, receiver_port);
        }
        else
        {
            return make_err_result(
                UDP_SOCKET_ERR_NO_RECEIVER,
                msg_no_receiver
            );
        }
    }
    else
    {
        Udp.beginPacket(to->address, to->port);
    }

    Udp.write((const uint8_t*) packet_data, packet_data_len);
    Udp.endPacket();

    return make_success_result();
}

 UdpSocketResult udp_socket_receive_from(UdpSocket* socket, void* packet_buffer, size_t packet_buffer_capacity, size_t* received_byte_count, UdpEndpoint* sender)
{
    assert(packet_buffer != NULL);
    assert(packet_buffer_capacity > 0);

    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        int bytes_read = Udp.read((unsigned char*) packet_buffer, packet_buffer_capacity);

        if(received_byte_count)
        {
            *received_byte_count = bytes_read;
        }

        if(sender) {
            sender->address = Udp.remoteIP();
            sender->port = Udp.remotePort();
        }

        return make_success_result();
    } else {
        if(received_byte_count)
        {
            *received_byte_count = 0;
        }

        return make_err_result(
            UDP_SOCKET_ERR_NOTHING_RECEIVED,
            msg_nothing_received
        );
    }
}

bool udp_endpoint_equal(UdpEndpoint* a, UdpEndpoint* b)
{
    return a->address == b->address && a->port == b->port;
}

#endif
