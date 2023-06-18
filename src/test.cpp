#include <FreeRTOS.hpp>

void test( void* )
{
    int i = 0;
    while( true )
    {
        i++;
        vTaskDelay( 1000 );
    }
}

FreeRTOS::task<
    configMINIMAL_STACK_SIZE,
    configTIMER_TASK_PRIORITY
> _task (
    test,
    "task"
);

