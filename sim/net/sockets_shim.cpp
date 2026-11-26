// FreeRTOS+TCP socket API -> Linux sockets (see FreeRTOS_IP.h). Return conventions follow
// FreeRTOS+TCP: recv returns 0 on timeout, negative when the peer has gone.
//
// Never blocks in the kernel: under the FreeRTOS POSIX port a task stuck in a system call
// starves every other task. Waits are poll()-with-zero-timeout plus sim_net_yield(), which the
// RTOS build maps to vTaskDelay(1) (default: sleep 1 ms, for plain-thread builds).
#include "FreeRTOS_Sockets.h"
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#ifndef configTICK_RATE_HZ_SIM
#define configTICK_RATE_HZ_SIM 1000
#endif

extern "C" __attribute__((weak)) void sim_net_yield() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

struct SimSocket { int fd; uint32_t rcv_ms = 0xFFFFFFFFu, snd_ms = 0xFFFFFFFFu; };

static const uint32_t MAX_DELAY = 0xFFFFFFFFu;

static uint64_t now_ms()
{
    using namespace std::chrono;
    return uint64_t(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// Wait until fd is ready for `events` or timeout_ms passes. true = ready.
static bool wait_fd(int fd, short events, uint32_t timeout_ms, bool dontwait)
{
    uint64_t start = now_ms();
    for (;;)
    {
        pollfd p{ fd, events, 0 };
        int r = ::poll(&p, 1, 0);
        if (r > 0) return true;
        if (dontwait) return false;
        if (timeout_ms != MAX_DELAY && now_ms() - start >= timeout_ms) return false;
        sim_net_yield();
    }
}

static int make_nonblocking(int fd)
{
    ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);
    return fd;
}

extern "C" {

Socket_t FreeRTOS_socket(long, long, long)
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return FREERTOS_INVALID_SOCKET;
    int one = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    return new SimSocket{ make_nonblocking(fd) };
}

long FreeRTOS_setsockopt(Socket_t s, long, long name, const void* value, size_t)
{
    uint32_t ticks = *static_cast<const uint32_t*>(value);
    uint32_t ms = ticks == MAX_DELAY ? MAX_DELAY : uint32_t(uint64_t(ticks) * 1000u / configTICK_RATE_HZ_SIM);
    if (name == FREERTOS_SO_RCVTIMEO) s->rcv_ms = ms; else s->snd_ms = ms;
    return 0;
}

// Port the TunerStudio listener actually got (ECU_SIM_PORT=0 picks a free one).
uint16_t sim_ts_port = 0;

long FreeRTOS_bind(Socket_t s, freertos_sockaddr* addr, socklen_t)
{
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port   = addr->sin_port;
    if (const char* port = std::getenv("ECU_SIM_PORT")) a.sin_port = htons(uint16_t(std::atoi(port)));
    const char* bind_ip = std::getenv("ECU_SIM_BIND");
    a.sin_addr.s_addr = inet_addr(bind_ip ? bind_ip : "127.0.0.1");
    if (::bind(s->fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) != 0) return -1;
    socklen_t len = sizeof(a);
    ::getsockname(s->fd, reinterpret_cast<sockaddr*>(&a), &len);
    sim_ts_port = ntohs(a.sin_port);
    return 0;
}

long FreeRTOS_listen(Socket_t s, long backlog) { return ::listen(s->fd, int(backlog)) == 0 ? 0 : -1; }

Socket_t FreeRTOS_accept(Socket_t s, freertos_sockaddr*, socklen_t*)
{
    for (;;)
    {
        if (!wait_fd(s->fd, POLLIN, s->rcv_ms, false)) return nullptr;   // accept timeout
        int fd = ::accept(s->fd, nullptr, nullptr);
        if (fd >= 0)
        {
            int one = 1;
            ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
            return new SimSocket{ make_nonblocking(fd) };
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return FREERTOS_INVALID_SOCKET;
    }
}

long FreeRTOS_recv(Socket_t s, void* buf, size_t len, long flags)
{
    if (!wait_fd(s->fd, POLLIN, s->rcv_ms, flags & FREERTOS_MSG_DONTWAIT)) return 0;   // timeout
    for (;;)
    {
        ssize_t r = ::recv(s->fd, buf, len, 0);
        if (r > 0)  return long(r);
        if (r == 0) return -128;                                        // peer closed (-ENOTCONN)
        if (errno == EINTR) continue;
        return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -128;
    }
}

long FreeRTOS_send(Socket_t s, const void* buf, size_t len, long)
{
    size_t done = 0;
    while (done < len)
    {
        if (!wait_fd(s->fd, POLLOUT, s->snd_ms, false)) break;
        ssize_t r = ::send(s->fd, static_cast<const char*>(buf) + done, len - done, MSG_NOSIGNAL);
        if (r > 0) { done += size_t(r); continue; }
        if (r < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        return done ? long(done) : -128;
    }
    return long(done);
}

long FreeRTOS_shutdown(Socket_t s, long) { return ::shutdown(s->fd, SHUT_RDWR) == 0 ? 0 : -1; }

long FreeRTOS_closesocket(Socket_t s)
{
    if (!s || s == FREERTOS_INVALID_SOCKET) return 0;
    ::close(s->fd);
    delete s;
    return 1;
}

uint16_t FreeRTOS_htons(uint16_t v) { return htons(v); }
uint32_t FreeRTOS_inet_addr(const char* d) { return inet_addr(d); }

}
