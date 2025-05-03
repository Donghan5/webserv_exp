#!/usr/bin/env python3
"""
WebServer Resource Leak Tester - Specialized tool for detecting and analyzing resource leaks
"""

import argparse
import os
import sys
import time
import threading
import signal
import traceback
import socket
import json
import psutil
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse, urljoin
import requests
import matplotlib.pyplot as plt
from datetime import datetime

# ANSI colors for terminal output
class Colors:
    GREEN = "\033[92m"
    BLUE = "\033[94m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    RESET = "\033[0m"

def log(message, level="info"):
    """Log a message with color coding"""
    timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
    
    if level == "info":
        color = Colors.BLUE
        prefix = f"[{timestamp}] [INFO] "
    elif level == "success":
        color = Colors.GREEN
        prefix = f"[{timestamp}] [SUCCESS] "
    elif level == "warning":
        color = Colors.YELLOW
        prefix = f"[{timestamp}] [WARNING] "
    elif level == "error":
        color = Colors.RED
        prefix = f"[{timestamp}] [ERROR] "
    else:
        color = Colors.RESET
        prefix = f"[{timestamp}] "
        
    print(f"{prefix}{color}{message}{Colors.RESET}")

class ResourceMonitor:
    """Monitor process resource usage over time"""
    
    def __init__(self, pid, interval=0.5):
        self.pid = pid
        self.interval = interval
        self.process = None
        self.stop_event = threading.Event()
        self.metrics = []
        self.start_time = None
        
    def start(self):
        """Start monitoring resources"""
        try:
            # Check if the process exists
            self.process = psutil.Process(self.pid)
            
            # Start monitoring thread
            self.stop_event.clear()
            self.start_time = time.time()
            
            self.thread = threading.Thread(target=self._monitor_thread, daemon=True)
            self.thread.start()
            
            log(f"Started monitoring process {self.pid}", "info")
            return True
        except psutil.NoSuchProcess:
            log(f"Process with PID {self.pid} not found", "error")
            return False
    
    def stop(self):
        """Stop monitoring resources"""
        if self.thread and self.thread.is_alive():
            self.stop_event.set()
            self.thread.join(timeout=2)
            log(f"Stopped monitoring process {self.pid}", "info")
            return True
        return False
    
    def get_metrics(self):
        """Get captured metrics"""
        return self.metrics
    
    def _monitor_thread(self):
        """Thread function for monitoring resources"""
        log(f"Resource monitoring started", "info")
        
        # Headers for the resource usage report
        print("\nResource Monitoring:")
        print("{:<10} {:<10} {:<15} {:<15} {:<15} {:<15}".format(
            "Time(s)", "CPU(%)", "Memory(MB)", "FDs", "Threads", "Connections"))
        
        try:
            while not self.stop_event.is_set():
                try:
                    # Get resource information
                    cpu_percent = self.process.cpu_percent()
                    memory_info = self.process.memory_info()
                    memory_mb = memory_info.rss / (1024 * 1024)  # Convert to MB
                    
                    try:
                        open_files = len(self.process.open_files())
                        connections = len(self.process.connections())
                    except psutil.AccessDenied:
                        open_files = -1
                        connections = -1
                        
                    num_threads = self.process.num_threads()
                    
                    # Calculate elapsed time
                    elapsed = time.time() - self.start_time
                    
                    # Store metrics
                    metrics = {
                        'timestamp': time.time(),
                        'elapsed': elapsed,
                        'cpu_percent': cpu_percent,
                        'memory_mb': memory_mb,
                        'open_files': open_files,
                        'connections': connections,
                        'num_threads': num_threads
                    }
                    self.metrics.append(metrics)
                    
                    # Print current resource usage
                    print("{:<10.1f} {:<10.1f} {:<15.2f} {:<15} {:<15} {:<15}".format(
                        elapsed, cpu_percent, memory_mb, open_files, num_threads, connections))
                    
                except psutil.NoSuchProcess:
                    log(f"Process {self.pid} no longer exists", "error")
                    break
                
                # Sleep until next sample
                time.sleep(self.interval)
                
        except Exception as e:
            log(f"Resource monitoring error: {str(e)}", "error")
            if self.stop_event.is_set():
                log("Monitoring thread stopping due to stop event", "info")
            else:
                traceback.print_exc()
    
    def analyze_metrics(self):
        """Analyze collected metrics for signs of resource leaks"""
        if not self.metrics:
            log("No metrics collected", "warning")
            return {}
        
        # Calculate basic statistics
        first = self.metrics[0]
        last = self.metrics[-1]
        duration = last['elapsed'] - first['elapsed']
        
        # Calculate growth rates
        memory_growth = (last['memory_mb'] - first['memory_mb']) / duration if duration > 0 else 0
        fd_growth = (last['open_files'] - first['open_files']) / duration if duration > 0 else 0
        thread_growth = (last['num_threads'] - first['num_threads']) / duration if duration > 0 else 0
        
        # Determine if there's a leak
        has_memory_leak = memory_growth > 0.1  # MB/s
        has_fd_leak = fd_growth > 0.1  # FDs/s
        has_thread_leak = thread_growth > 0.1  # Threads/s
        
        results = {
            'duration': duration,
            'memory': {
                'start': first['memory_mb'],
                'end': last['memory_mb'],
                'growth_rate': memory_growth,
                'has_leak': has_memory_leak
            },
            'file_descriptors': {
                'start': first['open_files'],
                'end': last['open_files'],
                'growth_rate': fd_growth,
                'has_leak': has_fd_leak
            },
            'threads': {
                'start': first['num_threads'],
                'end': last['num_threads'],
                'growth_rate': thread_growth,
                'has_leak': has_thread_leak
            }
        }
        
        return results
    
    def generate_charts(self, output_path=None):
        """Generate charts from the collected metrics"""
        if not self.metrics:
            log("No metrics to chart", "warning")
            return False
        
        try:
            # Extract data for plotting
            elapsed = [m['elapsed'] for m in self.metrics]
            cpu = [m['cpu_percent'] for m in self.metrics]
            memory = [m['memory_mb'] for m in self.metrics]
            fds = [m['open_files'] for m in self.metrics]
            threads = [m['num_threads'] for m in self.metrics]
            connections = [m['connections'] for m in self.metrics]
            
            # Create subplots
            fig, axes = plt.subplots(3, 1, figsize=(10, 12), sharex=True)
            
            # CPU and Memory plot
            ax1 = axes[0]
            ax1.set_title('CPU and Memory Usage')
            ax1.set_ylabel('CPU (%)', color='tab:red')
            ax1.plot(elapsed, cpu, 'tab:red', label='CPU %')
            ax1.tick_params(axis='y', labelcolor='tab:red')
            
            ax1_twin = ax1.twinx()
            ax1_twin.set_ylabel('Memory (MB)', color='tab:blue')
            ax1_twin.plot(elapsed, memory, 'tab:blue', label='Memory (MB)')
            ax1_twin.tick_params(axis='y', labelcolor='tab:blue')
            
            # File Descriptors plot
            ax2 = axes[1]
            ax2.set_title('File Descriptors')
            ax2.plot(elapsed, fds, 'tab:green')
            ax2.set_ylabel('Count')
            
            # Threads and Connections plot
            ax3 = axes[2]
            ax3.set_title('Threads and Connections')
            ax3.plot(elapsed, threads, 'tab:purple', label='Threads')
            ax3.plot(elapsed, connections, 'tab:orange', label='Connections')
            ax3.set_ylabel('Count')
            ax3.set_xlabel('Time (seconds)')
            ax3.legend()
            
            # Adjust layout
            plt.tight_layout()
            
            # Save or show
            if output_path:
                plt.savefig(output_path)
                log(f"Chart saved to {output_path}", "success")
            else:
                plt.show()
            
            return True
        except Exception as e:
            log(f"Error generating charts: {str(e)}", "error")
            traceback.print_exc()
            return False

class ResourceLeakTester:
    """Test for resource leaks in a webserver"""
    
    def __init__(self, args):
        self.args = args
        self.base_url = args.url.rstrip('/')
        self.pid = args.pid
        self.monitor = None
        self.results = {}
        
        # Parse the URL to get host and port
        url_parts = urlparse(self.base_url)
        self.host = url_parts.netloc.split(':')[0] if ':' in url_parts.netloc else url_parts.netloc
        self.port = url_parts.port or (443 if url_parts.scheme == 'https' else 80)
    
    def run_tests(self):
        """Run all resource leak tests"""
        log(f"Starting resource leak tests against {self.base_url}", "info")
        
        # Start resource monitoring if PID provided
        if self.pid:
            self.monitor = ResourceMonitor(self.pid, interval=self.args.interval)
            if not self.monitor.start():
                log("Failed to start resource monitoring", "error")
                return False
        else:
            log("No PID provided, resource monitoring disabled", "warning")
        
        try:
            # Run the selected tests
            if self.args.test == 'all' or 'connection' in self.args.test:
                self.connection_leak_test()
            
            if self.args.test == 'all' or 'request' in self.args.test:
                self.request_leak_test()
            
            if self.args.test == 'all' or 'cgi' in self.args.test:
                self.cgi_leak_test()
            
            if self.args.test == 'all' or 'error' in self.args.test:
                self.error_leak_test()
            
            if self.args.test == 'all' or 'memory' in self.args.test:
                self.memory_leak_test()
            
            # Stop monitoring and analyze results
            if self.monitor:
                time.sleep(2)  # Wait for any remaining resource changes
                self.monitor.stop()
                
                # Analyze resource metrics
                analysis = self.monitor.analyze_metrics()
                self.results['resource_analysis'] = analysis
                
                # Generate charts
                if self.args.output:
                    output_file = os.path.join(self.args.output, "resource_charts.png")
                    self.monitor.generate_charts(output_file)
                
                # Print analysis results
                self._print_analysis_results(analysis)
            
            return True
            
        except KeyboardInterrupt:
            log("Tests interrupted by user", "warning")
            if self.monitor:
                self.monitor.stop()
            return False
            
        except Exception as e:
            log(f"Error during tests: {str(e)}", "error")
            traceback.print_exc()
            if self.monitor:
                self.monitor.stop()
            return False
    
    def _print_analysis_results(self, analysis):
        """Print resource analysis results"""
        print("\n" + "="*60)
        print("Resource Leak Analysis")
        print("="*60)
        
        # Memory analysis
        memory = analysis.get('memory', {})
        memory_status = f"{Colors.RED}POTENTIAL LEAK{Colors.RESET}" if memory.get('has_leak', False) else f"{Colors.GREEN}OK{Colors.RESET}"
        print(f"Memory Usage:")
        print(f"  Start: {memory.get('start', 0):.2f} MB")
        print(f"  End: {memory.get('end', 0):.2f} MB")
        print(f"  Growth Rate: {memory.get('growth_rate', 0):.4f} MB/s")
        print(f"  Status: {memory_status}")
        
        # File descriptor analysis
        fd = analysis.get('file_descriptors', {})
        fd_status = f"{Colors.RED}POTENTIAL LEAK{Colors.RESET}" if fd.get('has_leak', False) else f"{Colors.GREEN}OK{Colors.RESET}"
        print(f"\nFile Descriptors:")
        print(f"  Start: {fd.get('start', 0)}")
        print(f"  End: {fd.get('end', 0)}")
        print(f"  Growth Rate: {fd.get('growth_rate', 0):.4f} FDs/s")
        print(f"  Status: {fd_status}")
        
        # Thread analysis
        threads = analysis.get('threads', {})
        thread_status = f"{Colors.RED}POTENTIAL LEAK{Colors.RESET}" if threads.get('has_leak', False) else f"{Colors.GREEN}OK{Colors.RESET}"
        print(f"\nThreads:")
        print(f"  Start: {threads.get('start', 0)}")
        print(f"  End: {threads.get('end', 0)}")
        print(f"  Growth Rate: {threads.get('growth_rate', 0):.4f} threads/s")
        print(f"  Status: {thread_status}")
        
        # Overall assessment
        has_any_leak = memory.get('has_leak', False) or fd.get('has_leak', False) or threads.get('has_leak', False)
        overall_status = f"{Colors.RED}RESOURCE LEAKS DETECTED{Colors.RESET}" if has_any_leak else f"{Colors.GREEN}NO LEAKS DETECTED{Colors.RESET}"
        print(f"\nOverall Assessment: {overall_status}")
        print("="*60)
    
    def connection_leak_test(self):
        """Test for connection resource leaks"""
        test_name = "connection_leak"
        log(f"Starting {test_name} test", "info")
        
        num_connections = self.args.connections
        connections = []
        log(f"Opening {num_connections} connections and holding them open", "info")
        
        try:
            # Open connections
            for i in range(num_connections):
                try:
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(5)
                    sock.connect((self.host, self.port))
                    
                    # Send a minimal HTTP request to keep the connection alive
                    http_request = f"GET / HTTP/1.1\r\nHost: {self.host}\r\nConnection: keep-alive\r\n\r\n"
                    sock.send(http_request.encode())
                    
                    # Receive initial response
                    sock.recv(4096)
                    
                    # Add to our list of open connections
                    connections.append(sock)
                    
                    if (i+1) % 10 == 0:
                        log(f"Opened {i+1}/{num_connections} connections", "info")
                        
                except Exception as e:
                    log(f"Failed to open connection {i+1}: {str(e)}", "error")
                    break
            
            # Hold connections open
            total_connections = len(connections)
            if total_connections > 0:
                log(f"Successfully opened {total_connections} connections, holding for {self.args.hold_time} seconds", "success")
                time.sleep(self.args.hold_time)
                
                # Send data on each connection to verify they're still active
                active_connections = 0
                for i, sock in enumerate(connections):
                    try:
                        ping_request = f"GET /ping HTTP/1.1\r\nHost: {self.host}\r\n\r\n"
                        sock.send(ping_request.encode())
                        response = sock.recv(4096)
                        if response:
                            active_connections += 1
                    except:
                        pass
                
                log(f"{active_connections}/{total_connections} connections still active after {self.args.hold_time}s", "info")
            else:
                log("Failed to open any connections", "error")
        
        finally:
            # Close all connections
            for sock in connections:
                try:
                    sock.close()
                except:
                    pass
            
            log(f"Closed all {len(connections)} connections", "info")
            
            # Wait to see if resources are properly cleaned up
            log(f"Waiting {self.args.cleanup_time} seconds to check resource cleanup", "info")
            time.sleep(self.args.cleanup_time)
        
        log(f"Completed {test_name} test", "success")
    
    def request_leak_test(self):
        """Test for leaks caused by normal HTTP requests"""
        test_name = "request_leak"
        log(f"Starting {test_name} test", "info")
        
        num_requests = self.args.requests
        concurrent = min(20, num_requests)
        log(f"Sending {num_requests} HTTP requests ({concurrent} concurrently)", "info")
        
        # Create various paths to request
        paths = [
            "/",
            "/index.html",
            "/nonexistent",
            "/this/path/does/not/exist",
            "/very/long/path/to/test/buffer/handling/and/memory/usage/in/the/server"
        ]
        
        results = {'success': 0, 'failure': 0}
        
        def worker(thread_id, requests_per_thread):
            local_success = 0
            local_failure = 0
            
            for i in range(requests_per_thread):
                path = paths[i % len(paths)]
                url = urljoin(self.base_url, path)
                
                try:
                    response = requests.get(url, timeout=self.args.timeout)
                    
                    if response.status_code < 500:  # Any response that's not a server error
                        local_success += 1
                    else:
                        local_failure += 1
                        
                except Exception as e:
                    local_failure += 1
                
                # Small delay to avoid overwhelming the server
                time.sleep(0.01)
            
            return local_success, local_failure
        
        # Create and start worker threads
        threads = []
        requests_per_thread = num_requests // concurrent
        remaining = num_requests % concurrent
        
        for i in range(concurrent):
            # Distribute any remaining requests among the first few threads
            thread_requests = requests_per_thread + (1 if i < remaining else 0)
            thread = threading.Thread(target=lambda: results['success'] += worker(i, thread_requests)[0] or results['failure'] += worker(i, thread_requests)[1])
            threads.append(thread)
            thread.start()
        
        # Wait for all threads to complete
        for thread in threads:
            thread.join()
        
        success_rate = (results['success'] / num_requests) * 100 if num_requests > 0 else 0
        log(f"Completed {results['success'] + results['failure']}/{num_requests} requests - Success rate: {success_rate:.1f}%", 
            "success" if success_rate > 90 else "warning")
        
        # Wait to check resource cleanup
        log(f"Waiting {self.args.cleanup_time} seconds to check resource cleanup", "info")
        time.sleep(self.args.cleanup_time)
        
        log(f"Completed {test_name} test", "success")
    
    def cgi_leak_test(self):
        """Test for leaks caused by CGI script execution"""
        test_name = "cgi_leak"
        log(f"Starting {test_name} test", "info")
        
        # CGI test paths
        cgi_paths = [
            "/cgi-bin/basic.py",
            "/cgi-bin/slow.py",
            "/cgi-bin/error.py"
        ]
        
        num_requests = self.args.requests
        concurrent = min(10, num_requests)
        log(f"Sending {num_requests} CGI requests ({concurrent} concurrently)", "info")
        
        results = {'success': 0, 'failure': 0}
        
        def worker(thread_id, requests_per_thread):
            local_success = 0
            local_failure = 0
            
            for i in range(requests_per_thread):
                path = cgi_paths[i % len(cgi_paths)]
                url = urljoin(self.base_url, path)
                
                try:
                    response = requests.get(url, timeout=self.args.timeout)
                    
                    if response.status_code < 500:  # Any response that's not a server error
                        local_success += 1
                    else:
                        local_failure += 1
                        
                except Exception as e:
                    local_failure += 1
                
                # Small delay to avoid overwhelming the server
                time.sleep(0.05)
            
            return local_success, local_failure
        
        # Create and start worker threads
        with ThreadPoolExecutor(max_workers=concurrent) as executor:
            futures = []
            requests_per_thread = num_requests // concurrent
            remaining = num_requests % concurrent
            
            for i in range(concurrent):
                # Distribute any remaining requests among the first few threads
                thread_requests = requests_per_thread + (1 if i < remaining else 0)
                future = executor.submit(worker, i, thread_requests)
                futures.append(future)
            
            # Collect results
            for future in futures:
                success, failure = future.result()
                results['success'] += success
                results['failure'] += failure
        
        success_rate = (results['success'] / num_requests) * 100 if num_requests > 0 else 0
        log(f"Completed {results['success'] + results['failure']}/{num_requests} CGI requests - Success rate: {success_rate:.1f}%", 
            "success" if success_rate > 90 else "warning")
        
        # Wait to check resource cleanup
        log(f"Waiting {self.args.cleanup_time} seconds to check resource cleanup", "info")
        time.sleep(self.args.cleanup_time)
        
        log(f"Completed {test_name} test", "success")
    
    def error_leak_test(self):
        """Test for leaks caused by error conditions"""
        test_name = "error_leak"
        log(f"Starting {test_name} test", "info")
        
        num_requests = self.args.requests
        log(f"Sending {num_requests} requests designed to trigger errors", "info")
        
        # Error-inducing requests
        error_requests = [
            # 1. Very long URL to test buffer handling
            ("GET", "/extremely/long/path/that/exceeds/typical/buffer/sizes" + "a" * 1000),
            
            # 2. Malformed HTTP request
            ("BAD", "/"),
            
            # 3. Invalid HTTP version
            ("GET", "/", "HTTP/9.9"),
            
            # 4. Very large headers
            ("GET", "/", "HTTP/1.1", {"X-Large-Header": "X" * 8192}),
            
            # 5. Aborted request (connect and close immediately)
            ("ABORT", "/"),
            
            # 6. Request with invalid Content-Length
            ("POST", "/", "HTTP/1.1", {"Content-Length": "not_a_number"}),
            
            # 7. CGI script that causes errors
            ("GET", "/cgi-bin/error.py"),
            
            # 8. Invalid request with special characters
            ("GET", "/%%invalid%%"),
        ]
        
        results = {'success': 0, 'failure': 0}
        
        for i in range(num_requests):
            # Select an error request type
            req_type = error_requests[i % len(error_requests)]
            method = req_type[0]
            path = req_type[1]
            http_version = req_type[2] if len(req_type) > 2 else "HTTP/1.1"
            headers = req_type[3] if len(req_type) > 3 else {}
            
            try:
                if method == "ABORT":
                    # Special case: Connect and close immediately
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(2)
                    sock.connect((self.host, self.port))
                    sock.close()
                    results['success'] += 1
                elif method == "BAD":
                    # Send a malformed request
                    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    sock.settimeout(2)
                    sock.connect((self.host, self.port))
                    sock.send(b"THIS IS NOT A VALID HTTP REQUEST\r\n\r\n")
                    try:
                        sock.recv(4096)
                    except:
                        pass
                    sock.close()
                    results['success'] += 1
                else:
                    # Use the requests library for other request types
                    url = urljoin(self.base_url, path)
                    if method == "GET":
                        response = requests.get(url, headers=headers, timeout=self.args.timeout)
                        results['success'] += 1
                    elif method == "POST":
                        response = requests.post(url, headers=headers, timeout=self.args.timeout)
                        results['success'] += 1
                    else:
                        # Custom method
                        session = requests.Session()
                        request = requests.Request(method, url, headers=headers)
                        prepared = request.prepare()
                        response = session.send(prepared, timeout=self.args.timeout)
                        results['success'] += 1
            except Exception as e:
                results['failure'] += 1
            
            # Progress update
            if (i+1) % 10 == 0:
                log(f"Completed {i+1}/{num_requests} error requests", "info")
            
            # Small delay between requests
            time.sleep(0.01)
        
        # Report results
        log(f"Completed {results['success'] + results['failure']}/{num_requests} error requests", "info")
        
        # Wait to check resource cleanup
        log(f"Waiting {self.args.cleanup_time} seconds to check resource cleanup", "info")
        time.sleep(self.args.cleanup_time)
        
        log(f"Completed {test_name} test", "success")
    
    def memory_leak_test(self):
        """Test for memory leaks with large responses and requests"""
        test_name = "memory_leak"
        log(f"Starting {test_name} test", "info")
        
        num_requests = self.args.requests
        log(f"Sending {num_requests} requests with large data", "info")
        
        # Test with large responses and requests
        large_urls = [
            "/cgi-bin/large.py",  # CGI script that generates a large response
        ]
        
        # Create a large POST request
        large_data = "X" * (1024 * 1024)  # 1MB of data
        
        results = {'success': 0, 'failure': 0}
        
        for i in range(num_requests):
            try:
                if i % 2 == 0:
                    # Large response test
                    url = urljoin(self.base_url, large_urls[0])
                    response = requests.get(url, timeout=self.args.timeout)
                    if response.status_code < 500:
                        results['success'] += 1
                    else:
                        results['failure'] += 1
                else:
                    # Large request test
                    url = urljoin(self.base_url, "/")
                    response = requests.post(url, data=large_data, timeout=self.args.timeout)
                    if response.status_code < 500:
                        results['success'] += 1
                    else:
                        results['failure'] += 1
            except Exception as e:
                results['failure'] += 1
            
            # Progress update
            if (i+1) % 10 == 0:
                log(f"Completed {i+1}/{num_requests} large data requests", "info")
            
            # Small delay between requests
            time.sleep(0.05)
        
        # Report results
        log(f"Completed {results['success'] + results['failure']}/{num_requests} large data requests", "info")
        
        # Wait to check resource cleanup
        log(f"Waiting {self.args.cleanup_time} seconds to check resource cleanup", "info")
        time.sleep(self.args.cleanup_time)
        
        log(f"Completed {test_name} test", "success")

def main():
    parser = argparse.ArgumentParser(description="WebServer Resource Leak Tester")
    
    # Server configuration
    parser.add_argument("--url", default="http://localhost:8080", help="Base URL of the server to test")
    parser.add_argument("--pid", type=int, default=0, help="PID of the server process to monitor (0 to skip)")
    
    # Test configuration
    parser.add_argument("--test", default="all", help="Test to run (all, connection, request, cgi, error, memory)")
    parser.add_argument("--requests", type=int, default=100, help="Number of requests for request tests")
    parser.add_argument("--connections", type=int, default=100, help="Number of connections for connection test")
    parser.add_argument("--hold-time", type=int, default=10, help="Time to hold connections open (seconds)")
    parser.add_argument("--cleanup-time", type=int, default=5, help="Time to wait for resource cleanup (seconds)")
    parser.add_argument("--timeout", type=int, default=10, help="Request timeout (seconds)")
    
    # Monitoring configuration
    parser.add_argument("--interval", type=float, default=0.5, help="Resource monitoring interval (seconds)")
    parser.add_argument("--output", default="", help="Directory to store output files (charts, data)")
    
    args = parser.parse_args()
    
    # Create output directory if specified
    if args.output and not os.path.exists(args.output):
        try:
            os.makedirs(args.output)
        except Exception as e:
            log(f"Failed to create output directory: {str(e)}", "error")
            return 1
    
    # Run tests
    tester = ResourceLeakTester(args)
    success = tester.run_tests()
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())