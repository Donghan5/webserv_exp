#!/usr/bin/env python3
import sys
import os
import datetime
import traceback

# 로그 파일 경로 설정 (쓰기 권한이 확실한 곳으로)
LOG_FILE = "/home/donghank/webserv_exp/webserv_test/cgi-bin/debug_echo.log"

def log_message(message):
    """디버그 메시지를 로그 파일에 기록합니다."""
    with open(LOG_FILE, "a") as f:
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"[{timestamp}] {message}\n")

# --- 디버깅 시작 ---
log_message("--- CGI script execution started ---")

try:
    # 1. 파이썬 버전 및 경로 기록
    log_message(f"Python version: {sys.version}")
    log_message(f"Python executable: {sys.executable}")

    # 2. 환경 변수 기록
    log_message("Environment Variables:")
    for key, value in os.environ.items():
        log_message(f"  {key}: {value}")

    # 3. 표준 입력(stdin) 읽기 시도
    log_message("Attempting to read from stdin...")
    body = sys.stdin.read()
    log_message(f"Successfully read {len(body)} bytes from stdin.")
    log_message(f"Body content: {body}")

    # 4. 정상적인 HTTP 응답 생성
    log_message("Generating HTTP response...")
    print("Content-Type: text/plain")
    print(f"Content-Length: {len(body)}")
    print()  # 헤더와 본문 구분
    print(body)
    log_message("HTTP response sent successfully.")

except Exception as e:
    # 5. 예외 발생 시 에러 기록
    log_message("!!! AN ERROR OCCURRED !!!")
    log_message(f"Exception Type: {type(e).__name__}")
    log_message(f"Exception Message: {e}")
    log_message("Traceback:")
    log_message(traceback.format_exc())

    # 실패 시에도 최소한의 HTTP 응답을 시도
    # (이 부분은 클라이언트에게 보이지 않을 수 있지만, 디버깅에 유용)
    print("Content-Type: text/plain")
    print()
    print("CGI script failed. Check the debug log for details.")

log_message("--- CGI script execution finished ---")
