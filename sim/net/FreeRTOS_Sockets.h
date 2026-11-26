#pragma once
#include <cstddef>
#include <cstdint>

#ifndef SIM_SOCKET_TYPES_ONLY
#ifdef __cplusplus
extern "C" {
#endif
#endif

typedef struct SimSocket* Socket_t;
typedef uint32_t socklen_t;

struct freertos_sockaddr
{
    uint8_t  sin_len;
    uint8_t  sin_family;
    uint16_t sin_port;                       // network byte order, like FreeRTOS+TCP
    struct { uint32_t ulIP_IPv4; } sin_address;
};

#define FREERTOS_AF_INET4        2
#define FREERTOS_SOCK_STREAM     1
#define FREERTOS_IPPROTO_TCP     6
#define FREERTOS_INVALID_SOCKET  ((Socket_t)~0UL)
#define FREERTOS_SO_RCVTIMEO     0
#define FREERTOS_SO_SNDTIMEO     1
#define FREERTOS_SHUT_RDWR       2
#define FREERTOS_MSG_DONTWAIT    1

Socket_t  FreeRTOS_socket(long domain, long type, long protocol);
long      FreeRTOS_setsockopt(Socket_t s, long level, long name, const void* value, size_t len);
long      FreeRTOS_bind(Socket_t s, struct freertos_sockaddr* addr, socklen_t len);
long      FreeRTOS_listen(Socket_t s, long backlog);
Socket_t  FreeRTOS_accept(Socket_t s, struct freertos_sockaddr* addr, socklen_t* len);
long      FreeRTOS_recv(Socket_t s, void* buf, size_t len, long flags);
long      FreeRTOS_send(Socket_t s, const void* buf, size_t len, long flags);
long      FreeRTOS_shutdown(Socket_t s, long how);
long      FreeRTOS_closesocket(Socket_t s);
uint16_t  FreeRTOS_htons(uint16_t v);
uint32_t  FreeRTOS_inet_addr(const char* dotted);

#ifndef SIM_SOCKET_TYPES_ONLY
#ifdef __cplusplus
}
#endif
#endif
