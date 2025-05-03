#!/usr/bin/env python3
import sys

print("Content-type: text/html\n\n")
print("<html><body>")
print("<h1>CGI Error Test</h1>")
print("<p>This script will intentionally raise an error.</p>")
sys.stdout.flush()

# Cause an error
raise Exception("This is an intentional error to test CGI error handling")

# This part should never be reached
print("<p>If you can see this, error handling failed!</p>")
print("</body></html>")
