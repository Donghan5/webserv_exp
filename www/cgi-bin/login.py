#!/usr/bin/env python3

import os
import cgitb
import uuid
import cgi
import time
import urllib.parse
import json
import sys

# Enable debugging - Show detailed error information
cgitb.enable()

# Log message function
def log_message(message):
    with open("./www/login/log.txt", "a") as log_file:
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
        log_file.write(f"[{timestamp}] {message}\n")

# Debug POST data function
def debug_post_data():
    form = cgi.FieldStorage()
    debug_info = {}

    # Collect all form data
    for key in form.keys():
        debug_info[key] = form.getvalue(key)

    # Collect important information from environment variables
    debug_info["REQUEST_METHOD"] = os.environ.get("REQUEST_METHOD", "")
    debug_info["CONTENT_TYPE"] = os.environ.get("CONTENT_TYPE", "")
    debug_info["CONTENT_LENGTH"] = os.environ.get("CONTENT_LENGTH", "")

    # Log the information
    log_message(f"POST data: {json.dumps(debug_info)}")
    return debug_info

isNewClient = False

def generateId():
    return str(uuid.uuid4())

def generateExpirationDate():
    expiration_time = time.time() + 60 * 60 * 24 * 30
    formatted_time = time.strftime("%a, %d-%b-%Y %H:%M:%S GMT", time.gmtime(expiration_time))
    return formatted_time

def createNewCookie():
    global isNewClient
    isNewClient = True
    form = cgi.FieldStorage()

    # POST data debugging and logging
    post_data = debug_post_data()

    name = form.getvalue("username")
    password = form.getvalue("password")

    # Add input validation
    validation_errors = []
    if name is None or name.strip() == "":
        validation_errors.append("Username is required")
    if password is None or password.strip() == "":
        validation_errors.append("Password is required")

    if validation_errors:
        log_message(f"Validation errors: {', '.join(validation_errors)}")
        show_error_page(validation_errors)
        return None

    isNewClient = False
    user_id = generateId()
    expiration_date = generateExpirationDate()

    # Set cookie
    print(f"Set-Cookie: id={user_id}; Expires={expiration_date}; Path=/\r\n")

    # Check database directory
    if not os.path.exists("./www/login/database"):
        os.makedirs("./www/login/database")

    # Save user information
    with open(f"./www/login/database/{user_id}.txt", "w") as file:
        file.write(f"{name}\n{password}")

    log_message(f"New user created: {name}, ID: {user_id}")
    return user_id

def saveNote():
    form = cgi.FieldStorage()

    # POST data debugging and logging
    post_data = debug_post_data()

    note = form.getvalue("note")

    if note is None:
        log_message("Note save failed: No note data")
        return {"success": False, "message": "No note data provided"}

    user_id = getUserIdFromCookie()
    if user_id is None:
        log_message("Note save failed: No user ID")
        return {"success": False, "message": "Login required"}

    file_path = f"./www/login/database/{user_id}.txt"
    if not os.path.exists(file_path):
        log_message(f"Note save failed: File not found - {file_path}")
        return {"success": False, "message": "User information not found"}

    try:
        with open(file_path, "r") as file:
            lines = file.readlines()
            if len(lines) < 2:
                log_message(f"Note save failed: Invalid file format - {file_path}")
                return {"success": False, "message": "Invalid user data format"}
            username = lines[0].strip()
            password = lines[1].strip()

        with open(file_path, "w") as file:
            file.write(f"{username}\n{password}\n{note}")

        log_message(f"Note saved successfully: User {username}")
        return {"success": True, "message": "Note saved successfully"}
    except Exception as e:
        log_message(f"Error during note save: {str(e)}")
        return {"success": False, "message": f"Error during save: {str(e)}"}

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

def getUserInfo(user_id):
    file_path = f"./www/login/database/{user_id}.txt"

    if not os.path.exists(file_path):
        log_message(f"User info load failed: File not found - {file_path}")
        return None

    try:
        with open(file_path, "r") as file:
            lines = file.readlines()
            if len(lines) < 2:
                log_message(f"User info load failed: Invalid file format - {file_path}")
                return None
            username = lines[0].strip()
            password = lines[1].strip()
            if len(lines) > 2:
                note = lines[2].strip()
            else:
                note = ""
        return {"username": username, "password": password, "note": note}
    except Exception as e:
        log_message(f"Error during user info load: {str(e)}")
        return None

def show_error_page(errors):
    error_list = "</li><li>".join(errors)
    html = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Error Occurred</title>
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
            color: #dc2626;
            text-align: center;
            margin-bottom: 1rem;
        }
        ul {
            margin-bottom: 1.5rem;
        }
        .button {
            display: block;
            width: 100%;
            margin-top: 1rem;
            border: none;
            padding: 1rem;
            background-color: #333;
            border-radius: 15px;
            font-weight: semibold;
            color: #f0f0f0;
            cursor: pointer;
            text-align: center;
            text-decoration: none;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Error Occurred</h1>
        <ul>
            <li>""" + error_list + """</li>
        </ul>
        <a href="./" class="button">Try Again</a>
    </div>
</body>
</html>"""
    print("\r\n")
    print(html)

# API response function
def handle_api_request():
    # Handle POST request
    if os.environ.get("REQUEST_METHOD") == "POST":
        form = cgi.FieldStorage()

        # saveNote API call
        if form.getvalue("note") is not None:
            result = saveNote()
            # JSON format response
            print("Content-Type: application/json\r\n\r\n")
            print(json.dumps(result))
            return True

    return False

# Create login page HTML
def get_login_page_html():
    request_method = os.environ.get("REQUEST_METHOD", "")
    content_type = os.environ.get("CONTENT_TYPE", "")

    return """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Login Page</title>
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
            text-align: center;
            margin-bottom: 1rem;
        }
        form {
            display: flex;
            flex-direction: column;
        }
        label {
            font-size: 1rem;
            margin-bottom: 0.5rem;
            color: #333;
        }
        input[type="text"],
        input[type="password"] {
            padding: 0.8rem;
            font-size: 1rem;
            margin-bottom: 1rem;
            border-radius: 10px;
            border: 1px solid #ccc;
        }
        .button {
            margin-top: 1rem;
            border: none;
            padding: 1rem;
            background-color: #333;
            border-radius: 15px;
            font-weight: semibold;
            color: #f0f0f0;
            cursor: pointer;
        }
        .debug-info {
            margin-top: 2rem;
            padding: 1rem;
            background-color: #f9f9f9;
            border-radius: 10px;
            border: 1px solid #ddd;
            font-size: 0.9rem;
        }
        .debug-toggle {
            cursor: pointer;
            color: #333;
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Login</h1>
        <form action="./" method="post" id="loginForm">
            <label for="username">Username</label>
            <input type="text" id="username" name="username" required>
            <label for="password">Password</label>
            <input type="password" id="password" name="password" required>
            <button class="button" type="submit">Login</button>
        </form>

        <div class="debug-toggle" onclick="toggleDebug()">Show Debug Info</div>
        <div class="debug-info" id="debugInfo" style="display: none;">
            <h3>Request Info</h3>
            <p>Request Method: """ + request_method + """</p>
            <p>Content Type: """ + content_type + """</p>
            <pre id="debugData">POST data will be displayed here when form is submitted.</pre>
        </div>
    </div>

    <script>
        function toggleDebug() {
            const debugInfo = document.getElementById('debugInfo');
            if (debugInfo) {
                if (debugInfo.style.display === 'none' || !debugInfo.style.display) {
                    debugInfo.style.display = 'block';
                } else {
                    debugInfo.style.display = 'none';
                }
            }
        }

        document.getElementById('loginForm').addEventListener('submit', function(e) {
            // Form data collection
            const formData = new FormData(this);
            const debugData = {};

            for (const [key, value] of formData.entries()) {
                debugData[key] = value;
            }

            // Update debug info (hide password)
            if (debugData.password) {
                debugData.password = '******';
            }

            document.getElementById('debugData').textContent = JSON.stringify(debugData, null, 2);
        });
    </script>
</body>
</html>"""

# Create user page HTML
def get_user_page_html(userInfo):
    return """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>User Page</title>
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
            display: flex;
            flex-direction: column;
            align-items: center;
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
            text-align: center;
            margin-bottom: 1rem;
        }
        .button {
            font-size: .8rem;
            margin-top: 1rem;
            border: none;
            padding: 1rem;
            background-color: #333;
            border-radius: 15px;
            font-weight: semibold;
            color: #f0f0f0;
            cursor: pointer;
        }

        .button:hover {
            opacity: 0.9;
        }

        .delete {
            background-color: #dc2626;
        }

        .buttons {
            display: flex;
            justify-content: center;
            gap: 1rem;
        }

        .link {
            text-decoration: none;
            color: #f0f0f0;
        }

        .success-message, .error-message {
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            width: 100%;
            text-align: center;
            display: none;
        }

        .success-message {
            background-color: #10b981;
            color: white;
        }

        .error-message {
            background-color: #ef4444;
            color: white;
        }

        .debug-info {
            margin-top: 1rem;
            padding: 1rem;
            background-color: #f9f9f9;
            border-radius: 10px;
            border: 1px solid #ddd;
            font-size: 0.9rem;
            width: 100%;
            display: none;
        }

        .debug-toggle {
            margin-top: 1rem;
            cursor: pointer;
            color: #333;
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Welcome, """ + userInfo['username'] + """</h1>
        <p>Your password: """ + userInfo['password'] + """</p>
        <textarea id="note" name="note" rows="4" cols="50">""" + userInfo['note'] + """</textarea>

        <div id="successMessage" class="success-message">Note saved successfully!</div>
        <div id="errorMessage" class="error-message">Error occurred while saving.</div>

        <div class="buttons">
            <button class="button" onclick="saveNote()">Save Note</button>
            <a class="link" href="/"><div class="button">Home</div></a>
            <button class="button delete" onclick="window.location.href = './logout.py'">Delete Account</button>
        </div>

        <div class="debug-toggle" onclick="toggleDebug()">Show Debug Info</div>
        <div class="debug-info" id="debugInfo">
            <h3>POST Request Info</h3>
            <pre id="requestInfo">Request information will be displayed here.</pre>
            <h3>Server Response</h3>
            <pre id="responseInfo">Response information will be displayed here.</pre>
        </div>
    </div>
    <script>
        function toggleDebug() {
            const debugInfo = document.getElementById('debugInfo');
            if (debugInfo) {
                if (debugInfo.style.display === 'none' || !debugInfo.style.display) {
                    debugInfo.style.display = 'block';
                } else {
                    debugInfo.style.display = 'none';
                }
            }
        }

        function saveNote() {
            const note = document.getElementById("note").value;
            const requestInfo = document.getElementById("requestInfo");
            const responseInfo = document.getElementById("responseInfo");
            const successMessage = document.getElementById("successMessage");
            const errorMessage = document.getElementById("errorMessage");

            // Display request data in debug info
            const requestData = { note: note };
            requestInfo.textContent = JSON.stringify(requestData, null, 2);

            // Note save request
            fetch("./login.py", {
                method: "POST",
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded'
                },
                body: new URLSearchParams({
                    'note': note
                })
            })
            .then(response => response.json())
            .then(data => {
                // Display response info
                responseInfo.textContent = JSON.stringify(data, null, 2);

                // Show success/error message
                if (data.success) {
                    successMessage.style.display = 'block';
                    errorMessage.style.display = 'none';

                    // Hide success message after 3 seconds
                    setTimeout(() => {
                        successMessage.style.display = 'none';
                    }, 3000);
                } else {
                    errorMessage.textContent = data.message || "Error occurred while saving.";
                    errorMessage.style.display = 'block';
                    successMessage.style.display = 'none';
                }
            })
            .catch(error => {
                console.error("Error saving note:", error);
                responseInfo.textContent = "Error: " + error.message;
                errorMessage.textContent = "Error occurred while saving.";
                errorMessage.style.display = 'block';
                successMessage.style.display = 'none';
            });
        }
    </script>
</body>
</html>"""

# Create default page
def render_page():
    global isNewClient

    # First check if it's an API request
    if handle_api_request():
        return

    # Get user information
    user_id = getUserIdFromCookie()
    userInfo = {"username": "", "password": "", "note": ""}

    if user_id is None:
        user_id = createNewCookie()
    else:
        userInfo = getUserInfo(user_id)
        if userInfo is None:
            isNewClient = True

    # Check POST request debugging info
    if os.environ.get("REQUEST_METHOD") == "POST":
        post_data = debug_post_data()

    print("Content-Type: text/html\r\n")

    if isNewClient:
        print(get_login_page_html())
    else:
        print(get_user_page_html(userInfo))

# Main process
if __name__ == "__main__":
    try:
        render_page()
    except Exception as e:
        # Log the exception
        log_message(f"Exception occurred: {str(e)}")

        # Show error page to user
        print("Content-Type: text/html\r\n")
        show_error_page([f"An unexpected error occurred: {str(e)}"])
