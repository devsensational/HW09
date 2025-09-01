# BullsAndCows (UE5, C++)

멀티플레이 숫자 맞추기(Bulls and Cows) 게임 예제입니다. 채팅 기반 입력으로 숫자를 추측하고, 서버가 판정(S/B/OUT)하여 전 클라이언트에 브로드캐스트합니다. 서버는 플레이어 턴을 30초로 제한하며 타임아웃 시 자동으로 다음 플레이어로 턴을 넘깁니다. 턴이 바뀔 때는 각 클라이언트의 위젯 애니메이션과 초 단위 카운트다운(TextBox)이 재생됩니다.

## 주요 기능
- 채팅 시스템: 클라이언트 → 서버 RPC → GameMode → 전 클라이언트 브로드캐스트(Client RPC)
- 숫자 판정: 3자리(1~9, 중복 없음) 입력 시 S/B/OUT 계산 및 결과 방송
- 턴 기반 진행: 현재 턴인 플레이어만 추측 허용, 30초 제한, 타임아웃 시 자동 전환
- 승리/무승부 처리: 3S 승리 시 공지, 모든 플레이어 시도 소진 시 무승부 → 라운드 리셋
- UI
  - 채팅 입력: Enter로 전송
  - 턴 변경 위젯: 턴 변경 시 애니메이션/초 카운트다운(TextBox)이 모든 클라이언트에서 재생

## 코드 구조
- Source/BullsAndCows/Game/BCGameModeBase.(h|cpp)
  - 서버 권한에서 게임 진행을 총괄
  - 핵심 메서드
    - PrintChatMessageString: 채팅/추측 처리 → 브로드캐스트, 승리/턴 전환
    - GenerateSecretNumber: 비밀 숫자 생성(1~9, 3자리, 중복 없음)
    - IsGuessNumberString, JudgeResult: 숫자 유효성/판정 로직
    - 턴 관리: StartFirstTurnIfNeeded, StartTurnForIndex, AdvanceTurn, OnTurnTimeout
  - 상태
    - SecretNumberString, AllPlayerControllers, CurrentTurnIndex, TurnTimerHandle, TurnTimeLimitSeconds(기본 30)
- Source/BullsAndCows/Game/BCGameStateBase.(h|cpp)
  - 접속 브로드캐스트용 MulticastRPCBroadcastLoginMessage 제공
- Source/BullsAndCows/Player/BCPlayerController.(h|cpp)
  - RPC
    - ServerRPCPrintChatMessageString: 클라→서버 채팅 전달
    - ClientRPCPrintChatMessageString: 서버→클라 메시지 출력
    - ClientRPCOnTurnChanged: 서버→클라 턴 변경 알림(위젯 재생)
  - UI 생성: 채팅 입력, 공지 텍스트, 턴 변경 위젯(TurnChangeWidgetClass/Instance)
- Source/BullsAndCows/Player/BCPlayerState.(h|cpp)
  - PlayerNameString, CurrentGuessCount, MaxGuessCount(기본 3)
  - GetPlayerInfoString: 표시용 시도 횟수 1부터 시작(예: Player1(1/3))
- Source/BullsAndCows/UI/BCChatInput.(h|cpp)
  - Enter로 커밋 시 Owning PlayerController에 SetChatMessageString 호출
- Source/BullsAndCows/UI/BCUITurnChange.(h|cpp)
  - UUserWidget 파생, TextBox(UTextBlock) 바인딩(meta=BindWidget)
  - PlayTurnChange(BlueprintNativeEvent): 기본 구현에서 카운트다운 시작
  - StartCountdown(BlueprintCallable): 외부에서 강제 시작 가능(기본 30초)
  - 초 단위 OnCountdownTick으로 TextBox 숫자 감소, 0 도달 시 타이머 정리
  - NativeDestruct에서 타이머 안전 해제

## 에셋/블루프린트 연결
- BP_BCPlayerController
  - TurnChangeWidgetClass에 UBCUITurnChange를 부모로 하는 WBP_TurnChange 지정
- BP_BCGameMode
  - 필요 시 TurnTimeLimitSeconds 값을 조정(기본 30)

## 빌드/실행
1) Unreal Editor로 BullsAndCows.uproject 열기
2) Play 설정을 Listen Server + 1~2 Clients로 구성
3) 플레이 시작(PIE)
4) 테스트
   - 접속 시 로그인 방송 메시지 확인
   - 턴 공지 메시지와 함께 각 클라이언트의 WBP에서 카운트다운(TextBox 숫자)이 매초 감소하는지 확인
   - 현재 턴이 아닌 클라이언트가 숫자를 보내면 "It's not your turn." 안내가 표시되는지 확인
   - 유효 추측 시 S/B/OUT 결과가 전 클라이언트에 출력되는지 확인
   - 30초 타임아웃 시 자동으로 다음 플레이어로 턴이 넘어가는지 확인
   - 3S 시 라운드 종료/리셋, 무승부 시 "Draw..." 공지 확인

## 네트워킹/RPC 플로우(요약)
- 채팅/추측
  - Client: ABCPlayerController.SetChatMessageString → ServerRPCPrintChatMessageString
  - Server: ABCGameModeBase.PrintChatMessageString → 결과 계산/브로드캐스트
  - Clients: ClientRPCPrintChatMessageString → 화면 출력
- 턴 변경
  - Server: StartTurnForIndex → 각 플레이어에 ClientRPCOnTurnChanged(bIsYourTurn, TurnOwnerName)
  - Clients: UBCUITurnChange.StartCountdown(30) → PlayTurnChange → 카운트다운 표시

## 트러블슈팅
- 채팅/브로드캐스트가 동작하지 않음
  - ABCPlayerController의 ServerRPCPrintChatMessageString가 Server RPC로 선언되어 있는지 확인
  - 멀티플레이 PIE(서버/클라) 환경에서 테스트 여부 확인(싱글 PIE는 RPC 무의미)

## 제한/향후 개선
- 플레이어 이탈 시 턴 인덱스 조정 로직은 기본 정리만 포함(무효 컨트롤러 제거)
- UI/애니메이션은 예시 수준으로, 프로젝트 스타일에 맞춰 확장 필요
- 비밀 숫자 생성/판정 규칙 확장(자리수 가변 등) 가능