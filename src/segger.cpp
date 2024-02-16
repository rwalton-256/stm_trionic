#include <FreeRTOS.hpp>
#include <SEGGER_RTT.h>
#include <SEGGER_SYSVIEW.h>

static char buf[ 1024 ];

static Socket_t xSeggerSocket;

static FreeRTOS::task<
    256,
    configTIMER_TASK_PRIORITY
> _segger_tx_task (
    [](void*)
    {
        xSeggerSocket = FreeRTOS_socket(
            FREERTOS_AF_INET4,
            FREERTOS_SOCK_STREAM,
            FREERTOS_IPPROTO_TCP
        );
        configASSERT( xSeggerSocket != FREERTOS_INVALID_SOCKET );

        TickType_t xReceiveTimeOut = portMAX_DELAY;
        FreeRTOS_setsockopt(
            xSeggerSocket,
            0,
            FREERTOS_SO_RCVTIMEO,
            &xReceiveTimeOut,
            sizeof( xReceiveTimeOut )
        );

        struct freertos_sockaddr xBindAddress;
        xBindAddress.sin_port = ( uint16_t ) 19112;
        xBindAddress.sin_port = FreeRTOS_htons( xBindAddress.sin_port );
        configASSERT(
            ! FreeRTOS_bind(
                xSeggerSocket,
                &xBindAddress,
                sizeof( xBindAddress )
            )
        );

        struct freertos_sockaddr xClientAddr;
        socklen_t xClientAddrLen = sizeof( xClientAddr );

        configASSERT( 
            ! FreeRTOS_listen(
                xSeggerSocket,
                1
            )
        );

        while( true )
        {

            Socket_t xClientSock = FreeRTOS_accept(
                xSeggerSocket,
                &xClientAddr,
                &xClientAddrLen
            );
            configASSERT( xClientSock != FREERTOS_INVALID_SOCKET );

            xReceiveTimeOut = 100;
            FreeRTOS_setsockopt(
                xClientSock,
                0,
                FREERTOS_SO_RCVTIMEO,
                &xReceiveTimeOut,
                sizeof( xReceiveTimeOut )
            );

            const char* version_info = "SEGGER SystemView V0 .0 0 0 \0\0\0\0";
            FreeRTOS_send(
                xClientSock,
                version_info,
                32,
                0
            );

            while( true )
            {
                int num_read = SEGGER_RTT_ReadUpBufferNoLock( SEGGER_SYSVIEW_GetChannelID(), buf, sizeof( buf ) );
                if( num_read )
                {
                    FreeRTOS_send(
                        xClientSock,
                        buf,
                        num_read,
                        0
                    );
                }
                BaseType_t xNumRead = FreeRTOS_recv(
                    xClientSock,
                    buf,
                    sizeof( buf ),
                    FREERTOS_MSG_DONTWAIT
                );
                if( xNumRead < 0 )
                {
                    break;
                }
                if( xNumRead )
                {
                    SEGGER_RTT_WriteDownBufferNoLock( SEGGER_SYSVIEW_GetChannelID(), buf, xNumRead );
                }
            }

            FreeRTOS_closesocket( xClientSock );
        }
    },
    "SEGGER_Trace_Task"
);
