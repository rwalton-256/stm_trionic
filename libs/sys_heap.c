#include <stddef.h>
#include <stdlib.h>

void * pvPortMalloc( size_t xWantedSize )
{
    return malloc( xWantedSize );
}

void vPortFree( void * pv )
{
    free( pv );
}

size_t xPortGetMinimumEverFreeHeapSize()
{
    return __SIZE_MAX__;
}

size_t xPortGetFreeHeapSize()
{
    return __SIZE_MAX__;
}
