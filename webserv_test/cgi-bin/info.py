#!/usr/bin/env python3
import os
import sys
import platform

print("Content-type: text/html\n\n")
print("<html><head><title>Server Info</title></head><body>")
print("<h1>Server Information</h1>")

print("<h2>Python Info</h2>")
print("<ul>")
print(f"<li>Python version: {platform.python_version()}</li>")
print(f"<li>Python implementation: {platform.python_implementation()}</li>")
print(f"<li>Platform: {platform.platform()}</li>")
print("</ul>")

print("<h2>Server Info</h2>")
print("<ul>")
print(f"<li>Server software: {os.environ.get('SERVER_SOFTWARE', 'Not available')}</li>")
print(f"<li>Server protocol: {os.environ.get('SERVER_PROTOCOL', 'Not available')}</li>")
print(f"<li>Server name: {os.environ.get('SERVER_NAME', 'Not available')}</li>")
print(f"<li>Server port: {os.environ.get('SERVER_PORT', 'Not available')}</li>")
print("</ul>")

print("</body></html>")