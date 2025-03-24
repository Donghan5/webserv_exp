#!/usr/bin/env python3

import os
import time
import sys


# directory to store session
SESSION_DIR = "./session_data"

# expire the cookies
session_id = None
cookies = os.environ.get("HTTP_COOKIE", "")

if cookies:
    for cookie in cookies.split(';'):
        try:
            key, value = cookie.strip().split('=', 1)
            if key == "session_id":
                session_id = value
                # Delete session file if exists
                if session_id:
                    session_file = os.path.join(SESSION_DIR, f"{session_id}.txt")
                    if os.path.exists(session_file):
                        os.remove(session_file)
                break
        except ValueError:
            continue

# Set expired cookies
print("Set-Cookie: session_id=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")
print("Set-Cookie: visits=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")
print("Set-Cookie: username=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")
print("Set-Cookie: logged_in=false; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")

html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Cookies Reset</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin-top: 50px;
            background-color: #f8f9fa;
        }
        .container {
            width: 50%;
            margin: auto;
            padding: 20px;
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .success {
            color: #28a745;
            font-weight: bold;
        }
        button {
            background-color: #007bff;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            margin-top: 20px;
        }
        button:hover {
            background-color: #0069d9;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Cookies Reset</h1>
        <p class="success">✅ All cookies have been successfully reset!</p>
        <p>Your session information has been cleared.</p>
        <button onclick="window.location.href='/'">Back to Home</button>
    </div>
</body>
</html>
"""

print(html_content)
