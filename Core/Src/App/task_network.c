#include "task_network.h"
#include "main.h"
#include "timeService.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart1;
extern osMessageQueueId_t audioQueueHandle;

// AudioCommand_t는 main.h에 정의됨

// ESP32와 통신하여 시간 정보를 수신하고 웹 명령을 처리함
void EspNtpTask(void *argument)
{
    uint8_t rx_buf[128];
    HAL_StatusTypeDef status;

    printf("[STM] Booting Network Task...\r\n");

    // ESP32 부팅 및 시스템 안정화 대기 시간
    osDelay(5000); 

    // UART 주변장치 재설정을 통한 통신 노이즈 제거
    __HAL_UART_DISABLE(&huart1);
    osDelay(10);
    __HAL_UART_ENABLE(&huart1);
    
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_NEFLAG(&huart1);
    __HAL_UART_CLEAR_FEFLAG(&huart1);
    
    // 수신 버퍼에 남아있는 비정상 데이터를 강제로 비움
    uint8_t dummy;
    uint32_t flush_start = HAL_GetTick();
    while (HAL_UART_Receive(&huart1, &dummy, 1, 0) == HAL_OK) {
        if (HAL_GetTick() - flush_start > 100) break; 
    }

    // --- NTP 시간 동기화 섹션 ---
    printf("[STM] Requesting Time...\r\n");
    
    int retry_count = 0;
    while (retry_count < 5) {
        // 시간 정보 요청 문자열 전송
        uint8_t req = 'T';
        HAL_UART_Transmit(&huart1, &req, 1, 100);

        memset(rx_buf, 0, sizeof(rx_buf));
        status = HAL_UART_Receive(&huart1, rx_buf, sizeof(rx_buf) - 1, 1500);

        // 수신 데이터에서 시간 키워드 탐색 및 파싱
        char* p = strstr((char*)rx_buf, "TIME:");
        if (p) {
            printf("[STM] Raw Time String: %s", p);
            p += 5; 
            
            int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
            int parsed = sscanf(p, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);

            // 년, 월, 일, 시, 분, 초가 모두 정확히 파싱된 경우만 RTC 설정함
            if (parsed == 6) { 
                app_time_t t = { (uint16_t)year, (uint8_t)month, (uint8_t)day, (uint8_t)hour, (uint8_t)min, (uint8_t)sec };
                TimeService_SetRtcFromAppTime(&t);
                printf("[STM] RTC Synced: %04d-%02d-%02d %02d:%02d:%02d\r\n", 
                        t.year, t.month, t.day, t.hour, t.min, t.sec);
                break; 
            } else {
                printf("[STM] Partial/Invalid Time Received (Parsed: %d). Retrying...\r\n", parsed);
            }
        } else {
            printf("[STM] No Response. Retrying (%d/5)...\r\n", retry_count + 1);
        }
        
        retry_count++;
        osDelay(500); 
    }

    if (retry_count == 5) {
        printf("[STM] NTP Sync Failed. Using Default Time (00:00:00).\r\n");
    }

    (void)status; // 미사용 변수 컴파일 경고 무시

    // --- 웹 제어 명령 수신 섹션 ---
    printf("[STM] Listening for Web Commands...\r\n");
    
    AudioCommand_t web_cmd;
    uint8_t cmd_byte;

    while(1) {
        // ESP32로부터 1바이트 명령 수신 대기
        if (HAL_UART_Receive(&huart1, &cmd_byte, 1, 100) == HAL_OK) {
            if (cmd_byte == 'R') {
                // 원격 녹음 시작 명령 수신 시 처리
                printf("[STM] CMD: REC START\r\n");
                
                app_time_t now;
                TimeService_GetNowLocal(&now, 0); 

                // 현재 시각을 반영한 파일명 생성 및 큐 전송
                web_cmd.command = 1;
                snprintf(web_cmd.filename, 64, "remoterec_%04d%02d%02d_%02d%02d%02d.wav", 
                         now.year, now.month, now.day, now.hour, now.min, now.sec);
                
                printf("[STM] Target Filename: %s\r\n", web_cmd.filename);

                if (osMessageQueuePut(audioQueueHandle, &web_cmd, 0, 0) != osOK) {
                    printf("[STM] Queue Full!\r\n");
                }
            }
            else if (cmd_byte == 'S') {
                // 원격 녹음 정지 명령 수신 시 처리
                printf("[STM] CMD: REC STOP\r\n");
                
                web_cmd.command = 0;
                memset(web_cmd.filename, 0, sizeof(web_cmd.filename)); 
                
                osMessageQueuePut(audioQueueHandle, &web_cmd, 0, 0);
            }
        }
        osDelay(10); // 타 태스크에 실행 권한 양보를 위한 최소 지연
    }
}
