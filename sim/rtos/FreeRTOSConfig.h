// FreeRTOS config for the host simulation (POSIX port). Mirrors libs/include/FreeRTOSConfig.h
// where it matters to the firmware (tick rate, priorities, static allocation); no NVIC settings.
#pragma once
#include <assert.h>
#define configUSE_PREEMPTION              1
#define configUSE_IDLE_HOOK               1
#define configUSE_TICK_HOOK               0
#define configMAX_PRIORITIES              7
#define configSUPPORT_STATIC_ALLOCATION   1
#define configSUPPORT_DYNAMIC_ALLOCATION  1
#define configTOTAL_HEAP_SIZE             (256 * 1024)
#define configTICK_RATE_HZ                1000
#define configMINIMAL_STACK_SIZE          ((unsigned short)PTHREAD_STACK_MIN / sizeof(StackType_t) + 1)
#define configMAX_TASK_NAME_LEN           16
#define configUSE_TRACE_FACILITY          1
#define configTICK_TYPE_WIDTH_IN_BITS     TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD           1
#define configUSE_MUTEXES                 1
#define configUSE_RECURSIVE_MUTEXES       1
#define configUSE_COUNTING_SEMAPHORES     1
#define configQUEUE_REGISTRY_SIZE         8
#define configCHECK_FOR_STACK_OVERFLOW    0
#define configUSE_MALLOC_FAILED_HOOK      0
#define configUSE_TIMERS                  0
#define configTIMER_TASK_PRIORITY         2
#define configTIMER_QUEUE_LENGTH          10
#define configTIMER_TASK_STACK_DEPTH      (configMINIMAL_STACK_SIZE * 2)
#define configUSE_CO_ROUTINES             0
#define configKERNEL_PROVIDED_STATIC_MEMORY 1
#define INCLUDE_vTaskDelay                1
#define INCLUDE_xTaskDelayUntil           1
#define INCLUDE_vTaskDelete               1
#define INCLUDE_vTaskSuspend              1
#define INCLUDE_xTaskGetSchedulerState    1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define configASSERT(x)                   assert(x)
#include <limits.h>
#include <pthread.h>
