#ifndef _sockets_headers_h_
#define _sockets_headers_h_

#if defined(ARDUINO_ARCH_ESP8266)

    // On Arduino use WifiUdp library
    #include <Arduino.h>
    #include <WiFiUdp.h>

#elif defined(_WIN32) || defined(WIN32)

    // On windows use almost-bsd sockets
    #pragma comment( lib, "wsock32.lib" )
    #include <winsock2.h>

#else

    // Everywhere else, use bsd sockets
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <netdb.h>

#endif

#endif /* _sockets_headers_h_ */
