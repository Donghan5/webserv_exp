#!/usr/bin/env python3

import os
import cgi
import uuid
import time
import sys

# Print content type header first
print("Content-Type: text/html; charset=utf-8\r\n\r\n")

# Debug information
print("<!-- DEBUG: Script started -->")

# directory to store session
SESSION_DIR = "./session_data"

try:
    if not os.path.exists(SESSION_DIR):
        os.makedirs(SESSION_DIR)
except Exception as e:
    print(f"<!-- Error creating session directory: {e} -->")

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

# Simple user database (in real app, use secure storage)
USERS = {
    "admin": "admin123",
    "test": "test123",
    "user": "password"
}

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

# Process logout
if action == "logout":
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
        login_message = "❌ Username or password is incorrect."

# HTML content based on login status
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
            </div>
            <form action="/cgi-bin/login.py" method="get">
                <input type="hidden" name="action" value="logout">
                <button type="submit">Logout</button>
                <button type="button" class="home-btn" onclick="window.location.href='/'">Back to Home</button>
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
                <button type="button" class="home-btn" onclick="window.location.href='/'">Back to Home</button>
            </form>
            <div class="user-note">
                <p><strong>Available test accounts:</strong></p>
                <ul style="text-align: left;">
                    <li>Username: admin, Password: admin123</li>
                    <li>Username: test, Password: test123</li>
                    <li>Username: user, Password: password</li>
                </ul>
            </div>
        </div>
    </body>
    </html>
    """

print(html_content)
