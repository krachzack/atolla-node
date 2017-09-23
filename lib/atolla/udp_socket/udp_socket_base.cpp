#include "udp_socket.h"
#include "udp_socket_results_internal.h"

UdpSocketResult udp_socket_init(UdpSocket* socket)
{
    // use any free port
    return udp_socket_init_on_port(socket, 0);
}

UdpSocketResult udp_socket_receive(UdpSocket* socket, void* packet_buffer, size_t packet_buffer_capacity, size_t* received_byte_count, bool set_sender_as_receiver)
{
    UdpEndpoint endpoint;
    UdpSocketResult receive_result = udp_socket_receive_from(socket, packet_buffer, packet_buffer_capacity, received_byte_count, &endpoint);

    if(set_sender_as_receiver && receive_result.code == UDP_SOCKET_OK) {
        return udp_socket_set_endpoint(socket, &endpoint);
    } else {
        return receive_result;
    }
}

UdpSocketResult udp_socket_send(UdpSocket* socket, void* packet_data, size_t packet_data_len)
{
    return udp_socket_send_to(socket, packet_data, packet_data_len, NULL);
}
