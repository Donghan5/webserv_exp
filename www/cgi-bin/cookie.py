#!/usr/bin/env python3

import os
import cgi
import uuid
import time

# directory to store session
SESSION_DIR = "./session_data"

if not os.path.exists(SESSION_DIR):
    os.makedirs(SESSION_DIR)

print("Content-Type: text/html; charset=utf-8")

# to get a cookie
cookies = os.environ.get("HTTP_COOKIE", "")
session_id = None

if cookies:
    for cookie in cookies.split(';'):
        try:
            key, value = cookie.strip().split('=', 1)
            if key == "session_id":
                session_id = value
                break
        except ValueError:
            continue

if not session_id:
    session_id = str(uuid.uuid4())
    expiration_time = time.time() + 60 * 60 * 24 * 30  # expires 30 days
    formatted_time = time.strftime("%a, %d-%b-%Y %H:%M:%S GMT", time.gmtime(expiration_time))
    print(f"Set-Cookie: session_id={session_id}; Expires={formatted_time}; Path=/")

# path session file
session_file = os.path.join(SESSION_DIR, f"{session_id}.txt")

# load visit time
visits = 0
try:
    with open(session_file, "r") as file:
        visits = int(file.read().strip())
except FileNotFoundError:
    visits = 0

# increment and store
visits += 1
with open(session_file, "w") as file:
    file.write(str(visits))

expiration_time = time.time() + 60 * 60 * 24 * 30
formatted_time = time.strftime("%a, %d-%b-%Y %H:%M:%S GMT", time.gmtime(expiration_time))
print(f"Set-Cookie: visits={visits}; Expires={formatted_time}; Path=/")

# print HTML
html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Visit Log</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            text-align: center;
            margin-top: 50px;
        }}
        .container {{
            width: 50%;
            margin: auto;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>Visit Log</h1>
        <p>You've visited <strong>{visits} times</strong>.</p>
    </div>
</body>
</html>
"""

print("\r\n")
print(html_content)
