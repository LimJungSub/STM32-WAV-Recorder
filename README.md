# STM32 PTT Voice Recorder

STM32F411 MCU를 기반으로 구현한 PTT(Push-to-Talk) 방식의 디지털 음성 녹음기 프로젝트다. 버튼을 누르고 있는 동안 마이크 입력을 받아 WAV 파일로 변환하며, SD 카드에 저장한다. ESP-01S Wi-Fi 모듈을 이용해 NTP 서버와 시간을 동기화함으로써 녹음 파일에 정확한 생성 시간을 기록하는 것이 특징이다.

## 주요 기능

1. **PTT 녹음 (Push-to-Talk)**
   - 물리 버튼을 누르는 동안 오디오를 녹음하고 버튼을 떼면 파일로 저장한다.
   - 포맷: RIFF WAVE (16kHz, 16-bit, Mono).

2. **고속 데이터 처리**
   - I2S 인터페이스와 DMA(Direct Memory Access)를 활용하여 CPU 부하를 최소화했다.
   - Double Buffering 구조를 적용해 녹음 중 데이터 유실이나 끊김을 방지했다.

3. **시간 동기화 (NTP Sync)**
   - 부팅 초기 ESP-01S 모듈이 Wi-Fi에 접속하여 NTP 서버로부터 UTC 시간을 받아온다.
   - FatFs 파일 시스템에 정확한 파일 생성/수정 시간을 반영한다.

4. **운영체제 (RTOS)**
   - FreeRTOS를 적용하여 오디오 처리, UI 제어, 네트워크 통신 태스크를 분리했다.

## 하드웨어 구성

- **MCU**: STM32F411RET6 (Nucleo-F411RE)
- **Audio Input**: I2S MEMS Microphone (INMP441 등)
- **Storage**: Micro SD Card (SDIO 4-bit mode)
- **Network**: ESP-01S (ESP8266, UART 연결)
- **Display**: 0.96" OLED (SSD1306, I2C)
- **Input**: PTT Push Button

## 소프트웨어 아키텍처

시스템은 FreeRTOS 기반의 멀티태스킹 환경에서 동작한다.

- **AudioTask (High Priority)**
  - I2S DMA로부터 수신된 PCM 데이터를 SD 카드로 스트리밍한다.
  - 실시간성 보장을 위해 최상위 우선순위를 가진다.
- **UiTask (Normal Priority)**
  - PTT 버튼의 상태(Debouncing 포함)를 감지하고 OLED 화면을 갱신한다.
  - 녹음 시작/중지 이벤트를 AudioTask로 전달한다.
- **EspNtpTask (Normal Priority)**
  - 시스템 초기화 시 AT 커맨드로 시간을 동기화하고 RTC를 갱신한 뒤 대기 상태로 전환한다.
