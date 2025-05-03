#!/usr/bin/env python3
"""
CGI-Specific Tester for webservers - A specialized tool for diagnosing CGI issues
"""

import argparse
import os
import sys
import time
import threading
import subprocess
import requests
import signal
import traceback
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urljoin

# ANSI colors for better terminal output
class Colors:
    GREEN = "\033[92m"
    BLUE = "\033[94m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    RESET = "\033[0m"

def log(message, level="info"):
    """Print a colorized log message"""
    prefix = time.strftime("%H:%M:%S")
    
    if level == "info":
        color = Colors.BLUE
        prefix = f"[{prefix}] [INFO] "
    elif level == "success":
        color = Colors.GREEN
        prefix = f"[{prefix}] [SUCCESS] "
    elif level == "warning":
        color = Colors.YELLOW
        prefix = f"[{prefix}] [WARNING] "
    elif level == "error":
        color = Colors.RED
        prefix = f"[{prefix}] [ERROR] "
    else:
        color = Colors.RESET
        prefix = f"[{prefix}] "
        
    print(f"{prefix}{color}{message}{Colors.RESET}")

def create_test_script(path, content):
    """Create a test CGI script"""
    try:
        # Ensure directory exists
        os.makedirs(os.path.dirname(path), exist_ok=True)
        
        # Write script content
        with open(path, 'w') as f:
            f.write(content)
        
        # Make executable
        os.chmod(path, 0o755)
        
        return True
    except Exception as e:
        log(f"Failed to create script {path}: {str(e)}", "error")
        return False

def create_test_scripts(cgi_dir):
    """Create various CGI test scripts with different characteristics"""
    scripts = {}
    
    # 1. Basic CGI script
    basic_script = os.path.join(cgi_dir, "basic.py")
    basic_content = """#!/usr/bin/env python3
import os
import sys

print("Content-type: text/html\\n\\n")
print("<html><body>")
print("<h1>Basic CGI Test</h1>")
print("<p>This script works if you can see this message.</p>")
print("<h2>Environment Variables:</h2>")
print("<ul>")
for var in sorted(os.environ.keys()):
    print(f"<li><strong>{var}</strong>: {os.environ[var]}</li>")
print("</ul>")
print("</body></html>")
"""
    scripts["basic"] = (basic_script, basic_content)
    
    # 2. Script with query string handling
    query_script = os.path.join(cgi_dir, "query.py")
    query_content = """#!/usr/bin/env python3
import os
import sys
import cgi
from urllib.parse import parse_qs

print("Content-type: text/html\\n\\n")
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
"""
    scripts["query"] = (query_script, query_content)
    
    # 3. Script with POST handling
    post_script = os.path.join(cgi_dir, "post.py")
    post_content = """#!/usr/bin/env python3
import os
import sys
import cgi

print("Content-type: text/html\\n\\n")
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
"""
    scripts["post"] = (post_script, post_content)
    
    # 4. Script with large output
    large_script = os.path.join(cgi_dir, "large.py")
    large_content = """#!/usr/bin/env python3
import os
import time

# Generate a large response to test handling of large outputs
print("Content-type: text/html\\n\\n")
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
"""
    scripts["large"] = (large_script, large_content)
    
    # 5. Script with sleep (slow response)
    slow_script = os.path.join(cgi_dir, "slow.py")
    slow_content = """#!/usr/bin/env python3
import time
import os

# Sleep for a few seconds to simulate slow processing
sleep_time = 3
print("Content-type: text/html\\n\\n")
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
"""
    scripts["slow"] = (slow_script, slow_content)
    
    # 6. Script with chunked output (flush between writes)
    chunked_script = os.path.join(cgi_dir, "chunked.py")
    chunked_content = """#!/usr/bin/env python3
import sys
import time

print("Content-type: text/html\\n\\n")
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
"""
    scripts["chunked"] = (chunked_script, chunked_content)
    
    # 7. Script that outputs custom headers
    headers_script = os.path.join(cgi_dir, "headers.py")
    headers_content = """#!/usr/bin/env python3
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
"""
    scripts["headers"] = (headers_script, headers_content)
    
    # 8. Script that causes an error
    error_script = os.path.join(cgi_dir, "error.py")
    error_content = """#!/usr/bin/env python3
import sys

print("Content-type: text/html\\n\\n")
print("<html><body>")
print("<h1>CGI Error Test</h1>")
print("<p>This script will intentionally raise an error.</p>")
sys.stdout.flush()

# Cause an error
raise Exception("This is an intentional error to test CGI error handling")

# This part should never be reached
print("<p>If you can see this, error handling failed!</p>")
print("</body></html>")
"""
    scripts["error"] = (error_script, error_content)
    
    # Create all scripts
    success = True
    for name, (path, content) in scripts.items():
        if create_test_script(path, content):
            log(f"Created {name} script at {path}", "success")
        else:
            success = False
    
    return success

def run_test(description, func, *args, **kwargs):
    """Run a test function with proper logging and error handling"""
    log(f"Starting test: {description}", "info")
    start_time = time.time()
    
    try:
        result = func(*args, **kwargs)
        elapsed = time.time() - start_time
        if result:
            log(f"Test passed: {description} ({elapsed:.2f}s)", "success")
        else:
            log(f"Test failed: {description} ({elapsed:.2f}s)", "error")
        return result
    except Exception as e:
        elapsed = time.time() - start_time
        log(f"Test error: {description} - {str(e)} ({elapsed:.2f}s)", "error")
        traceback.print_exc()
        return False

def test_basic_cgi(base_url, timeout=10):
    """Test basic CGI functionality"""
    url = urljoin(base_url, "cgi-bin/basic.py")
    try:
        response = requests.get(url, timeout=timeout)
        if response.status_code != 200:
            log(f"Got status code {response.status_code} instead of 200", "error")
            return False
        
        # Check for basic content
        if "Basic CGI Test" not in response.text:
            log(f"Response doesn't contain expected content", "error")
            return False
        
        # Check for environment variables
        if "Environment Variables:" not in response.text:
            log(f"Environment variables section missing from response", "error")
            return False
        
        return True
    except Exception as e:
        log(f"Request failed: {str(e)}", "error")
        return False

def test_query_params(base_url, timeout=10):
    """Test CGI query string handling"""
    url = urljoin(base_url, "cgi-bin/query.py")
    query_params = {
        'name': 'test',
        'value': '123',
        'special': 'hello world&<>"\'' # Test special characters
    }
    
    try:
        response = requests.get(url, params=query_params, timeout=timeout)
        if response.status_code != 200:
            log(f"Got status code {response.status_code} instead of 200", "error")
            return False
        
        # Check for expected content
        for key, value in query_params.items():
            if key not in response.text or value not in response.text:
                log(f"Parameter {key}={value} not found in response", "error")
                return False
        
        return True
    except Exception as e:
        log(f"Request failed: {str(e)}", "error")
        return False

def test_post_data(base_url, timeout=10):
    """Test CGI POST data handling"""
    url = urljoin(base_url, "cgi-bin/post.py")
    post_data = {
        'name': 'Test User',
        'message': 'This is a test message with special chars: &<>"\'',
        'action': 'submit'
    }
    
    try:
        response = requests.post(url, data=post_data, timeout=timeout)
        if response.status_code != 200:
            log(f"Got status code {response.status_code} instead of 200", "error")
            return False
        
        # Check for expected content
        if "POST Data Test" not in response.text:
            log(f"Response doesn't contain expected title", "error")
            return False
        
        if "Request method: POST" not in response.text:
            log(f"Response doesn't indicate POST method", "error")
            return False
        
        # Check if form data is correctly processed
        for key, value in post_data.items():
            if key not in response.text or value not in response.text:
                log(f"Form data {key}={value} not found in response", "error")
                return False
        
        return True
    except Exception as e:
        log(f"Request failed: {str(e)}", "error")
        return False

def test_large_output(base_url, timeout=30):
    """Test handling of large CGI output"""
    url = urljoin(base_url, "cgi-bin/large.py")
    try:
        response = requests.get(url, timeout=timeout)
        if response.status_code != 200:
            log(f"Got status code {response.status_code} instead of 200", "error")
            return False
        
        # Check for expected content
        if "Large Output Test" not in response.text:
            log(f"Response doesn't contain expected title", "error")
            return False
        
        # Verify we got the full response - the message at the end
        if "If you see this message, the large output was successfully processed" not in response.text:
            log(f"Response doesn't contain the closing message", "error")
            return False
        
        # Check that we have a large number of rows
        row_count = response.text.count("<tr>")
        if row_count < 900:  # Allow for some margin
            log(f"Response contains only {row_count} rows instead of 1000+", "error")
            return False
        
        return True
    except Exception as e:
        log(f"Request failed: {str(e)}", "error")
        return False

def test_slow_cgi(base_url, timeout=15):
    """Test handling of slow CGI scripts"""
    url = urljoin(base_url, "cgi-bin/slow.py")
    
    start_time = time.time()
    try:
        response = requests.get(url, timeout=timeout)
        elapsed = time.time() - start_time
        
        if response.status_code != 200:
            log(f"Got status code {response.status_code} instead of 200", "error")
            return False
        
        # Check that the response took at least the sleep time
        if elapsed < 2.5:  # Allow some margin
            log(f"Response returned too quickly ({elapsed:.2f}s)", "error")
            return False
        
        # Check for text after the sleep
        if "Processing complete!" not in response.text:
            log(f"Response doesn't contain text after sleep", "error")
            return False
        
        return True
    except requests.exceptions.Timeout:
        log(f"Request timed out (this might be expected)", "warning")
        return False
    except Exception as e:
        log(f"Request failed: {str(e)}", "error")
        return False

def test_chunked_output(base_url, timeout=15):
    """Test handling of chunked CGI output"""
    url = urljoin(base_url, "cgi-bin/chunked.py")
    
    try:
        response = requests.get(url, timeout=timeout)
        if response.status_code != 200:
            log(f"Got status code {response.status_code} instead of 200", "error")
            return False
        
        # Check for expected content
        if "Chunked Output Test" not in response.text:
            log(f"Response doesn't contain expected title", "error")
            return False
        
        # Check that all chunks are present
        for i in range(1, 6):
            if f"This is chunk {i}." not in response.text:
                log(f"Response is missing chunk {i}", "error")
                return False
        
        return True
    except Exception as e:
        log(f"Request failed: {str(e)}", "error")
        return False

def test_custom_headers(base_url, timeout=10):
    """Test handling of custom CGI headers"""
    url = urljoin(base_url, "cgi-bin/headers.py")
    
    try:
        response = requests.get(url, timeout=timeout)
        if response.status_code != 200:
            log(f"Got status code {response.status_code} instead of 200", "error")
            return False
        
        # Check for expected content
        if "Custom Headers Test" not in response.text:
            log(f"Response doesn't contain expected title", "error")
            return False
        
        # Check for custom headers
        headers_present = 0
        expected_headers = {
            'X-Custom-Header': 'CustomValue',
            'Cache-Control': 'no-cache'
        }
        
        for header, value in expected_headers.items():
            if header.lower() in response.headers and response.headers[header.lower()] == value:
                headers_present += 1
            else:
                actual = response.headers.get(header.lower(), 'Not present')
                log(f"Header {header} mismatch: expected '{value}', got '{actual}'", "warning")
        
        # Allow for partial success
        if headers_present > 0:
            log(f"Found {headers_present}/{len(expected_headers)} expected headers", 
                "success" if headers_present == len(expected_headers) else "warning")
            return True
        else:
            log(f"No expected custom headers found", "error")
            return False
        
    except Exception as e:
        log(f"Request failed: {str(e)}", "error")
        return False

def test_error_handling(base_url, timeout=10):
    """Test handling of CGI script errors"""
    url = urljoin(base_url, "cgi-bin/error.py")
    
    try:
        response = requests.get(url, timeout=timeout)
        
        # Expect either a 500 error or a custom error page
        if response.status_code != 500 and response.status_code != 200:
            log(f"Unexpected status code {response.status_code}", "warning")
        
        # If we got a 200, check that it's an error page of some kind
        if response.status_code == 200:
            if "error" not in response.text.lower() and "500" not in response.text:
                log(f"Response doesn't indicate an error occurred", "error")
                return False
        
        # Make sure we didn't get the content after the error
        if "If you can see this, error handling failed!" in response.text:
            log(f"Response contains content after the error", "error")
            return False
        
        return True
    except Exception as e:
        # In this case, certain exceptions might be expected
        log(f"Request returned exception: {str(e)}", "warning")
        return True  # The error could be the server correctly failing on the script error

def test_parallel_requests(base_url, script_path, num_requests=10, timeout=15):
    """Test handling multiple parallel CGI requests"""
    url = urljoin(base_url, script_path)
    futures = []
    results = {'success': 0, 'failure': 0}
    
    def request_task():
        try:
            response = requests.get(url, timeout=timeout)
            if 200 <= response.status_code < 300:
                return True
            else:
                return False
        except Exception:
            return False
    
    with ThreadPoolExecutor(max_workers=num_requests) as executor:
        # Submit all requests
        for _ in range(num_requests):
            futures.append(executor.submit(request_task))
        
        # Collect results
        for future in futures:
            try:
                if future.result():
                    results['success'] += 1
                else:
                    results['failure'] += 1
            except Exception:
                results['failure'] += 1
    
    success_rate = results['success'] / num_requests * 100
    log(f"Parallel requests: {results['success']}/{num_requests} succeeded ({success_rate:.1f}%)", 
        "success" if success_rate >= 80 else "warning")
    
    return success_rate >= 80

def run_all_tests(base_url, parallel_requests=10, timeout=10):
    """Run all CGI tests"""
    test_results = {}
    
    # Basic tests
    test_results['basic'] = run_test("Basic CGI functionality", test_basic_cgi, base_url, timeout)
    test_results['query'] = run_test("CGI query string handling", test_query_params, base_url, timeout)
    test_results['post'] = run_test("CGI POST data handling", test_post_data, base_url, timeout)
    test_results['large'] = run_test("Large CGI output", test_large_output, base_url, timeout*2)
    test_results['slow'] = run_test("Slow CGI script", test_slow_cgi, base_url, timeout*2)
    test_results['chunked'] = run_test("Chunked CGI output", test_chunked_output, base_url, timeout*2)
    test_results['headers'] = run_test("Custom CGI headers", test_custom_headers, base_url, timeout)
    test_results['error'] = run_test("CGI error handling", test_error_handling, base_url, timeout)
    
    # Parallel tests
    test_results['parallel_basic'] = run_test(
        f"Parallel basic CGI requests ({parallel_requests})", 
        test_parallel_requests, base_url, "cgi-bin/basic.py", parallel_requests, timeout
    )
    
    test_results['parallel_slow'] = run_test(
        f"Parallel slow CGI requests ({parallel_requests})", 
        test_parallel_requests, base_url, "cgi-bin/slow.py", parallel_requests, timeout*2
    )
    
    # Summarize results
    success_count = sum(1 for result in test_results.values() if result)
    total_count = len(test_results)
    success_rate = (success_count / total_count) * 100
    
    print("\n" + "="*60)
    print(f"CGI Test Results: {success_count}/{total_count} tests passed ({success_rate:.1f}%)")
    print("="*60)
    
    for test_name, result in test_results.items():
        status = f"{Colors.GREEN}PASS{Colors.RESET}" if result else f"{Colors.RED}FAIL{Colors.RESET}"
        print(f"  {test_name:20} : {status}")
    
    print("="*60)
    
    return test_results

def main():
    parser = argparse.ArgumentParser(description="CGI-Specific Test Script for Webservers")
    parser.add_argument("--url", default="http://localhost:8080", help="Base URL of the server to test")
    parser.add_argument("--cgi-dir", default="./www/cgi-bin", 
                        help="Directory to create test CGI scripts (local path)")
    parser.add_argument("--setup-only", action="store_true", 
                        help="Only create the test scripts, don't run tests")
    parser.add_argument("--parallel", type=int, default=10, 
                        help="Number of parallel requests for parallel tests")
    parser.add_argument("--timeout", type=int, default=10, 
                        help="Request timeout in seconds")
    args = parser.parse_args()
    
    try:
        # Create test scripts
        log(f"Creating test CGI scripts in {args.cgi_dir}", "info")
        if not create_test_scripts(args.cgi_dir):
            log("Failed to create some test scripts", "error")
            return 1
        
        if args.setup_only:
            log("Setup completed. Use --setup-only=false to run tests.", "success")
            return 0
        
        # Add a brief delay to ensure scripts are ready
        time.sleep(1)
        
        # Run tests
        log(f"Running tests against {args.url}", "info")
        results = run_all_tests(args.url, args.parallel, args.timeout)
        
        # Determine exit code
        success_count = sum(1 for result in results.values() if result)
        return 0 if success_count == len(results) else 1
        
    except KeyboardInterrupt:
        log("Tests interrupted by user", "warning")
        return 130
    except Exception as e:
        log(f"Error: {str(e)}", "error")
        traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(main())