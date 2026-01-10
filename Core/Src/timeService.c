#include "timeService.h"
#include "main.h"
#include "cmsis_os.h"

#ifdef HAL_RTC_MODULE_ENABLED
extern RTC_HandleTypeDef hrtc;
#endif

// 시간 준비 완료 여부를 나타내는 내부 플래그
static volatile bool s_time_ready = false;

// RTC 하드웨어 레지스터에서 현재 날짜와 시간을 읽어 구조체에 저장함
static void rtc_read(app_time_t* out) {
#ifdef HAL_RTC_MODULE_ENABLED
  RTC_TimeTypeDef t; RTC_DateTypeDef d;
  // 하드웨어 제약에 따라 시간 읽기 후 날짜를 읽어 동기화 보장함
  HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);
  out->year  = 2000 + d.Year;
  out->month = d.Month;
  out->day   = d.Date;
  out->hour  = t.Hours;
  out->min   = t.Minutes;
  out->sec   = t.Seconds;
#else
  *out = (app_time_t){0};
#endif
}

// 시간 서비스 초기화 및 이전 동기화 기록 확인
// 백업 레지스터 값을 체크하여 시스템 리셋 후에도 시간 유효성 유지함
void TimeService_Init(void) {
#ifdef HAL_RTC_MODULE_ENABLED
  // 이전에 NTP 동기화 성공 시 기록한 매직 넘버 확인함
  uint32_t magic = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
  if (magic == RTC_VALID_MAGIC) {
    s_time_ready = true;
  } else {
    s_time_ready = false; 
  }
#endif
}

// 외부에서 입력받은 시간 정보를 RTC 하드웨어에 설정함
// 설정 완료 후 백업 레지스터에 동기화 마커를 기록함
void TimeService_SetRtcFromAppTime(const app_time_t* t_utc) {
#ifdef HAL_RTC_MODULE_ENABLED
  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};
  
  time.Hours = t_utc->hour; time.Minutes = t_utc->min; time.Seconds = t_utc->sec;
  date.Year  = (uint8_t)(t_utc->year - 2000);
  date.Month = t_utc->month; date.Date = t_utc->day;
  date.WeekDay = RTC_WEEKDAY_MONDAY; 

  HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);

  // 동기화 성공 플래그를 백업 영역에 영구 저장함
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_VALID_MAGIC);
  s_time_ready = true;
#endif
}

// 시간 동기화가 완료될 때까지 지정된 시간 동안 대기함
bool TimeService_WaitReady(uint32_t timeout_ms) {
  uint32_t start = HAL_GetTick();
  while (!s_time_ready) {
    if ((HAL_GetTick() - start) >= timeout_ms) return false;
    osDelay(10);
  }
  return true;
}

// 현재 시간을 UTC 기준으로 조회함
void TimeService_GetNowUTC(app_time_t* out) {
  rtc_read(out);
}

// 시간 값에 오프셋을 더하거나 뺌
// 자정 경계 발생 시 시간 값만 롤오버 처리함
static void add_hours(app_time_t* t, int8_t dh) {
  int h = (int)t->hour + dh;
  while (h < 0)  { h += 24; }
  while (h >= 24){ h -= 24; }
  t->hour = (uint8_t)h;
}

// 특정 시간대 오프셋이 적용된 로컬 시간 획득함
void TimeService_GetNowLocal(app_time_t* out, int8_t tz_offset_hours) {
  rtc_read(out);
  add_hours(out, tz_offset_hours);
}