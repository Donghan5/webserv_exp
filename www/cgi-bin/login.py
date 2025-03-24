#!/usr/bin/env python3

import os
import cgi
import uuid
import time
import sys
import json
from datetime import datetime

# NO HTTP header here - let the CGI handler manage it
# print("Content-Type: text/html; charset=utf-8\r\n\r\n")

# Debug information
print("<!-- DEBUG: Script started -->")

# Hardcoded absolute paths as requested
WWW_DIR = "/www"
CGI_BIN_DIR = "/www/cgi-bin"

# Data storage paths
DATA_DIR = os.path.join(WWW_DIR, "data")
SESSION_DIR = os.path.join(DATA_DIR, "session_data")
USERS_FILE = os.path.join(DATA_DIR, "users_data.json")
LOGIN_LOG_DIR = os.path.join(DATA_DIR, "login_logs")

print(f"<!-- DEBUG: Using hardcoded paths: WWW_DIR={WWW_DIR}, CGI_BIN_DIR={CGI_BIN_DIR} -->")
print(f"<!-- DEBUG: DATA_DIR={DATA_DIR} -->")

# Check if directories exist and create them if not
try:
    for directory in [DATA_DIR, SESSION_DIR, LOGIN_LOG_DIR]:
        if not os.path.exists(directory):
            os.makedirs(directory)
            print(f"<!-- DEBUG: Created directory: {directory} -->")
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
                print(f"<!-- DEBUG: Created default users file at {USERS_FILE} -->")
        except Exception as e:
            print(f"<!-- Error creating user file: {e} -->")
            return {}

    try:
        with open(USERS_FILE, "r") as file:
            user_data = json.load(file)
            print(f"<!-- DEBUG: Loaded {len(user_data)} users from {USERS_FILE} -->")
            return user_data
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
            print(f"<!-- DEBUG: Saved login log to {log_file} -->")

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

print(f"<!-- DEBUG: Cookies={cookies} -->")

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
    print(f"<!-- DEBUG: Checking session file {session_file} -->")
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

    # Clear cookies (directly to output)
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

        # Set cookies (directly to output)
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
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>Login Status</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
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
            <h1>Login Status</h1>
            {f'<p class="success">{login_message}</p>' if login_message else ''}
            <div class="user-info">
                <h3>Session Information</h3>
                <p><strong>Username:</strong> {current_username}</p>
                <p><strong>Session ID:</strong> {session_id}</p>
                <p><strong>Login Status:</strong> Logged In</p>
                <p><strong>Session File:</strong> {os.path.join(SESSION_DIR, f"{session_id}.txt")}</p>
            </div>
            <form action="/cgi-bin/login.py" method="get">
                <input type="hidden" name="action" value="logout">
                <button type="submit">Logout</button>
                <button type="button" class="home-btn" onclick="window.location.href='/'">Home</button>
            </form>
        </div>
    </body>
    </html>
    """
else:
    html_content = f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>Login</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
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
            .debug-info {{
                margin-top: 30px;
                font-size: 0.8em;
                color: #6c757d;
                text-align: left;
                padding: 10px;
                background-color: #f8f9fa;
                border-radius: 4px;
                border: 1px dashed #ccc;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>Login</h1>
            {f'<p class="error">{login_message}</p>' if login_message else ''}
            <form action="/cgi-bin/login.py" method="post">
                <div class="form-group">
                    <label for="username">Username:</label>
                    <input type="text" id="username" name="username" required>
                </div>
                <div class="form-group">
                    <label for="password">Password:</label>
                    <input type="password" id="password" name="password" required>
                </div>
                <button type="submit">Login</button>
                <button type="button" class="home-btn" onclick="window.location.href='/'">Home</button>
            </form>
            <div class="user-note">
                <p><strong>Test Accounts:</strong></p>
                <ul style="text-align: left;">
                    <li>Username: admin, Password: admin123</li>
                    <li>Username: test, Password: test123</li>
                    <li>Username: user, Password: password</li>
                </ul>
            </div>
            <div class="debug-info">
                <p><strong>Debug Information:</strong></p>
                <p>Current Directory: {os.getcwd()}</p>
                <p>Using WWW Directory: {WWW_DIR}</p>
                <p>Data Directory: {DATA_DIR}</p>
                <p>Users File: {USERS_FILE}</p>
                <p>Session Directory: {SESSION_DIR}</p>
                <p>Log Directory: {LOGIN_LOG_DIR}</p>
                <p>Session ID: {session_id}</p>
                <p>File Exists: Users={os.path.exists(USERS_FILE)}, Data Dir={os.path.exists(DATA_DIR)}</p>
                <p>Write Permission: Data Dir={os.access(DATA_DIR, os.W_OK) if os.path.exists(DATA_DIR) else False}</p>
            </div>
        </div>
    </body>
    </html>
    """

print(html_content)
