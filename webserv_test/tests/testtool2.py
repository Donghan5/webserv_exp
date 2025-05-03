#!/usr/bin/env python3
"""
Improved WebServer Diagnostic Tool - A comprehensive tester for HTTP servers 
with fixes for HEAD/OPTIONS and specific return code checks
"""

import argparse
import os
import re
import psutil
import requests
import socket
import sys
import threading
import time
import traceback
import concurrent.futures
from typing import Dict, List, Tuple, Any, Optional
from urllib.parse import urlparse
from datetime import datetime
from collections import defaultdict

# ANSI colors for terminal output
class Colors:
    GREEN = "\033[92m"
    BLUE = "\033[94m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    RESET = "\033[0m"

class WebServerTester:
    def __init__(self, args):
        self.base_url = args.url.rstrip('/')
        self.pid = args.pid
        self.verbose = args.verbose
        self.num_requests = args.requests
        self.max_connections = args.connections
        self.hold_time = args.hold_time
        self.timeout = args.timeout
        self.tests = args.tests.split(',') if args.tests != 'all' else ['basic', 'static', 'cgi', 'methods', 'concurrent', 'stress', 'return_codes']
        self.output_dir = args.output
        self.monitor_interval = args.monitor_interval
        self.check_head_options = not args.no_head_options
        
        # Parse the URL to get host and port
        url_parts = urlparse(self.base_url)
        self.host = url_parts.netloc.split(':')[0]
        self.port = url_parts.port or (443 if url_parts.scheme == 'https' else 80)
        
        # Create output directory if needed
        if self.output_dir and not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)
        
        # Initialize results storage
        self.results = {
            'summary': {},
            'tests': defaultdict(list),
            'errors': defaultdict(list),
            'resources': []
        }
        
        # Setup typical test paths
        self.static_paths = [
            "/",
            "/index.html",
            "/css/style.css",
            "/js/script.js",
            "/images/logo.png",
            "/nonexistent-page",  # 404 test
            "/this-is-a-very-long-url-path-that-might-cause-issues-in-some-servers-depending-on-buffer-sizes-and-parsing-logic-implementation.html",  # Long URL test
        ]
        
        # Setup CGI paths - only include ones that exist
        self.cgi_paths = []
        for path in ["/cgi-bin/test.py", "/cgi-bin/env.py"]:
            if self.check_path_exists(path):
                self.cgi_paths.append(path)
        
        # Return code test paths
        self.return_code_tests = [
            {"path": "/", "expected_code": 200, "description": "Root path returns 200"},
            {"path": "/index.html", "expected_code": 200, "description": "Existing file returns 200"},
            {"path": "/nonexistent", "expected_code": 404, "description": "Non-existent path returns 404"},
            {"path": "/..", "expected_code": 403, "description": "Directory traversal attempt returns 403"},
            {"path": "/cgi-bin/", "method": "GET", "expected_code": [403, 404], "description": "CGI directory listing denied"},
            {"path": "/cgi-bin/nonexistent.py", "expected_code": [404, 403, 500], "description": "Non-existent CGI script"},
            {"path": "/index.html", "method": "POST", "expected_code": [200, 405], "description": "POST to static file"},
            {"path": "/index.html", "method": "DELETE", "expected_code": [200, 403, 405], "description": "DELETE to static file"},
        ]
        
    def check_path_exists(self, path):
        """Check if a path exists by sending a HEAD request (or GET if HEAD not supported)"""
        try:
            url = f"{self.base_url}{path}"
            try:
                # Try HEAD first
                response = requests.head(url, timeout=self.timeout)
                return response.status_code == 200
            except:
                # Fall back to GET
                response = requests.get(url, timeout=self.timeout)
                return response.status_code == 200
        except:
            return False
        
    def log(self, message, level='info', test_name=None):
        """Log a message with optional color coding"""
        timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
        
        if level == 'info':
            color = Colors.BLUE
        elif level == 'success':
            color = Colors.GREEN
        elif level == 'warning':
            color = Colors.YELLOW
        elif level == 'error':
            color = Colors.RED
        else:
            color = Colors.RESET
            
        formatted = f"[{timestamp}] {color}{message}{Colors.RESET}"
        print(formatted)
        
        # Store in results if a test is running
        if test_name:
            if level == 'error':
                self.results['errors'][test_name].append(message)
            else:
                self.results['tests'][test_name].append(message)
    
    def start_resource_monitoring(self):
        """Start a thread to monitor server resources"""
        if not self.pid:
            self.log("No PID provided, resource monitoring disabled", level='warning')
            return None
            
        try:
            # Check if process exists
            process = psutil.Process(self.pid)
            
            # Start monitoring thread
            stop_event = threading.Event()
            monitor_thread = threading.Thread(
                target=self._monitor_resources,
                args=(process, stop_event),
                daemon=True
            )
            monitor_thread.start()
            self.log(f"Resource monitoring started for PID {self.pid}", level='info')
            return stop_event
            
        except psutil.NoSuchProcess:
            self.log(f"Process with PID {self.pid} not found", level='error')
            return None
    
    def _monitor_resources(self, process, stop_event):
        """Monitor and record resource usage for the server process"""
        headers_shown = False
        start_time = time.time()
        
        # Output CSV headers if needed
        if self.output_dir:
            resource_file = os.path.join(self.output_dir, "resource_usage.csv")
            with open(resource_file, 'w') as f:
                f.write("timestamp,elapsed_seconds,cpu_percent,memory_mb,num_fds,num_threads,connections\n")
        
        try:
            while not stop_event.is_set():
                try:
                    # Collect metrics
                    cpu_percent = process.cpu_percent()
                    memory_info = process.memory_info()
                    memory_mb = memory_info.rss / (1024 * 1024)  # Convert to MB
                    
                    try:
                        num_fds = len(process.open_files())
                        num_connections = len(process.connections())
                    except psutil.AccessDenied:
                        num_fds = -1
                        num_connections = -1
                        
                    num_threads = process.num_threads()
                    elapsed = time.time() - start_time
                    
                    # Store metrics
                    metrics = {
                        'timestamp': time.time(),
                        'elapsed': elapsed,
                        'cpu_percent': cpu_percent,
                        'memory_mb': memory_mb,
                        'num_fds': num_fds,
                        'num_threads': num_threads,
                        'connections': num_connections
                    }
                    self.results['resources'].append(metrics)
                    
                    # Print headers if first time
                    if not headers_shown and self.verbose >= 1:
                        print("\n{:<10} {:<10} {:<15} {:<15} {:<15} {:<15}".format(
                            "Time(s)", "CPU(%)", "Memory(MB)", "FDs", "Threads", "Connections"))
                        headers_shown = True
                    
                    # Print current resource usage if verbose
                    if self.verbose >= 1:
                        print("{:<10.1f} {:<10.1f} {:<15.2f} {:<15} {:<15} {:<15}".format(
                            elapsed, cpu_percent, memory_mb, num_fds, num_threads, num_connections))
                    
                    # Write to CSV if output_dir specified
                    if self.output_dir:
                        with open(resource_file, 'a') as f:
                            f.write(f"{time.time()},{elapsed:.2f},{cpu_percent:.2f},{memory_mb:.2f},{num_fds},{num_threads},{num_connections}\n")
                    
                except psutil.NoSuchProcess:
                    self.log(f"Process {self.pid} no longer exists", level='error')
                    break
                    
                # Sleep for the monitoring interval
                time.sleep(self.monitor_interval)
        
        except Exception as e:
            self.log(f"Resource monitoring error: {str(e)}", level='error')
            if self.verbose >= 2:
                traceback.print_exc()
    
    def run_tests(self):
        """Run all selected tests"""
        start_time = time.time()
        
        self.log(f"Starting tests against {self.base_url}", level='info')
        self.log(f"Test categories: {', '.join(self.tests)}", level='info')
        
        # Start resource monitoring if PID provided
        stop_monitoring = self.start_resource_monitoring()
        
        # Check if server is reachable
        try:
            response = requests.get(self.base_url, timeout=self.timeout)
            self.log(f"Server reached successfully, status: {response.status_code}", level='success')
        except requests.exceptions.RequestException as e:
            self.log(f"Failed to connect to server: {str(e)}", level='error')
            return False
        
        # Run selected tests
        if 'basic' in self.tests:
            self.basic_connectivity_test()
            
        if 'static' in self.tests:
            self.static_content_test()
        
        if 'cgi' in self.tests and self.cgi_paths:
            self.cgi_test()
            
        if 'methods' in self.tests:
            self.http_methods_test()
            
        if 'concurrent' in self.tests:
            self.concurrent_connections_test()
            
        if 'stress' in self.tests:
            self.stress_test()
            
        if 'return_codes' in self.tests:
            self.return_codes_test()
        
        # Stop resource monitoring
        if stop_monitoring:
            stop_monitoring.set()
            time.sleep(0.5)  # Give monitoring thread time to finish
        
        # Create final report
        total_time = time.time() - start_time
        self.results['summary']['total_time'] = total_time
        self.log(f"All tests completed in {total_time:.2f} seconds", level='success')
        
        # Write final report
        if self.output_dir:
            self.write_report()
            
        return True
    
    def basic_connectivity_test(self):
        """Test basic connectivity and server info"""
        test_name = 'basic_connectivity'
        self.log(f"Starting basic connectivity test", level='info', test_name=test_name)
        
        start_time = time.time()
        success = 0
        failure = 0
        
        try:
            # Simple GET to root
            response = requests.get(self.base_url, timeout=self.timeout)
            if 200 <= response.status_code < 400:
                success += 1
                self.log(f"Root page: OK ({response.status_code})", level='success', test_name=test_name)
            else:
                failure += 1
                self.log(f"Root page: Failed ({response.status_code})", level='error', test_name=test_name)
            
            # Get server info from headers
            server_info = response.headers.get('Server', 'Not provided')
            self.log(f"Server header: {server_info}", level='info', test_name=test_name)
            
            # Check for redirect handling
            redirect_url = f"{self.base_url}/redirect-test"
            try:
                response = requests.get(redirect_url, timeout=self.timeout, allow_redirects=False)
                if 300 <= response.status_code < 400:
                    self.log(f"Redirect test: OK ({response.status_code})", level='success', test_name=test_name)
                    success += 1
                else:
                    self.log(f"Redirect test: Not a redirect ({response.status_code})", level='warning', test_name=test_name)
            except requests.exceptions.RequestException as e:
                self.log(f"Redirect test: Error: {str(e)}", level='warning', test_name=test_name)
            
            # Check for 404 handling
            not_found_url = f"{self.base_url}/non-existent-resource-{time.time()}"
            try:
                response = requests.get(not_found_url, timeout=self.timeout)
                if response.status_code == 404:
                    self.log(f"404 handling: OK", level='success', test_name=test_name)
                    success += 1
                else:
                    self.log(f"404 handling: Unexpected status ({response.status_code})", level='warning', test_name=test_name)
                    failure += 1
            except requests.exceptions.RequestException as e:
                self.log(f"404 handling: Error: {str(e)}", level='error', test_name=test_name)
                failure += 1
        
        except Exception as e:
            self.log(f"Basic connectivity test error: {str(e)}", level='error', test_name=test_name)
            if self.verbose >= 2:
                traceback.print_exc()
            failure += 1
        
        test_time = time.time() - start_time
        result = {
            'name': test_name,
            'time': test_time,
            'success': success,
            'failure': failure,
            'total': success + failure
        }
        self.results['summary'][test_name] = result
        
        self.log(f"Basic connectivity test completed in {test_time:.2f}s - Success: {success}, Failure: {failure}", 
                'success' if failure == 0 else 'warning', test_name=test_name)
        return result
    
    def static_content_test(self):
        """Test static content handling"""
        test_name = 'static_content'
        self.log(f"Starting static content test ({len(self.static_paths)} paths)", level='info', test_name=test_name)
        
        start_time = time.time()
        success = 0
        failure = 0
        details = []
        
        for path in self.static_paths:
            url = f"{self.base_url}{path}"
            try:
                response = requests.get(url, timeout=self.timeout)
                status = response.status_code
                content_type = response.headers.get('Content-Type', 'Not specified')
                content_length = len(response.content)
                
                # Expected outcomes:
                # - Non-existent should return 404
                # - All others should return 200
                expected_status = 404 if 'nonexistent' in path.lower() else 200
                
                if status == expected_status:
                    success += 1
                    level = 'success'
                else:
                    failure += 1
                    level = 'error'
                
                message = f"GET {path}: {status} ({content_type}, {content_length} bytes)"
                self.log(message, level=level, test_name=test_name)
                
                details.append({
                    'path': path,
                    'status': status,
                    'content_type': content_type,
                    'content_length': content_length,
                    'expected_status': expected_status,
                    'success': status == expected_status
                })
                
            except requests.exceptions.RequestException as e:
                failure += 1
                self.log(f"GET {path}: Failed - {str(e)}", level='error', test_name=test_name)
                details.append({
                    'path': path,
                    'error': str(e),
                    'success': False
                })
        
        test_time = time.time() - start_time
        result = {
            'name': test_name,
            'time': test_time,
            'success': success,
            'failure': failure,
            'total': len(self.static_paths),
            'details': details
        }
        self.results['summary'][test_name] = result
        
        self.log(f"Static content test completed in {test_time:.2f}s - Success: {success}, Failure: {failure}/{len(self.static_paths)}", 
                'success' if failure == 0 else 'warning', test_name=test_name)
        return result
    
    def cgi_test(self):
        """Test CGI script execution"""
        test_name = 'cgi'
        self.log(f"Starting CGI test ({len(self.cgi_paths)} scripts)", level='info', test_name=test_name)
        
        start_time = time.time()
        success = 0
        failure = 0
        details = []
        
        for path in self.cgi_paths:
            url = f"{self.base_url}{path}"
            try:
                # Add query params to test CGI query string handling
                params = {'test': 'value', 'time': str(time.time())}
                response = requests.get(url, params=params, timeout=self.timeout*2)  # Double timeout for CGI
                status = response.status_code
                content_type = response.headers.get('Content-Type', 'Not specified')
                content_length = len(response.content)
                
                if 200 <= status < 400:
                    success += 1
                    level = 'success'
                else:
                    failure += 1
                    level = 'error'
                
                message = f"CGI {path}: {status} ({content_type}, {content_length} bytes)"
                self.log(message, level=level, test_name=test_name)
                
                # Check for specific CGI environment variables in response
                if status == 200:
                    response_text = response.text.lower()
                    
                    # Look for key CGI environment variables in response
                    env_vars_present = any(var in response_text for var in 
                                          ['request_method', 'query_string', 'remote_addr'])
                    
                    if env_vars_present:
                        self.log(f"  CGI environment variables detected in response", level='success', test_name=test_name)
                    else:
                        self.log(f"  No CGI environment variables found in response", level='warning', test_name=test_name)
                
                details.append({
                    'path': path,
                    'status': status,
                    'content_type': content_type,
                    'content_length': content_length,
                    'success': 200 <= status < 400
                })
                
            except requests.exceptions.RequestException as e:
                failure += 1
                self.log(f"CGI {path}: Failed - {str(e)}", level='error', test_name=test_name)
                details.append({
                    'path': path,
                    'error': str(e),
                    'success': False
                })
                
            except Exception as e:
                failure += 1
                self.log(f"CGI {path}: Unexpected error - {str(e)}", level='error', test_name=test_name)
                if self.verbose >= 2:
                    traceback.print_exc()
                details.append({
                    'path': path,
                    'error': str(e),
                    'success': False
                })
        
        # Now test POST to CGI
        if len(self.cgi_paths) > 0:
            cgi_post_path = self.cgi_paths[0]  # Use first CGI script for POST test
            url = f"{self.base_url}{cgi_post_path}"
            try:
                post_data = {'field1': 'value1', 'field2': 'value2'}
                headers = {'Content-Type': 'application/x-www-form-urlencoded'}
                response = requests.post(url, data=post_data, headers=headers, timeout=self.timeout*2)
                
                status = response.status_code
                if 200 <= status < 400:
                    success += 1
                    self.log(f"POST to CGI {cgi_post_path}: {status}", level='success', test_name=test_name)
                else:
                    failure += 1
                    self.log(f"POST to CGI {cgi_post_path}: Failed ({status})", level='error', test_name=test_name)
                
                details.append({
                    'path': cgi_post_path,
                    'method': 'POST',
                    'status': status,
                    'success': 200 <= status < 400
                })
                
            except requests.exceptions.RequestException as e:
                failure += 1
                self.log(f"POST to CGI {cgi_post_path}: Failed - {str(e)}", level='error', test_name=test_name)
                details.append({
                    'path': cgi_post_path,
                    'method': 'POST',
                    'error': str(e),
                    'success': False
                })
        
        test_time = time.time() - start_time
        result = {
            'name': test_name,
            'time': test_time,
            'success': success,
            'failure': failure,
            'total': len(self.cgi_paths) + 1,  # +1 for POST test
            'details': details
        }
        self.results['summary'][test_name] = result
        
        self.log(f"CGI test completed in {test_time:.2f}s - Success: {success}, Failure: {failure}", 
                'success' if failure == 0 else 'warning', test_name=test_name)
        return result
    
    def http_methods_test(self):
        """Test HTTP methods handling"""
        test_name = 'http_methods'
        self.log(f"Starting HTTP methods test", level='info', test_name=test_name)
        
        start_time = time.time()
        success = 0
        failure = 0
        details = []
        
        # Target URLs for method tests
        root_url = self.base_url
        static_url = f"{self.base_url}/index.html"
        post_url = f"{self.base_url}/upload"  # Assuming there's an upload endpoint
        
        # Test GET
        try:
            response = requests.get(root_url, timeout=self.timeout)
            status = response.status_code
            if 200 <= status < 400:
                success += 1
                self.log(f"GET: {status}", level='success', test_name=test_name)
            else:
                failure += 1
                self.log(f"GET: Failed ({status})", level='error', test_name=test_name)
            
            details.append({
                'method': 'GET',
                'url': root_url,
                'status': status,
                'success': 200 <= status < 400
            })
        except requests.exceptions.RequestException as e:
            failure += 1
            self.log(f"GET: Error - {str(e)}", level='error', test_name=test_name)
            details.append({
                'method': 'GET',
                'url': root_url,
                'error': str(e),
                'success': False
            })
        
        # Test HEAD if enabled
        if self.check_head_options:
            try:
                response = requests.head(static_url, timeout=self.timeout)
                status = response.status_code
                if 200 <= status < 400 and len(response.content) == 0:
                    success += 1
                    self.log(f"HEAD: {status} (Content-Length: {response.headers.get('Content-Length', 'Not set')})", 
                            level='success', test_name=test_name)
                else:
                    failure += 1
                    self.log(f"HEAD: Failed ({status}, content length: {len(response.content)})", 
                            level='error', test_name=test_name)
                
                details.append({
                    'method': 'HEAD',
                    'url': static_url,
                    'status': status,
                    'headers_only': len(response.content) == 0,
                    'success': 200 <= status < 400 and len(response.content) == 0
                })
            except requests.exceptions.RequestException as e:
                # HEAD might fail if server doesn't support it - that's acceptable
                self.log(f"HEAD: Error - {str(e)}", level='warning', test_name=test_name)
                details.append({
                    'method': 'HEAD',
                    'url': static_url,
                    'error': str(e),
                    'success': False
                })
        
        # Test POST
        try:
            post_data = {'field1': 'value1', 'field2': 'value2'}
            response = requests.post(post_url, data=post_data, timeout=self.timeout)
            status = response.status_code
            # POST might return 200, 201, or 4xx if endpoint doesn't exist
            if status < 500:  # Just ensure it's not a server error
                success += 1
                self.log(f"POST: {status}", level='success', test_name=test_name)
            else:
                failure += 1
                self.log(f"POST: Failed ({status})", level='error', test_name=test_name)
            
            details.append({
                'method': 'POST',
                'url': post_url,
                'status': status,
                'success': status < 500
            })
        except requests.exceptions.RequestException as e:
            # POST might fail if endpoint doesn't exist - that's acceptable
            self.log(f"POST: Error - {str(e)}", level='warning', test_name=test_name)
            details.append({
                'method': 'POST',
                'url': post_url,
                'error': str(e),
                'success': False
            })
        
        # Test OPTIONS if enabled
        if self.check_head_options:
            try:
                response = requests.options(root_url, timeout=self.timeout)
                status = response.status_code
                allowed_methods = response.headers.get('Allow', '')
                cors_headers = response.headers.get('Access-Control-Allow-Methods', '')
                
                if 200 <= status < 400:
                    success += 1
                    self.log(f"OPTIONS: {status} (Allow: {allowed_methods}, CORS: {cors_headers})", 
                            level='success', test_name=test_name)
                else:
                    failure += 1
                    self.log(f"OPTIONS: Failed ({status})", level='error', test_name=test_name)
                
                details.append({
                    'method': 'OPTIONS',
                    'url': root_url,
                    'status': status,
                    'allow_header': allowed_methods,
                    'cors_header': cors_headers,
                    'success': 200 <= status < 400
                })
            except requests.exceptions.RequestException as e:
                # OPTIONS might fail if server doesn't support it - that's acceptable
                self.log(f"OPTIONS: Error - {str(e)}", level='warning', test_name=test_name)
                details.append({
                    'method': 'OPTIONS',
                    'url': root_url,
                    'error': str(e),
                    'success': False
                })
        
        test_time = time.time() - start_time
        result = {
            'name': test_name,
            'time': test_time,
            'success': success,
            'failure': failure,
            'total': 2 + (2 if self.check_head_options else 0),  # GET, POST, (HEAD, OPTIONS if enabled)
            'details': details
        }
        self.results['summary'][test_name] = result
        
        self.log(f"HTTP methods test completed in {test_time:.2f}s - Success: {success}, Failure: {failure}", 
                'success' if failure == 0 else 'warning', test_name=test_name)
        return result
    
    def concurrent_connections_test(self):
        """Test handling of multiple concurrent connections"""
        test_name = 'concurrent_connections'
        self.log(f"Starting concurrent connections test ({self.max_connections} connections)", 
                level='info', test_name=test_name)
        
        start_time = time.time()
        success = 0
        failure = 0
        connections = []
        alive_connections = 0
        
        # First create all connections
        try:
            for i in range(self.max_connections):
                try:
                    # Create a socket
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(self.timeout)
                    
                    # Connect to the server
                    sock.connect((self.host, self.port))
                    
                    # Send a basic HTTP request to keep the connection alive
                    request = f"GET / HTTP/1.1\r\nHost: {self.host}\r\nConnection: keep-alive\r\n\r\n"
                    sock.send(request.encode())
                    
                    # Add to our list
                    connections.append(sock)
                    success += 1
                    
                    if (i+1) % 10 == 0 or i+1 == self.max_connections:
                        self.log(f"Opened {i+1}/{self.max_connections} connections", 
                                level='info', test_name=test_name)
                
                except Exception as e:
                    failure += 1
                    self.log(f"Failed to open connection {i+1}: {str(e)}", level='error', test_name=test_name)
                    if self.verbose >= 2:
                        traceback.print_exc()
                    break  # Stop if we can't open more connections
            
            total_connections = len(connections)
            self.log(f"Successfully opened {total_connections} connections. Holding for {self.hold_time} seconds...", 
                    level='success', test_name=test_name)
            
            # Hold the connections for the specified time
            time.sleep(self.hold_time)
            
            # Now let's send a request on each connection to test if they're still alive
            alive_connections = 0
            for i, sock in enumerate(connections):
                try:
                    # Send a basic HTTP request
                    request = f"GET /ping HTTP/1.1\r\nHost: {self.host}\r\n\r\n"
                    sock.send(request.encode())
                    
                    # Try to receive a response
                    sock.settimeout(2)
                    response = sock.recv(4096)
                    
                    if response:
                        alive_connections += 1
                except Exception:
                    pass
            
            self.log(f"{alive_connections}/{total_connections} connections still alive after hold time", 
                    level='info', test_name=test_name)
            
        finally:
            # Close all connections
            for sock in connections:
                try:
                    sock.close()
                except:
                    pass
            
            self.log(f"Closed all {len(connections)} connections", level='info', test_name=test_name)
        
        test_time = time.time() - start_time
        result = {
            'name': test_name,
            'time': test_time,
            'connections_attempted': self.max_connections,
            'connections_successful': success,
            'connections_failed': failure,
            'alive_after_hold': alive_connections
        }
        self.results['summary'][test_name] = result
        
        self.log(f"Concurrent connections test completed in {test_time:.2f}s - Success: {success}/{self.max_connections}", 
                'success' if failure == 0 else 'warning', test_name=test_name)
        return result
    
    def stress_test(self):
        """Run a stress test with many parallel requests"""
        test_name = 'stress'
        self.log(f"Starting stress test ({self.num_requests} requests)", level='info', test_name=test_name)
        
        start_time = time.time()
        success = 0
        failure = 0
        
        # Mix of different request types for stress testing
        stress_urls = []
        
        # Add static content requests
        for path in self.static_paths[:3]:  # Use first 3 static paths
            stress_urls.append(f"{self.base_url}{path}")
        
        # Add CGI requests (if available)
        if self.cgi_paths:
            for path in self.cgi_paths[:2]:  # Use first 2 CGI paths
                stress_urls.append(f"{self.base_url}{path}")
        
        # If no CGI paths, add more static paths
        if not self.cgi_paths and len(stress_urls) < 5:
            for path in self.static_paths[3:]:
                stress_urls.append(f"{self.base_url}{path}")
                if len(stress_urls) >= 5:
                    break
        
        # Number of worker threads
        num_workers = min(20, self.num_requests)
        requests_per_worker = self.num_requests // num_workers
        
        # Create a session for connection pooling
        session = requests.Session()
        
        # Results tracking
        results_lock = threading.Lock()
        request_times = []
        status_codes = defaultdict(int)
        errors = defaultdict(int)
        
        def worker_function(worker_id, num_requests):
            nonlocal success, failure
            local_success = 0
            local_failure = 0
            
            for i in range(num_requests):
                url = stress_urls[i % len(stress_urls)]
                try:
                    start = time.time()
                    response = session.get(url, timeout=self.timeout)
                    elapsed = time.time() - start
                    
                    with results_lock:
                        if 200 <= response.status_code < 400:
                            local_success += 1
                        else:
                            local_failure += 1
                        
                        status_codes[response.status_code] += 1
                        request_times.append(elapsed)
                    
                except Exception as e:
                    error_type = type(e).__name__
                    with results_lock:
                        local_failure += 1
                        errors[error_type] += 1
                
                # Small delay to prevent overwhelming the server
                time.sleep(0.01)
            
            with results_lock:
                success += local_success
                failure += local_failure
        
        # Create and start worker threads
        threads = []
        for i in range(num_workers):
            requests_for_this_worker = requests_per_worker
            # Distribute any remainder among the first workers
            if i < self.num_requests % num_workers:
                requests_for_this_worker += 1
                
            thread = threading.Thread(
                target=worker_function, 
                args=(i, requests_for_this_worker)
            )
            threads.append(thread)
            thread.start()
            
            self.log(f"Started worker {i+1} with {requests_for_this_worker} requests", 
                    level='info', test_name=test_name)
        
        # Wait for all threads to complete
        for i, thread in enumerate(threads):
            thread.join()
            if self.verbose >= 1:
                self.log(f"Worker {i+1} completed", level='info', test_name=test_name)
        
        # Calculate statistics
        total_requests = success + failure
        success_rate = (success / total_requests * 100) if total_requests > 0 else 0
        
        # Calculate response time percentiles
        if request_times:
            request_times.sort()
            p50 = request_times[len(request_times) // 2]
            p90 = request_times[int(len(request_times) * 0.9)]
            p99 = request_times[int(len(request_times) * 0.99)]
            avg = sum(request_times) / len(request_times)
        else:
            p50 = p90 = p99 = avg = 0
        
        # Log results
        self.log(f"Stress test results:", level='info', test_name=test_name)
        self.log(f"  Success rate: {success_rate:.2f}% ({success}/{total_requests})", 
                'success' if success_rate > 95 else 'warning', test_name=test_name)
        self.log(f"  Response times - Avg: {avg:.3f}s, P50: {p50:.3f}s, P90: {p90:.3f}s, P99: {p99:.3f}s", 
                level='info', test_name=test_name)
        
        if status_codes:
            status_summary = ", ".join([f"{code}: {count}" for code, count in status_codes.items()])
            self.log(f"  Status codes: {status_summary}", level='info', test_name=test_name)
        
        if errors:
            error_summary = ", ".join([f"{err}: {count}" for err, count in errors.items()])
            self.log(f"  Errors: {error_summary}", level='error', test_name=test_name)
        
        # Clean up
        session.close()
        
        test_time = time.time() - start_time
        result = {
            'name': test_name,
            'time': test_time,
            'requests': total_requests,
            'success': success,
            'failure': failure,
            'success_rate': success_rate,
            'response_times': {
                'average': avg,
                'p50': p50,
                'p90': p90,
                'p99': p99
            },
            'status_codes': dict(status_codes),
            'errors': dict(errors)
        }
        self.results['summary'][test_name] = result
        
        self.log(f"Stress test completed in {test_time:.2f}s - {total_requests} requests, {success_rate:.2f}% success rate",
                'success' if success_rate > 95 else 'warning', test_name=test_name)
        return result
    
    def return_codes_test(self):
        """Test specific return codes for various requests"""
        test_name = 'return_codes'
        self.log(f"Starting return codes test", level='info', test_name=test_name)
        
        start_time = time.time()
        success = 0
        failure = 0
        details = []
        
        for test in self.return_code_tests:
            path = test['path']
            method = test.get('method', 'GET')
            expected_code = test['expected_code']
            description = test['description']
            
            url = f"{self.base_url}{path}"
            try:
                # Make the request with the specified method
                if method == 'GET':
                    response = requests.get(url, timeout=self.timeout)
                elif method == 'POST':
                    response = requests.post(url, data={}, timeout=self.timeout)
                elif method == 'DELETE':
                    response = requests.delete(url, timeout=self.timeout)
                else:
                    # Skip unsupported methods
                    continue
                
                status = response.status_code
                
                # Check if the status code matches the expected code or codes
                if isinstance(expected_code, list):
                    # If expected_code is a list, check if status is in the list
                    is_expected = status in expected_code
                else:
                    # If expected_code is a single value, compare directly
                    is_expected = status == expected_code
                
                if is_expected:
                    success += 1
                    level = 'success'
                else:
                    failure += 1
                    level = 'error'
                
                message = f"{method} {path}: {status} (Expected: {expected_code}) - {description}"
                self.log(message, level=level, test_name=test_name)
                
                details.append({
                    'path': path,
                    'method': method,
                    'status': status,
                    'expected_code': expected_code,
                    'description': description,
                    'success': is_expected
                })
                
            except requests.exceptions.RequestException as e:
                failure += 1
                self.log(f"{method} {path}: Failed - {str(e)}", level='error', test_name=test_name)
                details.append({
                    'path': path,
                    'method': method,
                    'error': str(e),
                    'expected_code': expected_code,
                    'description': description,
                    'success': False
                })
        
        test_time = time.time() - start_time
        result = {
            'name': test_name,
            'time': test_time,
            'success': success,
            'failure': failure,
            'total': len(self.return_code_tests),
            'details': details
        }
        self.results['summary'][test_name] = result
        
        self.log(f"Return codes test completed in {test_time:.2f}s - Success: {success}, Failure: {failure}", 
                'success' if failure == 0 else 'warning', test_name=test_name)
        return result
    
    def write_report(self):
        """Write a detailed HTML report of the test results"""
        report_file = os.path.join(self.output_dir, "report.html")
        
        # Basic HTML template
        html = f"""<!DOCTYPE html>
<html>
<head>
    <title>WebServer Test Report - {datetime.now().strftime('%Y-%m-%d %H:%M')}</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        h1, h2, h3 {{ color: #333; }}
        .summary {{ margin-bottom: 30px; }}
        .test-section {{ margin-bottom: 30px; border: 1px solid #ddd; padding: 15px; border-radius: 5px; }}
        .success {{ color: green; }}
        .failure {{ color: red; }}
        .warning {{ color: orange; }}
        table {{ border-collapse: collapse; width: 100%; margin-top: 10px; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
        tr:nth-child(even) {{ background-color: #f9f9f9; }}
        .chart-container {{ height: 300px; margin-top: 20px; }}
    </style>
</head>
<body>
    <h1>WebServer Test Report</h1>
    <p>Server: {self.base_url}</p>
    <p>Date: {datetime.now().strftime('%Y-%m-%d %H:%M')}</p>
    
    <div class="summary">
        <h2>Summary</h2>
        <table>
            <tr>
                <th>Test</th>
                <th>Success</th>
                <th>Failure</th>
                <th>Success Rate</th>
                <th>Time (s)</th>
            </tr>
"""
        
        # Add summary rows
        for test_name, result in self.results['summary'].items():
            if 'success' in result and 'total' in result:
                success_rate = (result['success'] / result['total'] * 100) if result['total'] > 0 else 0
                html += f"""
            <tr>
                <td>{test_name}</td>
                <td>{result['success']}</td>
                <td>{result['failure']}</td>
                <td>{success_rate:.2f}%</td>
                <td>{result['time']:.2f}</td>
            </tr>"""
            elif 'success_rate' in result:
                html += f"""
            <tr>
                <td>{test_name}</td>
                <td>{result.get('success', 0)}</td>
                <td>{result.get('failure', 0)}</td>
                <td>{result['success_rate']:.2f}%</td>
                <td>{result['time']:.2f}</td>
            </tr>"""
        
        html += """
        </table>
    </div>
"""
        
        # Add detailed test results
        for test_name, result in self.results['summary'].items():
            html += f"""
    <div class="test-section">
        <h2>{test_name}</h2>
        <p>Time: {result['time']:.2f} seconds</p>
"""
            
            # Add test-specific details
            if test_name == 'static_content' and 'details' in result:
                html += """
        <h3>Static Content Results</h3>
        <table>
            <tr>
                <th>Path</th>
                <th>Status</th>
                <th>Content Type</th>
                <th>Size (bytes)</th>
                <th>Result</th>
            </tr>
"""
                for detail in result['details']:
                    if 'error' in detail:
                        html += f"""
            <tr>
                <td>{detail['path']}</td>
                <td colspan="3">Error: {detail['error']}</td>
                <td class="failure">Failed</td>
            </tr>"""
                    else:
                        result_class = "success" if detail['success'] else "failure"
                        html += f"""
            <tr>
                <td>{detail['path']}</td>
                <td>{detail['status']}</td>
                <td>{detail.get('content_type', 'N/A')}</td>
                <td>{detail.get('content_length', 'N/A')}</td>
                <td class="{result_class}">{detail['success']}</td>
            </tr>"""
                html += """
        </table>
"""
            
            elif test_name == 'cgi' and 'details' in result:
                html += """
        <h3>CGI Results</h3>
        <table>
            <tr>
                <th>Path</th>
                <th>Method</th>
                <th>Status</th>
                <th>Content Type</th>
                <th>Result</th>
            </tr>
"""
                for detail in result['details']:
                    if 'error' in detail:
                        html += f"""
            <tr>
                <td>{detail['path']}</td>
                <td>{detail.get('method', 'GET')}</td>
                <td colspan="2">Error: {detail['error']}</td>
                <td class="failure">Failed</td>
            </tr>"""
                    else:
                        result_class = "success" if detail['success'] else "failure"
                        html += f"""
            <tr>
                <td>{detail['path']}</td>
                <td>{detail.get('method', 'GET')}</td>
                <td>{detail['status']}</td>
                <td>{detail.get('content_type', 'N/A')}</td>
                <td class="{result_class}">{detail['success']}</td>
            </tr>"""
                html += """
        </table>
"""
            
            elif test_name == 'http_methods' and 'details' in result:
                html += """
        <h3>HTTP Methods Results</h3>
        <table>
            <tr>
                <th>Method</th>
                <th>URL</th>
                <th>Status</th>
                <th>Result</th>
            </tr>
"""
                for detail in result['details']:
                    if 'error' in detail:
                        html += f"""
            <tr>
                <td>{detail['method']}</td>
                <td>{detail.get('url', 'N/A')}</td>
                <td>Error: {detail['error']}</td>
                <td class="failure">Failed</td>
            </tr>"""
                    else:
                        result_class = "success" if detail['success'] else "failure"
                        html += f"""
            <tr>
                <td>{detail['method']}</td>
                <td>{detail.get('url', 'N/A')}</td>
                <td>{detail['status']}</td>
                <td class="{result_class}">{detail['success']}</td>
            </tr>"""
                html += """
        </table>
"""
            
            elif test_name == 'return_codes' and 'details' in result:
                html += """
        <h3>Return Codes Results</h3>
        <table>
            <tr>
                <th>Method</th>
                <th>Path</th>
                <th>Status</th>
                <th>Expected</th>
                <th>Description</th>
                <th>Result</th>
            </tr>
"""
                for detail in result['details']:
                    if 'error' in detail:
                        html += f"""
            <tr>
                <td>{detail['method']}</td>
                <td>{detail['path']}</td>
                <td colspan="2">Error: {detail['error']}</td>
                <td>{detail.get('description', '')}</td>
                <td class="failure">Failed</td>
            </tr>"""
                    else:
                        result_class = "success" if detail['success'] else "failure"
                        expected_code = detail['expected_code']
                        if isinstance(expected_code, list):
                            expected_str = ", ".join(map(str, expected_code))
                        else:
                            expected_str = str(expected_code)
                        
                        html += f"""
            <tr>
                <td>{detail['method']}</td>
                <td>{detail['path']}</td>
                <td>{detail['status']}</td>
                <td>{expected_str}</td>
                <td>{detail.get('description', '')}</td>
                <td class="{result_class}">{detail['success']}</td>
            </tr>"""
                html += """
        </table>
"""
            
            elif test_name == 'concurrent_connections':
                html += f"""
        <h3>Concurrent Connections Results</h3>
        <p>Connections attempted: {result['connections_attempted']}</p>
        <p>Connections successful: {result['connections_successful']}</p>
        <p>Connections failed: {result['connections_failed']}</p>
        <p>Connections alive after hold time: {result.get('alive_after_hold', 'N/A')}</p>
"""
            
            elif test_name == 'stress':
                html += f"""
        <h3>Stress Test Results</h3>
        <p>Total requests: {result.get('requests', 0)}</p>
        <p>Success rate: {result.get('success_rate', 0):.2f}%</p>
        
        <h4>Response Times</h4>
        <table>
            <tr>
                <th>Average</th>
                <th>P50</th>
                <th>P90</th>
                <th>P99</th>
            </tr>
            <tr>
                <td>{result.get('response_times', {}).get('average', 0):.3f}s</td>
                <td>{result.get('response_times', {}).get('p50', 0):.3f}s</td>
                <td>{result.get('response_times', {}).get('p90', 0):.3f}s</td>
                <td>{result.get('response_times', {}).get('p99', 0):.3f}s</td>
            </tr>
        </table>
        
        <h4>Status Codes</h4>
        <table>
            <tr>
                <th>Code</th>
                <th>Count</th>
            </tr>
"""
                for code, count in result.get('status_codes', {}).items():
                    html += f"""
            <tr>
                <td>{code}</td>
                <td>{count}</td>
            </tr>"""
                html += """
        </table>
"""
                if result.get('errors', {}):
                    html += """
        <h4>Errors</h4>
        <table>
            <tr>
                <th>Error</th>
                <th>Count</th>
            </tr>
"""
                    for error, count in result.get('errors', {}).items():
                        html += f"""
            <tr>
                <td>{error}</td>
                <td>{count}</td>
            </tr>"""
                    html += """
        </table>
"""
            
            html += """
    </div>
"""
        
        # Add resource monitoring section if available
        if self.results['resources']:
            html += """
    <div class="test-section">
        <h2>Resource Monitoring</h2>
        <div class="chart-container">
            <canvas id="cpuChart"></canvas>
        </div>
        <div class="chart-container">
            <canvas id="memoryChart"></canvas>
        </div>
        <div class="chart-container">
            <canvas id="fdChart"></canvas>
        </div>
    </div>
    
    <!-- Include Chart.js -->
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script>
        // Prepare data for charts
        const timestamps = [];
        const cpuData = [];
        const memoryData = [];
        const fdData = [];
        const threadData = [];
        const connectionData = [];
"""
            
            # Add resource data points
            for i, metrics in enumerate(self.results['resources']):
                html += f"""
        timestamps.push({metrics['elapsed']:.2f});
        cpuData.push({metrics['cpu_percent']:.2f});
        memoryData.push({metrics['memory_mb']:.2f});
        fdData.push({metrics['num_fds']});
        threadData.push({metrics['num_threads']});
        connectionData.push({metrics['connections']});
"""
            
            html += """
        // Create CPU Chart
        new Chart(document.getElementById('cpuChart'), {
            type: 'line',
            data: {
                labels: timestamps,
                datasets: [{
                    label: 'CPU Usage (%)',
                    data: cpuData,
                    borderColor: 'rgba(75, 192, 192, 1)',
                    backgroundColor: 'rgba(75, 192, 192, 0.2)',
                    tension: 0.1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                            display: true,
                            text: 'CPU (%)'
                        }
                    },
                    x: {
                        title: {
                            display: true,
                            text: 'Time (seconds)'
                        }
                    }
                }
            }
        });
        
        // Create Memory Chart
        new Chart(document.getElementById('memoryChart'), {
            type: 'line',
            data: {
                labels: timestamps,
                datasets: [{
                    label: 'Memory Usage (MB)',
                    data: memoryData,
                    borderColor: 'rgba(255, 99, 132, 1)',
                    backgroundColor: 'rgba(255, 99, 132, 0.2)',
                    tension: 0.1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                            display: true,
                            text: 'Memory (MB)'
                        }
                    },
                    x: {
                        title: {
                            display: true,
                            text: 'Time (seconds)'
                        }
                    }
                }
            }
        });
        
        // Create FD and Connections Chart
        new Chart(document.getElementById('fdChart'), {
            type: 'line',
            data: {
                labels: timestamps,
                datasets: [{
                    label: 'File Descriptors',
                    data: fdData,
                    borderColor: 'rgba(54, 162, 235, 1)',
                    backgroundColor: 'rgba(54, 162, 235, 0.2)',
                    tension: 0.1
                }, {
                    label: 'Connections',
                    data: connectionData,
                    borderColor: 'rgba(153, 102, 255, 1)',
                    backgroundColor: 'rgba(153, 102, 255, 0.2)',
                    tension: 0.1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                            display: true,
                            text: 'Count'
                        }
                    },
                    x: {
                        title: {
                            display: true,
                            text: 'Time (seconds)'
                        }
                    }
                }
            }
        });
    </script>
"""
        
        # Close HTML document
        html += """
</body>
</html>
"""
        
        # Write to file
        with open(report_file, 'w') as f:
            f.write(html)
        
        self.log(f"Report written to {report_file}", level='success')

def create_cgi_scripts(directory, verbose=False):
    """Create test CGI scripts in the specified directory"""
    if not os.path.exists(directory):
        try:
            os.makedirs(directory)
            if verbose:
                print(f"Created directory: {directory}")
        except Exception as e:
            print(f"Error creating directory {directory}: {str(e)}")
            return False
    
    scripts = {
        'test.py': '''#!/usr/bin/env python3
import os
import sys
import cgi

print("Content-type: text/html\\n\\n")
print("<html><head><title>CGI Test</title></head><body>")
print("<h1>CGI Test Script</h1>")
print("<p>This is a test CGI script.</p>")

# Print some environment variables
print("<h2>Environment Variables</h2>")
print("<ul>")
for var in ["REQUEST_METHOD", "QUERY_STRING", "REMOTE_ADDR", "HTTP_USER_AGENT", "SERVER_PROTOCOL"]:
    if var in os.environ:
        print(f"<li>{var}: {os.environ[var]}</li>")
print("</ul>")

# Process form data if POST
if os.environ.get("REQUEST_METHOD") == "POST":
    print("<h2>POST Data</h2>")
    try:
        form = cgi.FieldStorage()
        print("<ul>")
        for key in form.keys():
            print(f"<li>{key}: {form[key].value}</li>")
        print("</ul>")
    except Exception as e:
        print(f"<p>Error processing form data: {str(e)}</p>")

print("</body></html>")
''',
        'env.py': '''#!/usr/bin/env python3
import os
import time

print("Content-type: text/html\\n\\n")
print("<html><head><title>Environment Variables</title></head><body>")
print("<h1>Environment Variables</h1>")
print("<table border='1'>")
print("<tr><th>Variable</th><th>Value</th></tr>")

# Print all environment variables
for key, value in sorted(os.environ.items()):
    print(f"<tr><td>{key}</td><td>{value}</td></tr>")

print("</table>")
print("<p>Generated at: " + time.strftime("%Y-%m-%d %H:%M:%S") + "</p>")
print("</body></html>")
''',
    }
    
    for filename, content in scripts.items():
        script_path = os.path.join(directory, filename)
        try:
            with open(script_path, 'w') as f:
                f.write(content)
            
            # Make executable
            os.chmod(script_path, 0o755)
            
            if verbose:
                print(f"Created CGI script: {script_path}")
        except Exception as e:
            print(f"Error creating CGI script {script_path}: {str(e)}")
            return False
    
    return True

def create_static_content(directory, verbose=False):
    """Create test static content in the specified directory"""
    if not os.path.exists(directory):
        try:
            os.makedirs(directory)
            if verbose:
                print(f"Created directory: {directory}")
        except Exception as e:
            print(f"Error creating directory {directory}: {str(e)}")
            return False
    
    # Create subdirectories
    for subdir in ["css", "js", "images"]:
        subdir_path = os.path.join(directory, subdir)
        if not os.path.exists(subdir_path):
            try:
                os.makedirs(subdir_path)
                if verbose:
                    print(f"Created directory: {subdir_path}")
            except Exception as e:
                print(f"Error creating directory {subdir_path}: {str(e)}")
                continue
    
    # Create static files
    files = {
        "index.html": '''<!DOCTYPE html>
<html>
<head>
    <title>Webserver Test Page</title>
    <link rel="stylesheet" href="/css/style.css">
    <script src="/js/script.js" defer></script>
</head>
<body>
    <div class="container">
        <h1>Welcome to the Webserver Test Page</h1>
        <p>This is a test page to verify that the webserver is functioning correctly.</p>
        
        <div class="section">
            <h2>Test Links</h2>
            <ul>
                <li><a href="/about.html">About Page</a></li>
                <li><a href="/404test">Test 404 Error</a></li>
                <li><a href="/cgi-bin/test.py">CGI Test Script</a></li>
                <li><a href="/cgi-bin/env.py">CGI Environment Variables</a></li>
            </ul>
        </div>
        
        <div class="section">
            <h2>Test Form</h2>
            <form action="/cgi-bin/test.py" method="post">
                <div class="form-group">
                    <label for="name">Name:</label>
                    <input type="text" id="name" name="name">
                </div>
                <div class="form-group">
                    <label for="message">Message:</label>
                    <textarea id="message" name="message"></textarea>
                </div>
                <button type="submit" id="submit-btn">Submit</button>
            </form>
        </div>
        
        <div class="section">
            <h2>Server Status</h2>
            <div id="server-status">
                <p>JavaScript is working if this text changes!</p>
            </div>
        </div>
    </div>
</body>
</html>''',
        
        "about.html": '''<!DOCTYPE html>
<html>
<head>
    <title>About - Webserver Test</title>
    <link rel="stylesheet" href="/css/style.css">
</head>
<body>
    <div class="container">
        <h1>About This Test</h1>
        <p>This is a simple about page to test the webserver.</p>
        <p>If you can see this page, the webserver is properly serving static HTML files.</p>
        
        <a href="/">Back to Home</a>
    </div>
</body>
</html>''',
        
        "css/style.css": '''/* Basic styles for test pages */
body {
    font-family: Arial, sans-serif;
    line-height: 1.6;
    margin: 0;
    padding: 20px;
    background-color: #f5f5f5;
    color: #333;
}

.container {
    max-width: 800px;
    margin: 0 auto;
    background: white;
    padding: 20px;
    border-radius: 5px;
    box-shadow: 0 2px 5px rgba(0,0,0,0.1);
}

h1, h2 {
    color: #444;
}

.section {
    margin-bottom: 30px;
    padding-bottom: 20px;
    border-bottom: 1px solid #eee;
}

a {
    color: #0066cc;
    text-decoration: none;
}

a:hover {
    text-decoration: underline;
}

form {
    background: #f9f9f9;
    padding: 15px;
    border-radius: 4px;
}

.form-group {
    margin-bottom: 15px;
}

label {
    display: block;
    margin-bottom: 5px;
    font-weight: bold;
}

input,
textarea {
    width: 100%;
    padding: 8px;
    border: 1px solid #ddd;
    border-radius: 4px;
}

button {
    background: #0066cc;
    color: white;
    padding: 10px 15px;
    border: none;
    border-radius: 4px;
    cursor: pointer;
}

button:hover {
    background: #0056b3;
}

#server-status {
    background: #f0f0f0;
    padding: 10px;
    border-radius: 4px;
}''',
        
        "js/script.js": '''// Simple JavaScript for the test page
document.addEventListener('DOMContentLoaded', function() {
    // Update the server status div
    const statusDiv = document.getElementById('server-status');
    if (statusDiv) {
        statusDiv.innerHTML = `
            <p>JavaScript is working correctly!</p>
            <p>Page loaded at: ${new Date().toLocaleTimeString()}</p>
        `;
    }
    
    // Add event listener to the submit button
    const submitBtn = document.getElementById('submit-btn');
    if (submitBtn) {
        submitBtn.addEventListener('click', function(e) {
            const nameInput = document.getElementById('name');
            const messageInput = document.getElementById('message');
            
            // Simple form validation
            if (!nameInput.value || !messageInput.value) {
                e.preventDefault();
                alert('Please fill out all fields!');
            }
        });
    }
});''',

        "images/logo.png": '''iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+P+/HgAFdQIff6xPPgAAAABJRU5ErkJggg=='''
    }
    
    for filename, content in files.items():
        file_path = os.path.join(directory, filename)
        dir_path = os.path.dirname(file_path)
        
        # Create directories if they don't exist
        if not os.path.exists(dir_path):
            try:
                os.makedirs(dir_path)
            except Exception as e:
                print(f"Error creating directory {dir_path}: {str(e)}")
                continue
        
        try:
            # Special handling for binary files like logo.png
            if filename.endswith('.png'):
                import base64
                with open(file_path, 'wb') as f:
                    f.write(base64.b64decode(content))
            else:
                with open(file_path, 'w') as f:
                    f.write(content)
            
            if verbose:
                print(f"Created file: {file_path}")
        except Exception as e:
            print(f"Error creating file {file_path}: {str(e)}")
    
    return True

def setup_test_environment(www_dir, cgi_dir, verbose=False):
    """Set up the test environment with static content and CGI scripts"""
    success = True
    
    # Create static content
    if not create_static_content(www_dir, verbose):
        print("Warning: Failed to create some static content")
        success = False
    
    # Create CGI scripts
    if not create_cgi_scripts(cgi_dir, verbose):
        print("Warning: Failed to create some CGI scripts")
        success = False
    
    return success

def main():
    parser = argparse.ArgumentParser(description="Improved WebServer Diagnostic Tool")
    
    # Server configuration
    parser.add_argument("--url", default="http://localhost:8080", help="Base URL of the server to test")
    parser.add_argument("--pid", type=int, default=0, help="PID of the server process to monitor (0 to skip)")
    
    # Test configuration
    parser.add_argument("--tests", default="all", help="Comma-separated list of tests to run (basic,static,cgi,methods,concurrent,stress,return_codes)")
    parser.add_argument("--requests", type=int, default=100, help="Number of requests for stress test")
    parser.add_argument("--connections", type=int, default=100, help="Max concurrent connections to test")
    parser.add_argument("--hold-time", type=int, default=5, help="Time to hold connections open (seconds)")
    parser.add_argument("--timeout", type=int, default=10, help="Request timeout (seconds)")
    parser.add_argument("--no-head-options", action="store_true", help="Skip HEAD and OPTIONS method tests")
    
    # Output configuration
    parser.add_argument("--output", default="", help="Directory to store test reports and data")
    parser.add_argument("--verbose", "-v", action="count", default=0, help="Increase verbosity (can be used multiple times)")
    parser.add_argument("--monitor-interval", type=float, default=0.5, help="Resource monitoring interval (seconds)")
    
    # Setup options
    parser.add_argument("--setup", action="store_true", help="Set up test environment with static content and CGI scripts")
    parser.add_argument("--www-dir", default="./www", help="Directory for static content")
    parser.add_argument("--cgi-dir", default="./www/cgi-bin", help="Directory for CGI scripts")
    
    args = parser.parse_args()
    
    # Set up test environment if requested
    if args.setup:
        print(f"Setting up test environment in {args.www_dir} and {args.cgi_dir}...")
        if setup_test_environment(args.www_dir, args.cgi_dir, args.verbose > 0):
            print("Test environment set up successfully!")
        else:
            print("Test environment setup completed with warnings (see above).")
        return 0
    
    # Create and run tester
    try:
        tester = WebServerTester(args)
        if tester.run_tests():
            print("All tests completed successfully.")
            return 0
        else:
            print("Tests completed with errors.")
            return 1
    except KeyboardInterrupt:
        print("\nTesting interrupted by user.")
        return 130
    except Exception as e:
        print(f"Error: {str(e)}")
        if args.verbose > 1:
            traceback.print_exc()
        return 1

if __name__ == "__main__":
    sys.exit(main())