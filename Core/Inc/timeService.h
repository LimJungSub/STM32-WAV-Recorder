#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint16_t year;               // 2025니까 8비트론 불충분
  uint8_t  month, day;         // 1-12, 1-31
  uint8_t  hour, min, sec;     // 0-23, 0-59, 0-59
} app_time_t;

#define RTC_VALID_MAGIC 0x5A5A // RTC 초기화 확인용 매직 넘버

void TimeService_Init(void);                         // 이벤트/리소스 생성, RTC 유효성 검사
void TimeService_SetRtcFromAppTime(const app_time_t* t_utc); // NTP 수신 후 호출(UTC 권장)
bool TimeService_WaitReady(uint32_t timeout_ms);     // RTC가 유효할 때까지 대기
void TimeService_GetNowUTC(app_time_t* out);         // RTC 읽기(UTC)
void TimeService_GetNowLocal(app_time_t* out, int8_t tz_offset_hours); // 필요 시 로컬 변환

// FatFs용 (ff/ff.c가 weak 심볼을 찾음): 현재 로컬 시간을 FATTIME 포맷으로 반환
unsigned long get_fattime(void);

#endif
