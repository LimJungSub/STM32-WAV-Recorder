/* USER CODE BEGIN Header */
// FatFs 애플리케이션 인터페이스 구현 파일
// RTC 시스템과 연동하여 파일 타임스탬프 및 데이터 정렬 처리 수행함
/* USER CODE END Header */
#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  app_time_t t; 
  // 동기화된 RTC 하드웨어로부터 현재 시간 획득
  TimeService_GetNowLocal(&t, 0); 

  unsigned long fattime = 0;
  unsigned yr = (t.year >= 1980) ? (t.year - 1980) : 0;
  
  // FatFs 표준 비트 필드 형식으로 시간 데이터 조립 (총 32비트 잘 구성한 것)
  fattime |= (yr   << 25);
  fattime |= (t.month << 21);
  fattime |= (t.day   << 16);
  fattime |= (t.hour  << 11);
  fattime |= (t.min   << 5);
  fattime |= ((t.sec/2) & 0x1F);
  
  return fattime;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */
