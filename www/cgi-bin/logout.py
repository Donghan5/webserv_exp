#!/usr/bin/env python3

import os
import time

print("Content-Type: text/html; charset=utf-8")

def getUserIdFromCookie():
    cookies = os.environ.get('HTTP_COOKIE', '')
    user_id = None

    if cookies:
        for cookie in cookies.split(';'):
            parts = cookie.strip().split('=', 1)
            if len(parts) == 2:
                name, value = parts
                if name == "id":
                    user_id = value
                    break

    return user_id

def deleteAccount():
    user_id = getUserIdFromCookie()

    if user_id:
        file_path = f"./www/login/database/{user_id}.txt"
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
            except:
                pass

    print("Set-Cookie: id=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")

deleteAccount()

logout_page = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Logout</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap');

        .container {
            --max-width: 400px;
            --padding: 2rem;
            width: min(var(--max-width), 100% - (var(--padding) * 1.2));
            margin-inline: auto;
            background-color: white;
            padding: 2rem;
            border-radius: 15px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            text-align: center;
        }
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            font-family: 'Inter', sans-serif;
            background-color: #f0f0f0;
            margin: 0;
        }
        h1 {
            font-size: 2rem;
            font-weight: 600;
            color: #333;
            margin-bottom: 1rem;
        }
        .message {
            color: #28a745;
            margin-bottom: 1.5rem;
        }
        .button {
            display: inline-block;
            margin-top: 1rem;
            border: none;
            padding: 0.8rem 1.5rem;
            background-color: #333;
            border-radius: 15px;
            font-weight: 500;
            color: #f0f0f0;
            cursor: pointer;
            text-decoration: none;
        }
        .button:hover {
            opacity: 0.9;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Account Deleted</h1>
        <p class="message">Your account has been successfully deleted and you've been logged out.</p>
        <a href="/cgi-bin/login.py" class="button">Login Again</a>
        <a href="/" class="button">Home</a>
    </div>
</body>
</html>
"""

print("\r\n")

print(logout_page)
