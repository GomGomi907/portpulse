# PortPulse

PortPulse는 자동화된 하드웨어 테스트를 위한 USB 2.0 재연결 동글입니다.
PC와 DUT(Device Under Test) 사이에 연결하며, 제어 MCU는 계속 연결된 상태로
유지하면서 DUT의 VBUS와 USB D+/D−를 소프트웨어 명령으로 차단하고 복구합니다.

실제 USB 장치를 뽑았다가 다시 연결하는 동작을 자동화 테스트에서 재현하는 것이
주요 목적입니다.

## Rev.C.1 하드웨어

- 4층 PCB, 49.5 mm × 21.6 mm
- USB-C upstream 입력
- USB-A downstream DUT 출력
- USB 2.0 Hub + CH552 제어 MCU
- DUT VBUS 독립 ON/OFF
- DUT D+/D− 독립 연결/차단
- TPS2552 전류 제한 및 Fault 감지
- JLCPCB Economic, Top-side 완전실장 릴리스

## 명령 인터페이스

Rev.C.1 펌웨어는 USB CDC 가상 COM 포트를 제공합니다. 현재 네이티브 C 호스트
프로그램은 Windows와 Linux에서 사용할 수 있습니다.

```text
portpulse --port COM12 status
portpulse --port COM12 on
portpulse --port COM12 off
portpulse --port COM12 --delay-ms 2000 cycle
```

지원 명령:

- `on`: DUT VBUS와 D+/D− 연결
- `off`: DUT VBUS와 D+/D− 차단
- `cycle`: DUT 차단 후 지정 시간 뒤 재연결
- `status`: 현재 연결 상태 확인
- `fault`: 전원 스위치 Fault 상태 확인
- `version`: 펌웨어 버전 확인

현재 Rev.C.1은 단일 DUT 포트이며, 다중 장치 자동 검색과 Serial Number 기반
장치 선택은 다음 호스트 버전에서 추가할 예정입니다.

## 저장소 구성

- `hardware/RevC1/kicad`: 수정 가능한 KiCad 회로도와 PCB 원본
- `hardware/RevC1/jlcpcb-release`: JLCPCB Gerber, BOM, CPL, 제조용 ZIP
- `hardware/RevC1/reports`: ERC/DRC, 라우팅, reset-domain, 제조 패키지 검증 보고서
- `firmware/CH552/release`: 펌웨어 소스, 바이너리, 테스트와 bring-up 문서
- `host/portpulse`: Windows/Linux용 네이티브 C 호스트 프로그램
- `docs`: 설계·라우팅·제조 관련 문서

## 검증 상태

생산 릴리스는 다음 검증을 통과했습니다.

- KiCad ERC/DRC
- BOM/CPL reference set 일치 검사
- USB differential routing 품질 검사
- reset-domain 분석
- Gerber/ZIP/manifest 무결성 검사
- 펌웨어 재현 빌드와 호스트 테스트

실제 제작 후에는 전원, USB enumeration, DUT 재연결 및 100회 반복 테스트가
추가로 필요합니다.

## 라이선스

별도 제3자 고지 파일이 명시하는 경우를 제외하고 하드웨어, 펌웨어, 호스트
프로그램과 문서는 MIT License로 배포합니다.
