#include "task_audio.h"
#include "audio_recorder.h"
#include "main.h"

// 외부에서 참조할 큐 핸들 (UiTask에서 파일명 전달용)
extern osMessageQueueId_t audioQueueHandle;

void AudioTask(void *argument)
{
    // AudioCommand_t는 main.h에 정의됨
    AudioCommand_t cmd;
    
    // 오디오 모듈 초기화 수행함
    Audio_Init();

    for(;;)
    {
        // task_network.c에서 전송한 messageQueue 데이터 확인
        if (osMessageQueueGet(audioQueueHandle, &cmd, NULL, 0) == osOK) {
            if (cmd.command == 1) { // 녹음 시작 처리
                Audio_StartRecording(cmd.filename);
            } else { // 녹음 중지 처리
                Audio_StopRecording();
            }
        }

        // 오디오 데이터 처리 로직 실행

        // Audio_Process 내부에서 버퍼 플래그 확인 후 SD 카드 쓰기 작업 수행함
        Audio_Process();

        // 녹음 상태 여부에 따라 태스크 지연 시간 차등 적용함
        if (Audio_GetState() == AUDIO_STATE_RECORDING) {
            // I2S DMA 버퍼 만료 시 즉시 SD 카드 기록 필요함
            // 지연 시간 증가 시 신규 데이터가 기존 버퍼를 덮어쓰는 오버런 현상 발생 가능함
            // 1ms 주기로 Audio_Process 호출하여 버퍼 플래그 실시간 확인 및 처리함
            osDelay(1);
        } else {
            // 대기 중일 때는 처리 속도 하향 조절함 (타 태스크에 CPU 자원 양보 목적)
            osDelay(100); 
        }
    }
}