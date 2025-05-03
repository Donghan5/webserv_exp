#!/usr/bin/env python3
import os
import time

# Generate a large response to test handling of large outputs
print("Content-type: text/html\n\n")
print("<html><body>")
print("<h1>Large Output Test</h1>")
print(f"<p>Generated at: {time.strftime('%Y-%m-%d %H:%M:%S')}</p>")

print("<p>This script generates a large output to test how the server handles it.</p>")

# Generate a large table with lots of data
print("<table border='1'>")
print("<tr><th>Index</th><th>Square</th><th>Cube</th><th>Timestamp</th></tr>")

for i in range(1, 1001):  # Generate 1000 rows
    print(f"<tr><td>{i}</td><td>{i*i}</td><td>{i*i*i}</td><td>{time.time()}</td></tr>")

print("</table>")
print("<p>If you see this message, the large output was successfully processed.</p>")
print("</body></html>")
