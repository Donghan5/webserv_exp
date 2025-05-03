#!/usr/bin/env python3
import sys
import time

print("Content-type: text/html\n\n")
print("<html><body>")
print("<h1>Chunked Output Test</h1>")
print("<p>This script sends data in chunks with flushes between writes.</p>")
sys.stdout.flush()

chunks = [
    "<p>This is chunk 1. Sent at {}</p>".format(time.strftime('%H:%M:%S')),
    "<p>This is chunk 2. Sent at {}</p>".format(time.strftime('%H:%M:%S')),
    "<p>This is chunk 3. Sent at {}</p>".format(time.strftime('%H:%M:%S')),
    "<p>This is chunk 4. Sent at {}</p>".format(time.strftime('%H:%M:%S')),
    "<p>This is chunk 5. Sent at {}</p>".format(time.strftime('%H:%M:%S')),
]

for i, chunk in enumerate(chunks):
    print(f"<div style='padding: 10px; margin: 10px; background-color: #f0f0f0;'>")
    print(chunk)
    print(f"</div>")
    sys.stdout.flush()
    time.sleep(1)  # Wait between chunks

print("<p>If you see all chunks above, the server correctly handled chunked output.</p>")
print("</body></html>")
