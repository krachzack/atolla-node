#include "udp_socket_results_internal.h"
#include "../test/assert.h"

UdpSocketResult make_success_result()
{
    UdpSocketResult result;
    result.code = UDP_SOCKET_OK;
    result.msg = NULL;
    return result;
}

UdpSocketResult make_err_result(UdpSocketResultCode code, const char* msg)
{
    assert(code != UDP_SOCKET_OK);

    UdpSocketResult result;
    result.code = code;
    result.msg = msg;
    return result;
}
