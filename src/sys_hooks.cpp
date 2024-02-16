#include <cstring>

#include <stm32f4xx_hal_rng.h>

extern "C"
{
    int getentropy(void *buffer, size_t length)
    {
        static RNG_HandleTypeDef rng = {
            .Instance = RNG
        };
        static bool initialized = false;
        if( !initialized )
        {
            initialized = true;
            HAL_RNG_Init( &rng );
        }
        size_t rem;
        uint32_t* ptr_u32 = (uint32_t*)buffer;
        while( rem >= 4 )
        {
            uint32_t rand;
            HAL_RNG_GenerateRandomNumber( &rng, &rand );
            *ptr_u32 = rand;
            ptr_u32++;
            rem -= 4;
        }
        uint8_t* ptr_u8 = (uint8_t*)ptr_u32;
        while( rem )
        {
            uint32_t rand;
            HAL_RNG_GenerateRandomNumber( &rng, &rand );
            *ptr_u8 = rand & 0xff;
            ptr_u8++;
            rem--;
        }
        return length;
    }
}
