#include "general.h"

void init_socket_api() {
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

void cleanup_socket_api() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void close_socket(socket_t sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}
