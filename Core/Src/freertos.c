/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
// 우리가 만든 태스크 헤더 포함
#include "task_audio.h"
#include "task_network.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for AudioTask */
osThreadId_t AudioTaskHandle;
const osThreadAttr_t AudioTask_attributes = {
  .name = "AudioTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for UiTask */
osThreadId_t UiTaskHandle;
const osThreadAttr_t UiTask_attributes = {
  .name = "UiTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for NtpTask */
osThreadId_t NtpTaskHandle;
const osThreadAttr_t NtpTask_attributes = {
  .name = "NtpTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for audioQueue */
osMessageQueueId_t audioQueueHandle;
const osMessageQueueAttr_t audioQueue_attributes = {
  .name = "audioQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Audio(void *argument);
void Ui(void *argument);
void Ntp(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
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

  /* Create the queue(s) */
  /* creation of audioQueue */
  audioQueueHandle = osMessageQueueNew (16, sizeof(AudioCommand_t), &audioQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of AudioTask */
  AudioTaskHandle = osThreadNew(Audio, NULL, &AudioTask_attributes);

  /* creation of UiTask */
  UiTaskHandle = osThreadNew(Ui, NULL, &UiTask_attributes);

  /* creation of NtpTask */
  NtpTaskHandle = osThreadNew(Ntp, NULL, &NtpTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  for(;;)
  {
    osDelay(1000); // 시스템 생존 확인용 (Heartbeat)
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Audio */
/**
* @brief Function implementing the AudioTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Audio */
void Audio(void *argument)
{
  /* USER CODE BEGIN Audio */
  // 실제 로직은 App/task_audio.c에 있음
  AudioTask(argument);
  /* USER CODE END Audio */
}

/* USER CODE BEGIN Header_Ui */
/**
* @brief Function implementing the UiTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Ui */
void Ui(void *argument)
{
  /* USER CODE BEGIN Ui */
  // 실제 로직은 App/task_ui.c에 있음
  // UiTask(argument);
  osThreadExit();
  /* USER CODE END Ui */
}

/* USER CODE BEGIN Header_Ntp */
/**
* @brief Function implementing the NtpTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Ntp */
void Ntp(void *argument)
{
  /* USER CODE BEGIN Ntp */
  // 실제 로직은 App/task_network.c에 있음
  EspNtpTask(argument);
  /* USER CODE END Ntp */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

