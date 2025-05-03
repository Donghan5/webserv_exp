#!/usr/bin/env python3
import os
import sys
import cgi

print("Content-type: text/html\n\n")
print("<html><body>")
print("<h1>POST Data Test</h1>")

request_method = os.environ.get('REQUEST_METHOD', 'GET')
print(f"<p>Request method: {request_method}</p>")

if request_method == 'POST':
    # Handle POST data
    try:
        form = cgi.FieldStorage()
        print("<h2>Form Data:</h2>")
        
        if form.keys():
            print("<ul>")
            for key in form.keys():
                print(f"<li><strong>{key}</strong>: {form[key].value}</li>")
            print("</ul>")
        else:
            print("<p>No form data received.</p>")
    except Exception as e:
        print(f"<p>Error processing form data: {str(e)}</p>")
else:
    print("<p>This script expects a POST request. Send a form submission to test.</p>")
    print('''
    <form method="post" action="">
        <label for="name">Name:</label>
        <input type="text" id="name" name="name"><br><br>
        <label for="message">Message:</label>
        <textarea id="message" name="message"></textarea><br><br>
        <input type="submit" value="Submit">
    </form>
    ''')

print("</body></html>")
