#include <stm32f4xx_hal.h>
#include <FreeRTOS.hpp>

static Socket_t xSeggerSocket;

struct cps_state
{
    uint32_t timestamp;
};

static FreeRTOS::streambuf<cps_state,10> streambuf;

extern "C"
{
    void EXTI9_5_IRQHandler();
    void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
}

void EXTI9_5_IRQHandler()
{
    HAL_GPIO_EXTI_IRQHandler( GPIO_PIN_6 );
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch( GPIO_Pin )
    {
        case GPIO_PIN_6:
        {
            cps_state cs =
            {
                .timestamp = DWT->CYCCNT
            };
            BaseType_t xTaskWoken;
            streambuf.sendFromISR( cs, xTaskWoken );
            portYIELD_FROM_ISR( xTaskWoken );
        }
    }
}

static FreeRTOS::task<
    256,
    configTIMER_TASK_PRIORITY
> _cps_send_task (
    [](void*)
    {
        GPIO_InitTypeDef init =
        {
            .Pin = GPIO_PIN_6,
            .Mode = GPIO_MODE_IT_RISING,
            .Pull = GPIO_NOPULL
        };

        HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

        HAL_GPIO_Init( GPIOA, &init );

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
        xBindAddress.sin_port = ( uint16_t ) 100;
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

            while( true )
            {
                cps_state cs;
                streambuf.receive( cs );

                FreeRTOS_send(
                    xClientSock,
                    &cs,
                    sizeof( cs ),
                    0
                );
            }

            FreeRTOS_closesocket( xClientSock );
        }
    },
    "SEGGER_Trace_Task"
);
