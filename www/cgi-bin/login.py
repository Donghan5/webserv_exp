#!/usr/bin/env python3

import os
import cgi
import uuid
import time
import sys
import json
from datetime import datetime

# Print content type header first
print("Content-Type: text/html; charset=utf-8\r\n\r\n")

# Debug information
print("<!-- DEBUG: Script started -->")

# Directory settings
SESSION_DIR = "./session_data"
USERS_FILE = "./users_data.json"
LOGIN_LOG_DIR = "./login_logs"

# Check if directories exist and create them if not
try:
    for directory in [SESSION_DIR, LOGIN_LOG_DIR]:
        if not os.path.exists(directory):
            os.makedirs(directory)
except Exception as e:
    print(f"<!-- Error creating directories: {e} -->")

# Load user data from JSON file
def load_users():
    if not os.path.exists(USERS_FILE):
        # Create default users if the file doesn't exist
        default_users = {
            "admin": "admin123",
            "test": "test123",
            "user": "password"
        }
        try:
            with open(USERS_FILE, "w") as file:
                json.dump(default_users, file, indent=4)
        except Exception as e:
            print(f"<!-- Error creating user file: {e} -->")
            return {}

    try:
        with open(USERS_FILE, "r") as file:
            return json.load(file)
    except Exception as e:
        print(f"<!-- Error loading user file: {e} -->")
        return {}

# Save login activity log
def save_login_log(username, success=True, ip_address=None):
    try:
        # Create log filename with today's date
        today = datetime.now().strftime("%Y-%m-%d")
        log_file = os.path.join(LOGIN_LOG_DIR, f"login_log_{today}.txt")

        # Get current time and IP address
        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        if ip_address is None:
            ip_address = os.environ.get("REMOTE_ADDR", "unknown")

        # Format log message
        log_message = f"{current_time} | {username} | {'SUCCESS' if success else 'FAILED'} | {ip_address}\n"

        # Append to log file
        with open(log_file, "a") as file:
            file.write(log_message)

    except Exception as e:
        print(f"<!-- Error saving login log: {e} -->")

# Load user data
USERS = load_users()

# Get form data
try:
    form = cgi.FieldStorage()
    username = form.getvalue("username", "")
    password = form.getvalue("password", "")
    action = form.getvalue("action", "login")
    print(f"<!-- DEBUG: Action={action}, Username={username} -->")
except Exception as e:
    print(f"<!-- Error processing form data: {e} -->")
    username = ""
    password = ""
    action = "login"

# Check if we have a session already
session_id = None
cookies = os.environ.get("HTTP_COOKIE", "")
logged_in = False
current_username = ""

if cookies:
    for cookie in cookies.split(';'):
        try:
            key, value = cookie.strip().split('=', 1)
            if key == "session_id":
                session_id = value
            elif key == "username":
                current_username = value
            elif key == "logged_in" and value == "true":
                logged_in = True
        except ValueError:
            continue

# Validate session if session_id exists
if session_id:
    session_file = os.path.join(SESSION_DIR, f"{session_id}.txt")
    if os.path.exists(session_file):
        try:
            with open(session_file, "r") as file:
                session_data = {}
                for line in file:
                    if '=' in line:
                        key, value = line.strip().split('=', 1)
                        session_data[key] = value

                if 'username' in session_data:
                    current_username = session_data['username']
                    logged_in = True
        except Exception as e:
            print(f"<!-- Error reading session file: {e} -->")
    else:
        # Invalidate session if file doesn't exist
        logged_in = False
        current_username = ""

# Process logout
if action == "logout" and logged_in:
    # Log the logout
    save_login_log(current_username, success=True, ip_address=os.environ.get("REMOTE_ADDR", "unknown"))

    # Clear session
    if session_id:
        session_file = os.path.join(SESSION_DIR, f"{session_id}.txt")
        if os.path.exists(session_file):
            os.remove(session_file)

    # Clear cookies
    print("Set-Cookie: session_id=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")
    print("Set-Cookie: username=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")
    print("Set-Cookie: logged_in=false; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")

    logged_in = False
    current_username = ""

# Process login
login_message = ""
if action == "login" and username and password:
    if username in USERS and USERS[username] == password:
        # Log successful login
        save_login_log(username, success=True)

        # Successful login
        session_id = str(uuid.uuid4())
        expiration_time = time.time() + 60 * 60 * 24  # expires in 24 hours
        formatted_time = time.strftime("%a, %d-%b-%Y %H:%M:%S GMT", time.gmtime(expiration_time))

        # Set cookies
        print(f"Set-Cookie: session_id={session_id}; Expires={formatted_time}; Path=/")
        print(f"Set-Cookie: username={username}; Expires={formatted_time}; Path=/")
        print(f"Set-Cookie: logged_in=true; Expires={formatted_time}; Path=/")

        # Save session info
        try:
            session_file = os.path.join(SESSION_DIR, f"{session_id}.txt")
            with open(session_file, "w") as file:
                file.write(f"username={username}\nlogin_time={time.time()}")
        except Exception as e:
            print(f"<!-- Error writing session file: {e} -->")

        logged_in = True
        current_username = username
        login_message = f"✅ Login successful! Welcome, {username}!"
    else:
        # Log failed login attempt
        save_login_log(username, success=False)
        login_message = "❌ Username or password is incorrect."

# Generate HTML content
if logged_in:
    html_content = f"""
    <!DOCTYPE html>
    <html lang="ko">
    <head>
        <meta charset="UTF-8">
        <title>Login Status</title>
        <style>
            body {{
                font-family: 'Malgun Gothic', sans-serif;
                text-align: center;
                margin-top: 50px;
                background-color: #f8f9fa;
            }}
            .container {{
                width: 50%;
                margin: auto;
                padding: 20px;
                background-color: white;
                border-radius: 8px;
                box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            }}
            .user-info {{
                background-color: #e9ecef;
                padding: 15px;
                border-radius: 5px;
                margin: 20px 0;
                text-align: left;
            }}
            .success {{
                color: #28a745;
                font-weight: bold;
            }}
            button {{
                background-color: #dc3545;
                color: white;
                border: none;
                padding: 10px 20px;
                border-radius: 4px;
                cursor: pointer;
                margin-top: 10px;
            }}
            button:hover {{
                background-color: #c82333;
            }}
            .home-btn {{
                background-color: #007bff;
                margin-left: 10px;
            }}
            .home-btn:hover {{
                background-color: #0069d9;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>로그인 상태</h1>
            {f'<p class="success">{login_message}</p>' if login_message else ''}
            <div class="user-info">
                <h3>세션 정보</h3>
                <p><strong>사용자:</strong> {current_username}</p>
                <p><strong>세션 ID:</strong> {session_id}</p>
                <p><strong>로그인 상태:</strong> 로그인됨</p>
            </div>
            <form action="/cgi-bin/login.py" method="get">
                <input type="hidden" name="action" value="logout">
                <button type="submit">로그아웃</button>
                <button type="button" class="home-btn" onclick="window.location.href='/'">홈으로</button>
            </form>
        </div>
    </body>
    </html>
    """
else:
    html_content = f"""
    <!DOCTYPE html>
    <html lang="ko">
    <head>
        <meta charset="UTF-8">
        <title>Login</title>
        <style>
            body {{
                font-family: 'Malgun Gothic', sans-serif;
                text-align: center;
                margin-top: 50px;
                background-color: #f8f9fa;
            }}
            .container {{
                width: 50%;
                margin: auto;
                padding: 20px;
                background-color: white;
                border-radius: 8px;
                box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            }}
            .form-group {{
                margin-bottom: 15px;
                text-align: left;
            }}
            label {{
                display: block;
                margin-bottom: 5px;
                font-weight: bold;
            }}
            input {{
                width: 100%;
                padding: 8px;
                border: 1px solid #ced4da;
                border-radius: 4px;
            }}
            .error {{
                color: #dc3545;
                margin-top: 10px;
            }}
            button {{
                background-color: #007bff;
                color: white;
                border: none;
                padding: 10px 20px;
                border-radius: 4px;
                cursor: pointer;
                margin-top: 10px;
            }}
            button:hover {{
                background-color: #0069d9;
            }}
            .home-btn {{
                background-color: #6c757d;
                margin-left: 10px;
            }}
            .home-btn:hover {{
                background-color: #5a6268;
            }}
            .user-note {{
                margin-top: 20px;
                font-size: 0.9em;
                color: #6c757d;
                text-align: left;
                padding: 10px;
                background-color: #f8f9fa;
                border-radius: 4px;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>로그인</h1>
            {f'<p class="error">{login_message}</p>' if login_message else ''}
            <form action="/cgi-bin/login.py" method="post">
                <div class="form-group">
                    <label for="username">사용자 이름:</label>
                    <input type="text" id="username" name="username" required>
                </div>
                <div class="form-group">
                    <label for="password">비밀번호:</label>
                    <input type="password" id="password" name="password" required>
                </div>
                <button type="submit">로그인</button>
                <button type="button" class="home-btn" onclick="window.location.href='/'">홈으로</button>
            </form>
            <div class="user-note">
                <p><strong>테스트 계정:</strong></p>
                <ul style="text-align: left;">
                    <li>사용자 이름: admin, 비밀번호: admin123</li>
                    <li>사용자 이름: test, 비밀번호: test123</li>
                    <li>사용자 이름: user, 비밀번호: password</li>
                </ul>
            </div>
        </div>
    </body>
    </html>
    """

print(html_content)
