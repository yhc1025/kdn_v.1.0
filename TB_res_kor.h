#ifndef __TB_RES_KOR_H__
#define __TB_RES_KOR_H__

#include "TB_resource.h"

char *sp_message_kor[] =
{
	"환경설정을 실행하려면 5초 안에 아무 키를 입력하세요.\n",
	"마스터 암호키가 없습니다.\n암호키가 반드시 설정되어야 합니다.\n아무 키를 입력하세요.\n",
	"현재 암호화 동일합니다.\n",
	"암호가 변경되었습니다.\n",
	"암호를 반드시 변경해야만 합니다.\n",
	"로그인을 5회 실패했습니다.\n5분 동안 로그인을 재시도할 수 없습니다.\n",
	"5분 동안 로그인을 재시도할 수 없습니다.\n",
	"초 남았습니다.\n",
	"5초 후에 재시작합니다. 취소는 아무 키를 누르세요.\n",
	"5초 후에 재시작합니다.\n",
	"모든 계정정보가 초기화됩니다.\n진행하시겠습니까? (Y/N)\n",
	"인버터 환경설정 변경 요청이 수신되었습니다.\n변경 완료 후 장치가 리부팅됩니다.\n",
	"알 수 없음.\n",	
	
	
};

char *sp_res_kor[RESID_MAX] = 
{
	" 1. 설정 (Comm, System, DER)      ",
	" 2. 통신상태                      ",
	" 3. 기기역할                      ",
	" 4. 첫번째 WISUN                  ",
	" 5. 두번째 WISUN                  ",	
	" 6. 이력                          ",
	" 7. 시간설정                      ",
	" 8. 환경설정 초기화               ",
	" 9. 펌웨어 업데이트               ",
	"10. 마스터키 관리                 ",
	"11. 시스템정보                    ",
	"12. 재부팅                        ",
	"13. 관리자 설정                   ",

	" 2. ------------------------------",
	
	////////////////////////////////////////////////////////////////////////////
		
	"           <환경설정>           ",
	"             <설정>             ",
	"           <통신상태>           ",
	"           <기기역할>           ",
	"         <첫번째 WISUN>         ",
	"         <두번째 WISUN>         ",	 
	"              <이력>            ",
	"            <통신이력>          ",
	"            <보안이력>          ",
	"       <통신 로그 자세히>       ",	 
	"       <보안 로그 자세히>       ",
	"          <시간설정>            ",
	"       <환경설정 초기화>        ",
	"       <펌웨어 업데이트>        ",
	"        <마스터키 관리>         ",
	"        <마스터키 편집>         ",
	"       <라이브러리 정보>        ",
	"                                ",
	"          <관리자 설정>         ",
	"          <디버그 모드>         ",
	"           <암호 변경>          ",
	"          <시스템정보>          ",

	"        <관리자 암호 변경>      ",
	"        <사용자 암호 변경>      ",
	"            <로그인>            ",

	////////////////////////////////////////////////////////////////////////////

	"1. 통신이력",
	"2. 보안이력",
	
	"1. 암호입력  ",	
	"1. 현재 암호 ",
	"2. 신규 암호 ",
	"3. 암호 확인 ",

	"마스터 모듈  ",
	"리피터 모듈  ",
	"슬래이브 모듈",

	"그룹 게이트웨이",
	"단말장치-1     ",
	"단말장치-2     ",
	"단말장치-3     ",
	"중계장치-1     ",
	"중계장치-2     ",
	"중계장치-3     ",
	"리피터         ",

	" 4. 첫번째 WISUN (DER-MID연동)    ",
	" 4. 첫번째 WISUN (DER-FRTU연동)   ",
	" 4. 첫번째 WISUN (GROUP G/W연동)  ",
	" 5. 두번째 WISUN (GROUP G/W연동)  ",
	" 5. 리피터                        ",
	" 5. ------------------------------",
	"                                  ",

	"1. 환경설정 초기화 시작",
	"1. 펌웨어 다운로드 시작",

	"1. 활성화       ",
	"2. 동작모드     ",
	"3. 최대 연결[EA]",
	"4. 주파수       ",

	"1. 통신 상태      ",
	"2. 통신 성공률/dBm",

	"1. 마스터키 편집  ",
	"2. 라이브러리 정보",

	"1. 라이브러리 이름",
	"2. 인터페이스 버전",
	"3. 라이브러리 버전",

	"1. 마스터키 수정   ",
	"2. 키 파일 업로드  ",
	"3. 키 파일 다운로드",

	"1. 계정 설정  ",
	"2. 계정 초기화",
	"3. 디버그 모드",
	"1. 관리자 ",
	"2. 사용자 ",

	" 1. 펌웨어 버전  ",
	" 2. 전압 종류    ",
	" 3. EMS 주소     ",
	" 4. 제품 번호    ",
	" 5. 제조일자     ",
	" 6. COT IP 주소  ",
	" 7. RT 포트번호  ",
	" 8. FRTU DNP 주소",
	" 9. EMS 사용     ",
	"10. 세션키 갱신  ",

	" 1. DER-FRTU MODBUS 통신속도  ",
	" 2. Inverter MODBUS 통신속도  ",
	" 3. 인버터 갯수 [EA]          ",
	" 4. 인버터 통신 타임아웃      ",
	" 5. 인터버 통신 재시도 횟수   ",
	" 6. 인버터 읽기 지연시간      ",
	" 7. 인버터 쓰기 지연시간      ",
	" 8. 자동 잠금                 ",
	" 9. 광센서                    ",
	"10. 마스터 통신 모드          ",
	"11. 슬래이브 통신 모드        ",

	////////////////////////////////////////////////////////////////////////////
    
	"부팅 완료                           ",			//	SYSTEM LOG
	"WATCHDOG 리부팅                     ",
	"환경설정 변경                       ",
	"시간 동기화                         ",
	"마지막 동작시간                     ",
	"정상 동작시간                       ",
	"펌웨어 업데이트 실패                ",
	"펌웨어 업데이트 성공                ",
	"환경설정 초기화 실패                ",
	"환경설정 초기화 성공                ",
	"로그인 실패                         ",
	"로그인 성공                         ",
	"관리자 암호 변경                    ",
	"사용자 암호 변경                    ",
	"환경설정 파일 손상                  ",
	"보안로그 파일 손상                  ",
	"통신로그 파일 손상                  ",
	"알 수 없음                          ",

	"통신 연결 성공  Group GW : FRTU     ",			//	COMMUNICATION LOG
	"통신 연결 성공  Group GW : REPEATER ",
	"통신 연결 성공  FRTU     : MID      ",
	"통신 연결 실패  Group GW : FRTU     ",
	"통신 연결 실패  Group GW : REPEATER ",
	"통신 연결 실패  FRTU     : MID      ",
	"데이터 오류     Group GW : MAIN UNIT",
	"무선 데이터 오류                    ",
	"통신장치 연결 상태 변경             ",
	"알 수 없음                          ",

	"상세 정보 없음",								//	Detail for LOG
	"시간 동기화",
	"알 수 없음",
	"동작시간",
	"환경설정 값이 변경되었습니다.",				//	Detail for WATCHDOG
	"WiSUN을 마스터 장치에 연결할 수 없습니다.",
	"마스터 장치에서 ping 응답이 없습니다.",
	"재부팅 명령은 SETUP 프로그램에서 실행되었습니다.",
	"EMS로부터 재부팅 명령을 수신하였습니다.",
	"테스트 모드가 활성화되었습니다.",
	"통신 연결상태 변경",
	"파일 열기 오류",
	"파일 데이터 오류",
	"파일 읽기 오류",
	"파일 쓰기 오류",

	////////////////////////////////////////////////////////////////////////////

	"진입",
	"활성화  ",
	"비활성화",
	"시",
	"분",
	"특고압",
	"저압  ",

	////////////////////////////////////////////////////////////////////////////

	"\"LEFT\"  : 좌측으로 이동",
	"\"RIGHT\" : 우측으로 이동",
	"\"UP\"    : 값 증가",
	"\"DOWN\"  : 값 감소",
	"\"ESC\"   : 복귀",
	"\"ENTER\" : 실행",

	"\"HOME\"  : 첫번째 페이지 이동",
	"\"END\"   : 마지막 페이지 이동",
	"\"LEFT\"  : 이전로 페이지 이동",
	"\"RIGHT\" : 다음로 페이지 이동",
	"\"UP\"    : 위로 이동         ",
	"\"DOWN\"  : 아래로 이동       ",
	"\"ENTER\" : 자세히 보기       ",

	"알 수 없음"

};

#endif//__TB_RES_KOR_H__

