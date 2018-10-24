/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "usart.h"
#include "types.h"
#include "helpers.h"
#include "tx_helpers.h"
#include "rx_helpers.h"
#include "control.h"
#include "lfsr.h"
#include "data_table.h"
/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/
osThreadId defaultTaskHandle;
osThreadId RXHandle;
uint32_t RXBuffer[ 512 ];
osStaticThreadDef_t RXControlBlock;
osThreadId TXHandle;
uint32_t TXBuffer[ 512 ];
osStaticThreadDef_t TXControlBlock;
osThreadId ControlHandle;
uint32_t ControlBuffer[ 128 ];
osStaticThreadDef_t ControlControlBlock;
osThreadId SensorHandle;
uint32_t SensorBuffer[ 128 ];
osStaticThreadDef_t SensorControlBlock;
osMessageQId toBeSentQHandle;
uint8_t toBeSentQBuffer[ 8 * sizeof( Data_t ) ];
osStaticMessageQDef_t toBeSentQControlBlock;

/* USER CODE BEGIN Variables */
/**
 * @brief ID of the motor, programmed into the data table in MX_FREERTOS_Init
 */
static const uint16_t MY_ID = 0x01;
/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
void StartDefaultTask(void const * argument);
extern void StartRX(void const * argument);
extern void StartTX(void const * argument);
extern void StartControlTask(void const * argument);
extern void StartSensorTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}                   
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* Init FreeRTOS */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  // Set the ID of this motor
  writeDataTable(ID_IDX, MY_ID);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of RX */
  osThreadStaticDef(RX, StartRX, osPriorityRealtime, 0, 512, RXBuffer, &RXControlBlock);
  RXHandle = osThreadCreate(osThread(RX), NULL);

  /* definition and creation of TX */
  osThreadStaticDef(TX, StartTX, osPriorityHigh, 0, 512, TXBuffer, &TXControlBlock);
  TXHandle = osThreadCreate(osThread(TX), NULL);

  /* definition and creation of Control */
  osThreadStaticDef(Control, StartControlTask, osPriorityAboveNormal, 0, 128, ControlBuffer, &ControlControlBlock);
  ControlHandle = osThreadCreate(osThread(Control), NULL);

  /* definition and creation of Sensor */
  osThreadStaticDef(Sensor, StartSensorTask, osPriorityNormal, 0, 128, SensorBuffer, &SensorControlBlock);
  SensorHandle = osThreadCreate(osThread(Sensor), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the queue(s) */
  /* definition and creation of toBeSentQ */
  osMessageQStaticDef(toBeSentQ, 8, Data_t, toBeSentQBuffer, &toBeSentQControlBlock);
  toBeSentQHandle = osMessageCreate(osMessageQ(toBeSentQ), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
}

/* StartDefaultTask function */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Application */
void StartRX(void const * argument){
    bool statusIsOkay;

    for(;;){
        statusIsOkay = receive();

        if(statusIsOkay){
            processData();
        }
    }
}


void StartTX(void const * argument){
    Data_t data;

    for(;;){
        while(xQueueReceive(toBeSentQHandle, &data, portMAX_DELAY) != pdTRUE);

        update_buffer_contents(&data);

        transmit_buffer_contents();
    }
}

void StartControlTask(void const * argument){
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    for(;;){
        // Service this task once every ms
        vTaskDelayUntil(
            &xLastWakeTime,
            pdMS_TO_TICKS(1)
        );

        controlUpdateStateVariables();

        controlUpdateSignals();
    }
}

void StartSensorTask(void const * argument){
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    for(;;){
        // Service this task once every ms
        vTaskDelayUntil(
            &xLastWakeTime,
            pdMS_TO_TICKS(1)
        );

        // TODO: add routine to read from sensors (perhaps ADC)

        // Dummy routine (to be replaced with real implementation)
        {
            uint16_t data;
            readDataTable(GOAL_POSITION_IDX, &data);
            static uint32_t lfsr = 0x2F; // Seed value
            static uint32_t polynomial = POLY_MASK_PERIOD_63;

            // Add statistical noise to the data
            lfsr_update(&lfsr, polynomial);
            int8_t noise = (lfsr >> 2) - 8;
            if(data + noise > 0){
                data = data + noise;
            }
            writeDataTable(CURRENT_POSITION_IDX, data);
        }

    }
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
