#!/usr/bin/env python3
import time
import os

# Simulate slow processing
time.sleep(2)

print("Content-type: text/html\n\n")
print("<html><head><title>Slow CGI</title></head><body>")
print("<h1>Slow CGI Response</h1>")
print("<p>This script intentionally delays for 2 seconds before responding.</p>")
print("<p>Current time: " + time.strftime("%Y-%m-%d %H:%M:%S") + "</p>")
print("<p>REQUEST_METHOD: " + os.environ.get("REQUEST_METHOD", "Not set") + "</p>")
print("</body></html>")