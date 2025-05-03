#!/usr/bin/env python3
import time
import os

# Sleep for a few seconds to simulate slow processing
sleep_time = 3
print("Content-type: text/html\n\n")
print("<html><body>")
print("<h1>Slow CGI Test</h1>")
print(f"<p>This script will pause for {sleep_time} seconds...</p>")
print("<!-- Flushing output buffer -->")
sys.stdout.flush()

time.sleep(sleep_time)

print("<p>Processing complete!</p>")
print(f"<p>If you can see this, the server correctly waited for the script to finish.</p>")
print(f"<p>Script executed at: {time.strftime('%Y-%m-%d %H:%M:%S')}</p>")
print("</body></html>")
