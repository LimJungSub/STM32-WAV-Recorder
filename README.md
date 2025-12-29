# PTT/Remote Voice Recorder with Wi-Fi Sync

STM32F411 Nucleo를 기반으로 구현한 하이브리드 IoT 음성 녹음기 프로젝트다. 
현장에서 즉각적인 사용이 가능한 물리적 PTT 버튼과, 원격 제어가 가능한 웹 인터페이스를 동시에 지원한다. ESP32-S3와의 고속 UART 통신을 도입하여 녹음 파일의 무선 전송 및 정확한 시각 동기화를 구현했다.

## 주요 기능

1. **하이브리드 녹음 제어**
   - **PTT 모드**: 기기의 물리 버튼을 누르는 동안 즉시 오디오를 녹음하고, 버튼을 떼면 파일을 저장하는 직관적인 방식을 제공한다.
   - **원격 모드**: 스마트폰 웹 브라우저 접속을 통해 녹음 시작 및 중지 버튼을 눌러 원격으로 기기를 제어할 수 있다.

2. **고속 오디오 처리**
   - **Audio**: I2S 인터페이스를 통해 16kHz, 16-bit 모노 품질로 오디오를 샘플링한다.
   - **Buffering**: DMA를 활용하여 CPU 개입 없이 데이터를 메모리로 전송하며, Double Buffering 구조를 적용해 녹음 데이터가 끊기거나 유실되는 현상을 방지했다.

3. **무선 데이터 관리**
   - **NTP Sync**: 부팅 시 Wi-Fi를 통해 표준 시간 서버에 접속, 현재 시각을 받아와 파일 생성 시간을 정확히 기록한다.
   - **Wireless Transfer**: 저장된 WAV 파일을 SD카드 분리 없이 Wi-Fi를 통해 스마트폰이나 PC로 고속 다운로드할 수 있다.

4. **시스템 아키텍처**
   - **Dual-Core Architecture**: 실시간 제어 및 오디오 처리는 STM32가 전담하고, 무거운 네트워크 스택 및 웹 서버는 ESP32-S3가 전담하도록 분리하여 구현했다.
   - **High-Speed Interlink**: 두 MCU 간 1Mbps 이상의 고속 UART 통신을 구축하여 대용량 오디오 데이터를 지연 없이 전송한다.

## 하드웨어 구성

- **Main MCU**: STM32F411RET6 (Nucleo-F411RE)
- **Network MCU**: ESP32-S3 DevKit
- **Audio Input**: I2S MEMS Microphone (INMP441)
- **Storage**: Micro SD Card - Samsung pro plus 256gb (SDIO 4-bit mode 또는 USB MSC 방식 적용 예정)
- **Display**: 0.96" OLED (SSD1306, I2C)
- **Input**: Tactile Switch

## 소프트웨어 구조 (FreeRTOS)

시스템은 FreeRTOS 기반의 멀티태스킹 환경에서 동작하며, 기능별로 태스크가 독립적으로 분리되어 있다.

- **AudioTask**
  - I2S DMA 버퍼 이벤트를 처리하고 파일 시스템을 통해 데이터를 저장한다. 실시간성을 위해 가장 높은 우선순위를 가진다.
- **NetworkTask**
  - ESP32-S3와 통신하여 NTP 시간 동기화, 웹 명령 수신, 파일 데이터 전송을 수행한다. UART DMA를 활용해 CPU 부하를 최소화했다.
- **UiTask**
  - 버튼 입력의 디바운싱을 처리하고 OLED 디스플레이에 현재 기기 상태(녹음 중, Wi-Fi 연결 등)를 시각화한다.

## 개발 환경

- **IDE**: STM32CubeIDE 1.16.0
- **Framework**: STM32 HAL Library, FreeRTOS
- **Network F/W**: Arduino (ESP32-S3)