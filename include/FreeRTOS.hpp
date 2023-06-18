#pragma once

#include <functional>

#include <FreeRTOS.h>

#include <semphr.h>
#include <stream_buffer.h>
#include <task.h>

namespace FreeRTOS
{
    class mutex_base
    {
    protected:
        StaticSemaphore_t _str;
        SemaphoreHandle_t _hdl;
    public:
        mutex_base()
            : _hdl( xSemaphoreCreateBinaryStatic( &_str ) )
        {
        }
        BaseType_t take()
        {
            return xSemaphoreTake( _hdl, portMAX_DELAY );
        }
        BaseType_t give()
        {
            return xSemaphoreGive( _hdl );
        }
        BaseType_t takeFromISR( BaseType_t& xTaskWoken )
        {
            return xSemaphoreTakeFromISR( _hdl, &xTaskWoken );
        }
        BaseType_t giveFromISR( BaseType_t& xTaskWoken )
        {
            return xSemaphoreGiveFromISR( _hdl, &xTaskWoken );
        }
    };

    class Given {};
    class Taken {};


    template <class>
    class mutex;

    template<>
    class mutex< Given > : public mutex_base
    {
    public:
        mutex< Given >()
        {
            xSemaphoreGive( _hdl );
        }
    };

    template<>
    class mutex< Taken > : public mutex_base
    {
    public:
        mutex< Taken >() {}
    };

    template <typename T, size_t size, size_t trig=1>
    class streambuf
    {
    protected:
        StaticStreamBuffer_t _str;
        StreamBufferHandle_t _hdl;
        T _buf[ size ];
    public:
        streambuf<T,size,trig>()
            : _hdl(
                xStreamBufferCreateStatic(
                    sizeof( _buf ),
                    trig,
                    (uint8_t*)_buf,
                    &_str
                )
            )
        {}
        size_t send( const T* ptr, size_t num, TickType_t timeout=portMAX_DELAY )
        {
            return xStreamBufferSend( _hdl, ptr, sizeof( T ) * num, timeout );
        }
        size_t send( const T& t, TickType_t timeout=portMAX_DELAY )
        {
            return send( &t, 1, timeout );
        }
        size_t receive( T* ptr, size_t num, TickType_t timeout=portMAX_DELAY )
        {
            return xStreamBufferReceive( _hdl, ptr, sizeof( T ) * num, timeout );
        }
        size_t receive( T& t, TickType_t timeout=portMAX_DELAY )
        {
            return receive( &t, 1, timeout );
        }
        size_t sendFromISR( const T* ptr, size_t num, BaseType_t& xTaskWoken )
        {
            return xStreamBufferSendFromISR( _hdl, ptr, sizeof( T ) * num, &xTaskWoken );
        }
        size_t sendFromISR( const T& t, BaseType_t& xTaskWoken  )
        {
            return sendFromISR( &t, 1, xTaskWoken );
        }
        size_t receiveFromISR( T* ptr, size_t num, BaseType_t& xTaskWoken )
        {
            return xStreamBufferReceiveFromISR( _hdl, ptr, sizeof( T ) * num, &xTaskWoken );
        }
        size_t receiveFromISR( T& t, BaseType_t& xTaskWoken )
        {
            return receiveFromISR( &t, 1, xTaskWoken );
        }
    };

    template <size_t stack_size, UBaseType_t priority>
    class task
    {
    protected:
        StaticTask_t _str;
        StackType_t _stck[ stack_size ];
        TaskHandle_t _hdl;
    public:
        task<stack_size,priority>(
            TaskFunction_t func,
            const char* name,
            void* pvParam=nullptr
        )
            : _hdl(
                xTaskCreateStatic(
                    func,
                    name,
                    stack_size,
                    pvParam,
                    priority,
                    _stck,
                    &_str
                )
            )
        {}
        ~task<stack_size,priority>()
        {
            vTaskDelete( NULL );
        }
    };

}

