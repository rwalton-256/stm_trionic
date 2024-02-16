#include <stm32f4xx_hal.h>
#include <FreeRTOS.hpp>
extern "C"
{
#include <FreeRTOS_ICMP.h>
}
#include <iostream>
#include <chrono>

#if 0
char buf[1400] = "Hello World!\n";

static FreeRTOS::task<
    configMINIMAL_STACK_SIZE,
    configTIMER_TASK_PRIORITY
> _task (
    [](void*)
    {
        GPIO_InitTypeDef init =
        {
            .Pin = GPIO_PIN_13,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL
        };
        HAL_GPIO_Init( GPIOC, &init );
        while( true )
        {
            //FreeRTOS_sendto( xSocket, "Hello World!\n\r", strlen( "Hello World!\n\r" ), NULL, &xDestinationAddress, sizeof( xDestinationAddress ) );
            //printf(";laksjdf");
            HAL_GPIO_TogglePin( GPIOC, GPIO_PIN_13 );
            vLoggingPrintf("HELLO WORLD!\n\r");
            vTaskDelay( 1000 );
            //std::cout << "Hello World!" << ( std::chrono::high_resolution_clock::now() - std::chrono::high_resolution_clock::now() ).count() << std::endl;
        }
    },
    "task"
);

int _write(int file, char *data, int len)
{
    configASSERT(0);
}
#endif
int
_gettimeofday_r(
    struct _reent *ptr,
    struct timeval *ptimeval,
    void *ptimezone
)
{
    configASSERT(0);
}
