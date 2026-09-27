/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdio.h>
#include <string.h>

#define mainDELAY_LOOP_COUNT 20000

/*-----------------------------------------------------------*/

static void vTask1(void *pvParameters) {
    volatile unsigned long ulCount;
    int i = 0;

    for (;;) {
        taskENTER_CRITICAL();
        printf("Task 1 is running: #%d\n", i++);
        taskEXIT_CRITICAL();

        for (ulCount = 0; ulCount < mainDELAY_LOOP_COUNT; ulCount++) {
        }
    }
}

static void vTask2(void *pvParameters) {
    volatile unsigned long ulCount;
    int i = 0;

    for (;;) {
        taskENTER_CRITICAL();
        printf("Task 2 is running: #%d\n", i++);
        taskEXIT_CRITICAL();

        for (ulCount = 0; ulCount < mainDELAY_LOOP_COUNT; ulCount++) {
        }
    }
}

/*-----------------------------------------------------------*/

int main(void) {

    BaseType_t result;

    printf("Before scheduler\n");

    // Perform any hardware setup necessary
    // prvSetupHardware();

    /* --- APPLICATION TASKS CAN BE CREATED HERE --- */

    /* Start the two tasks as described in the comments at the top of this
     * file. */
    result =
        xTaskCreate(vTask1,   /* The function that implements the task. */
                    "Task 1", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    configMINIMAL_STACK_SIZE, /* The size of the stack to allocate to the task. */
                    NULL,                     /* The parameter passed to the task - not used in this simple case. */
                    1,                        /* The priority assigned to the task. */
                    NULL);                    /* The task handle is not required, so NULL is passed. */

    printf("Create vTask1: %ld\n", (long)result);

    result =
        xTaskCreate(vTask2,   /* The function that implements the task. */
                    "Task 2", /* The text name assigned to the task - for debug only as it is not used by the kernel. */
                    configMINIMAL_STACK_SIZE, /* The size of the stack to allocate to the task. */
                    NULL,                     /* The parameter passed to the task - not used in this simple case. */
                    1,                        /* The priority assigned to the task. */
                    NULL);

    printf("Create vTask2: %ld\n", (long)result);

    vTaskStartScheduler();

    printf("After scheduler\n");

    // Execution will only reach here if there was insufficient heap to start the scheduler.
    for (;;)
        ;
    return 0;
}

/*-----------------------------------------------------------*/
