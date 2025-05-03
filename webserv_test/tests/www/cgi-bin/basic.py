#!/usr/bin/env python3
import os
import sys

print("Content-type: text/html\n\n")
print("<html><body>")
print("<h1>Basic CGI Test</h1>")
print("<p>This script works if you can see this message.</p>")
print("<h2>Environment Variables:</h2>")
print("<ul>")
for var in sorted(os.environ.keys()):
    print(f"<li><strong>{var}</strong>: {os.environ[var]}</li>")
print("</ul>")
print("</body></html>")
