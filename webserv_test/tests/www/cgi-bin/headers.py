#!/usr/bin/env python3
import time
import os

# Output custom headers
print("Content-type: text/html")
print("X-Custom-Header: CustomValue")
print("X-Generated-Time: " + time.strftime('%Y-%m-%d %H:%M:%S'))
print("Cache-Control: no-cache")
print()  # Empty line to separate headers from body

print("<html><body>")
print("<h1>Custom Headers Test</h1>")
print("<p>This script outputs custom HTTP headers.</p>")
print("<p>If the server is handling CGI headers correctly, the following headers were sent:</p>")
print("<ul>")
print("<li>Content-type: text/html</li>")
print("<li>X-Custom-Header: CustomValue</li>")
print("<li>X-Generated-Time: " + time.strftime('%Y-%m-%d %H:%M:%S') + "</li>")
print("<li>Cache-Control: no-cache</li>")
print("</ul>")
print("<p>You can check the network tab in developer tools to confirm these headers.</p>")
print("</body></html>")
