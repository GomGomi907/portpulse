# PortPulse

## 무엇인가

PortPulse는 PC와 USB 장치(DUT) 사이에 연결하는 USB 2.0 재연결 동글입니다.

PC와 제어 MCU의 연결은 유지한 채, 외부 DUT의 VBUS와 D+/D−를 소프트웨어로
차단하고 복구합니다. 따라서 USB 장치를 실제로 뽑았다가 다시 연결하는 동작을
자동화 테스트에서 재현할 수 있습니다.

## 사용 방법

1. PortPulse의 USB-C upstream 포트를 PC에 연결합니다.
2. DUT를 PortPulse의 USB-A downstream 포트에 연결합니다.
3. 장치 관리자에서 PortPulse의 COM 포트 번호를 확인합니다.
4. 네이티브 `portpulse` 프로그램으로 명령을 보냅니다.

```text
portpulse --port COM12 status
portpulse --port COM12 on
portpulse --port COM12 off
portpulse --port COM12 --delay-ms 2000 cycle
```

주요 명령:

- `on`: DUT 연결
- `off`: DUT 차단
- `cycle`: DUT를 차단한 뒤 지정한 시간 후 다시 연결
- `status`: 현재 상태 확인
- `fault`: 전원 스위치 Fault 상태 확인
- `version`: 펌웨어 버전 확인

현재 Rev.C.1은 DUT 1포트를 지원합니다. 여러 PortPulse를 연결하는 경우에는
각 장치의 COM 포트를 지정해서 사용합니다.

하드웨어·펌웨어·호스트 프로그램은 MIT License로 배포합니다. 제3자 구성요소는
각 구성요소의 고지 파일을 따릅니다.
