#include <cstdarg>
#include <cstdio>
#include <FreeRTOS.h>
#include <FreeRTOS.hpp>
extern "C"
{
#include <FreeRTOS_UDP_IP.h>
}
#include <iostream>

static char buf[10000];
char* ptr = buf;

extern "C"
{
    void vLoggingPrintf( const char *pcFormatString, ... )
    {
        va_list argptr;
        va_start(argptr, pcFormatString);
        ptr += vsnprintf( ptr, sizeof(buf)-(ptr-buf), pcFormatString, argptr );
        va_end(argptr);
    }
}
#if 0
static FreeRTOS::task<
    configMINIMAL_STACK_SIZE * 2,
    configTIMER_TASK_PRIORITY + 1
> _task (
    [](void*)
    {
        struct freertos_sockaddr xDestinationAddress;
        xDestinationAddress.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr( "1.1.1.1" );
        xDestinationAddress.sin_port = 0xffff;
        xDestinationAddress.sin_family = FREERTOS_AF_INET4;
        Socket_t xSocket = FreeRTOS_socket( FREERTOS_AF_INET4, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
        configASSERT( xSocket != FREERTOS_INVALID_SOCKET );
        while( true )
        {
            if( buf != ptr )
            {
                int ret = FreeRTOS_sendto(
                    xSocket,
                    buf,
                    ptr-buf,
                    0,
                    &xDestinationAddress,
                    sizeof( xDestinationAddress )
                );
                ptr = buf;
                //std::cout << "Hello World!" << std::endl;
            }
            vTaskDelay( 1 );
        }
    },
    "Print Task"
);
#endif