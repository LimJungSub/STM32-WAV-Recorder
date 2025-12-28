# PTT/Remote Voice Recorder with Wi-Fi Sync

STM32F411 Nucleo를 기반으로 구현한 하이브리드 IoT 음성 녹음기 프로젝트다. 
물리적인 PTT 버튼뿐만 아니라 웹 인터페이스를 통한 원격 제어를 지원한다. ESP32-S3와의 고속 UART 통신을 통해 녹음 파일의 무선 전송 및 정확한 NTP 시간 동기화를 구현했다.

## 주요 기능

1. **하이브리드 녹음 제어**
   - **PTT 모드**: 기기의 물리 버튼을 누르는 동안 즉시 녹음하고 떼면 저장한다.
   - **원격 모드**: 스마트폰 웹 브라우저에서 녹음 시작 및 중지 버튼을 눌러 원격으로 제어한다.

2. **고속 오디오 처리**
   - **Audio**: I2S 인터페이스와 DMA를 활용한 16kHz 16-bit 모노 녹음.
   - **Buffering**: Double Buffering 구조를 적용해 데이터 유실 없는 연속 녹음을 보장한다.

3. **무선 데이터 관리**
   - **NTP Sync**: 부팅 시 인터넷 표준 시간 서버와 동기화하여 파일 생성 시간을 정확히 기록한다.
   - **Wireless Transfer**: 저장된 WAV 파일을 케이블 연결 없이 Wi-Fi를 통해 스마트폰이나 PC로 다운로드한다.

4. **시스템 아키텍처**
   - **Dual-Core Architecture**: 실시간 제어 및 오디오 처리는 STM32가, 네트워크 스택 및 웹 서버는 ESP32-S3가 전담하여 안정성을 극대화했다.
   - **High-Speed Interlink**: 두 MCU 간 1Mbps 이상의 고속 UART 통신으로 대용량 오디오 데이터를 빠르게 전송한다.

## 하드웨어 구성

- **Main MCU**: STM32F411RET6 (Nucleo-F411RE)
- **Network MCU**: ESP32-S3 DevKit
- **Audio Input**: I2S MEMS Microphone (INMP441)
- **Storage**: Micro SD Card (SDIO 4-bit mode 예정)
- **Display**: 0.96" OLED (SSD1306, I2C)
- **Input**: Tactile Switch

## 소프트웨어 구조

시스템은 FreeRTOS 기반의 멀티태스킹 환경에서 동작하며, 기능별로 태스크가 분리되어 있다.

- **AudioTask**
  - I2S DMA 버퍼 이벤트를 처리하고 SDIO를 통해 SD카드에 데이터를 고속으로 저장한다.
  - 시스템 내 최상위 우선순위를 가진다.
- **NetworkTask**
  - ESP32-S3와 통신하여 NTP 시간 동기화, 웹 명령 수신, 파일 데이터 전송을 수행한다.
  - UART DMA를 활용해 CPU 부하를 줄였다.
- **UiTask**
  - 버튼 입력을 감지하고 OLED 디스플레이에 현재 상태를 갱신한다.

## 개발 환경

- **IDE**: STM32CubeIDE 1.16.0
- **Framework**: STM32 HAL Library, FreeRTOS
- **Network F/W**: Arduino