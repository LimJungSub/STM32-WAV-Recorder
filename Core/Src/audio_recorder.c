#include "audio_recorder.h"
#include <string.h>
#include <stdio.h>
#include "fatfs.h"

extern I2S_HandleTypeDef hi2s2;

// DMA 컨트롤러의 하드웨어 액세스 효율 및 제약 사항 준수를 위해 4바이트 단위로 정렬함 
// 16비트 오디오 샘플 처리 시 메모리 주소 불일치로 인한 전송 에러 방지 목적임
#define PCM_BUFFER_SIZE  4096 
static uint16_t pcm_buffer[PCM_BUFFER_SIZE] __attribute__((aligned(4)));
static volatile bool buf_half_ready = false;
static volatile bool buf_full_ready = false;

static FIL wavFile;
static AudioState_t currentState = AUDIO_STATE_IDLE;
static uint32_t totalDataBytes = 0;

// WAV 파일 규격 정의 구조체
// 리틀 엔디언 형식을 따르며 파일 선두에 위치하는 메타데이터 정보를 보관함
typedef struct {
    char riff[4];
    uint32_t overall_size;
    char wave[4];
    char fmt_chunk_marker[4];
    uint32_t length_of_fmt;
    uint16_t format_type;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byterate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_chunk_header[4];
    uint32_t data_size;
} WavHeader_t;

// 오디오 모듈 및 파일 시스템 초기화
void Audio_Init(void) {
    currentState = AUDIO_STATE_IDLE;
    buf_half_ready = false;
    buf_full_ready = false;

    // SD 카드 내부 컨트롤러의 부팅 및 전압 안정화를 위해 초기 대기 시간 부여함
    osDelay(200);

    // 접촉 불량이나 전원 노이즈로 인한 초기 인식 실패에 대비하여 재시도 로직 구성함
    // 실패 시 즉시 중단하지 않고 일정 간격을 두고 다시 시도하여 시스템 신뢰성 확보함 
    FRESULT res;
    for (int i = 0; i < 3; i++) {
        res = f_mount(&SDFatFS, SDPath, 1);
        if (res == FR_OK) {
            printf("[AUDIO] SD Mount Success (Attempt %d)\r\n", i + 1);
            break;
        }
        printf("[AUDIO] SD Mount Failed (Attempt %d/3). Error: %d. Retrying...\r\n", i + 1, res);
        osDelay(200);
    }
    
    if (res != FR_OK) {
        printf("[AUDIO] SD Mount Gave Up. Check Hardware.\r\n");
    }
}

// WAV 헤더 정보 생성 및 기록
// 녹음 완료 시점에 확정된 실제 데이터 크기를 파일 선두 영역에 반영함
static void WriteWavHeader(uint32_t data_len) {
    WavHeader_t header;
    UINT bw;

    // 표준 PCM 16kHz, 16bit, Mono 포맷에 맞춰 헤더 필드 작성함
    header.riff[0] = 'R'; header.riff[1] = 'I'; header.riff[2] = 'F'; header.riff[3] = 'F';
    header.overall_size = data_len + sizeof(WavHeader_t) - 8;
    header.wave[0] = 'W'; header.wave[1] = 'A'; header.wave[2] = 'V'; header.wave[3] = 'E';
    header.fmt_chunk_marker[0] = 'f'; header.fmt_chunk_marker[1] = 'm'; header.fmt_chunk_marker[2] = 't'; header.fmt_chunk_marker[3] = ' ';
    header.length_of_fmt = 16;
    header.format_type = 1; 
    header.channels = 1;    
    header.sample_rate = 16000;
    header.byterate = 16000 * 16 * 1 / 8; 
    header.block_align = 16 * 1 / 8;      
    header.bits_per_sample = 16;
    header.data_chunk_header[0] = 'd'; header.data_chunk_header[1] = 'a'; header.data_chunk_header[2] = 't'; header.data_chunk_header[3] = 'a';
    header.data_size = data_len;

    // 파일 포인터를 시작점으로 되돌려 예약된 헤더 영역에 데이터 덮어씀
    f_lseek(&wavFile, 0);
    f_write(&wavFile, &header, sizeof(WavHeader_t), &bw);
    printf("[AUDIO] Wav Header Updated. DataLen: %lu\r\n", data_len);
}

// 녹음 프로세스 시작
void Audio_StartRecording(const char* filename) {
    if (currentState != AUDIO_STATE_IDLE) return;

    printf("[AUDIO] Start Recording: %s\r\n", filename);

    // 요청된 파일명으로 새 파일 생성함 실패 시 에러 상태로 전이함
    FRESULT res = f_open(&wavFile, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        printf("[AUDIO] File Open Failed! Error: %d\r\n", res);
        currentState = AUDIO_STATE_ERROR;
        return;
    }

    // 녹음 종료 전까지 데이터 크기를 알 수 없으므로 헤더 영역을 비워두고 기록 시작함
    f_lseek(&wavFile, sizeof(WavHeader_t));
    
    totalDataBytes = 0;
    buf_half_ready = false;
    buf_full_ready = false;

    // I2S 인터페이스와 DMA를 연동하여 CPU 개입 없이 실시간 수신 처리함
    if (HAL_I2S_Receive_DMA(&hi2s2, pcm_buffer, PCM_BUFFER_SIZE) != HAL_OK) {
        printf("[AUDIO] I2S DMA Start Failed!\r\n");
        f_close(&wavFile);
        currentState = AUDIO_STATE_ERROR;
        return;
    }

    currentState = AUDIO_STATE_RECORDING;
}

// 녹음 프로세스 정지 요청
void Audio_StopRecording(void) {
    if (currentState == AUDIO_STATE_RECORDING) {
        // 하드웨어 데이터 수신 중단 및 잔여 버퍼 처리를 위한 상태 전환함
        HAL_I2S_DMAStop(&hi2s2);
        currentState = AUDIO_STATE_STOPPING;
        printf("[AUDIO] Stop Recording Request\r\n");
    }
}

// 오디오 데이터 실시간 처리 루프
// DMA가 채워주는 전/후반부 버퍼 상태를 감시하여 SD 카드에 순차 기록함
void Audio_Process(void) {
    UINT bw;
    
    if (currentState == AUDIO_STATE_RECORDING || currentState == AUDIO_STATE_STOPPING) {
        // 더블 버퍼링 구조를 통해 수신 중단 없이 파일 쓰기 작업 병행함
        if (buf_half_ready) {
            buf_half_ready = false;
            if (f_write(&wavFile, &pcm_buffer[0], PCM_BUFFER_SIZE, &bw) == FR_OK) {
                 totalDataBytes += bw;
            }
        }

        if (buf_full_ready) {
            buf_full_ready = false;
            if (f_write(&wavFile, &pcm_buffer[PCM_BUFFER_SIZE / 2], PCM_BUFFER_SIZE, &bw) == FR_OK) {
                 totalDataBytes += bw;
            }
        }

        // 정지 요청 후 모든 잔여 데이터가 기록 완료되면 종료 단계 진입함
        if (currentState == AUDIO_STATE_STOPPING && !buf_half_ready && !buf_full_ready) {
            // 최종 파일 크기 반영한 헤더 갱신 수행함
            WriteWavHeader(totalDataBytes);
            
            // f_sync 호출을 통해 SD 카드 내부 캐시의 데이터를 물리 영역으로 커밋함
            // 전원 차단 시 발생할 수 있는 파일 시스템 손상 및 데이터 유실 방지 목적임 
            FRESULT res_sync = f_sync(&wavFile);
            FRESULT res_close = f_close(&wavFile);
            
            if (res_sync == FR_OK && res_close == FR_OK) {
                printf("[AUDIO] Recording Completely Stopped. Size: %lu bytes\r\n", totalDataBytes);
                currentState = AUDIO_STATE_IDLE;
            } else {
                printf("[AUDIO] File Close Failed! SyncErr: %d, CloseErr: %d\r\n", res_sync, res_close);
                currentState = AUDIO_STATE_ERROR;
            }
        }
    }
}

// DMA 수신 절반 완료 콜백
void Audio_I2S_HalfCpltCallback(void) {
    buf_half_ready = true;
}

// DMA 수신 전체 완료 콜백
void Audio_I2S_CpltCallback(void) {
    buf_full_ready = true;
}

// 현재 오디오 동작 상태 반환
AudioState_t Audio_GetState(void) {
    return currentState;
}
