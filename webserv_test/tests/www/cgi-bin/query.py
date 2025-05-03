#!/usr/bin/env python3
import os
import sys
import cgi
from urllib.parse import parse_qs

print("Content-type: text/html\n\n")
print("<html><body>")
print("<h1>Query String Test</h1>")

# Get query string
query_string = os.environ.get('QUERY_STRING', '')
print(f"<p>Raw query string: {query_string}</p>")

if query_string:
    # Parse query string
    params = parse_qs(query_string)
    print("<h2>Parameters:</h2>")
    print("<ul>")
    for key, values in params.items():
        for value in values:
            print(f"<li><strong>{key}</strong>: {value}</li>")
    print("</ul>")
else:
    print("<p>No query parameters provided. Try adding ?name=value to the URL.</p>")

print("</body></html>")
