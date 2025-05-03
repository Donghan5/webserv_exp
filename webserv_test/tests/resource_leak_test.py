#!/usr/bin/env python3
import requests
import time
import threading
import argparse
import sys
import os
import psutil
import socket
import resource

def monitor_server_resources(pid, interval=0.5, duration=30):
    """Monitor the resource usage of a process for a specified duration."""
    process = psutil.Process(pid)
    start_time = time.time()
    
    # Headers for the resource usage report
    print("\nResource Monitoring:")
    print("{:<10} {:<10} {:<15} {:<15} {:<15}".format(
        "Time(s)", "CPU(%)", "Memory(MB)", "FDs", "Threads"))
    
    try:
        while time.time() - start_time < duration:
            # Get resource information
            cpu_percent = process.cpu_percent()
            memory_info = process.memory_info()
            memory_mb = memory_info.rss / (1024 * 1024)  # Convert to MB
            
            try:
                num_fds = len(process.open_files()) + len(process.connections())
            except psutil.AccessDenied:
                num_fds = "N/A"
                
            num_threads = process.num_threads()
            
            # Print current resource usage
            elapsed = time.time() - start_time
            print("{:<10.1f} {:<10.1f} {:<15.2f} {:<15} {:<15}".format(
                elapsed, cpu_percent, memory_mb, num_fds, num_threads))
            
            time.sleep(interval)
    
    except psutil.NoSuchProcess:
        print(f"Process {pid} no longer exists.")
    except KeyboardInterrupt:
        print("\nMonitoring interrupted.")
    
    print("\nResource monitoring completed.")

def sequential_requests(url, num_requests, delay=0.05):
    """Make sequential requests to the server with a controllable delay."""
    print(f"\nStarting sequential test with {num_requests} requests (delay: {delay}s)")
    success = 0
    failure = 0
    
    paths = [
        "/",
        "/about.html",
        "/css/style.css",
        "/js/script.js",
        "/cgi-bin/test.py",  # CGI scripts seem particularly problematic
        "/nonexistent"       # Test 404 handling
    ]
    
    start_time = time.time()
    
    for i in range(num_requests):
        path = paths[i % len(paths)]
        try:
            session = requests.Session()
            r = session.get(f"{url}{path}", timeout=5)
            session.close()
            
            if r.status_code < 400:
                success += 1
            else:
                failure += 1
                
            if (i + 1) % 10 == 0:
                print(f"Completed {i + 1}/{num_requests} requests. Success: {success}, Failure: {failure}")
                
            if delay > 0:
                time.sleep(delay)
                
        except Exception as e:
            failure += 1
            print(f"Request {i + 1} failed: {str(e)}")
    
    total_time = time.time() - start_time
    
    print("\nSequential Test Results:")
    print(f"Total time: {total_time:.2f} seconds")
    print(f"Requests per second: {num_requests / total_time:.2f}")
    print(f"Successful requests: {success} ({success / num_requests * 100:.2f}%)")
    print(f"Failed requests: {failure} ({failure / num_requests * 100:.2f}%)")

def concurrent_connections(url, max_connections, hold_time=5):
    """Open and hold multiple concurrent connections to test fd management."""
    print(f"\nTesting {max_connections} concurrent open connections (holding for {hold_time}s)")
    
    connections = []
    success = 0
    
    try:
        for i in range(max_connections):
            try:
                # Create a raw socket connection
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2)
                
                # Parse the URL to get host and port
                if url.startswith('http://'):
                    url = url[7:]
                elif url.startswith('https://'):
                    url = url[8:]
                
                host_port = url.split('/')
                host = host_port[0].split(':')[0]
                port = int(host_port[0].split(':')[1]) if ':' in host_port[0] else 80
                
                # Connect to the server
                sock.connect((host, port))
                
                # Send a minimal HTTP request
                request = f"GET / HTTP/1.1\r\nHost: {host}\r\n\r\n"
                sock.send(request.encode())
                
                # Add to our list of open connections
                connections.append(sock)
                success += 1
                
                if (i + 1) % 10 == 0:
                    print(f"Opened {i + 1}/{max_connections} connections")
                
            except Exception as e:
                print(f"Connection {i + 1} failed: {str(e)}")
        
        print(f"Successfully opened {success} connections. Holding for {hold_time} seconds...")
        time.sleep(hold_time)
        
    finally:
        # Clean up all connections
        for sock in connections:
            try:
                sock.close()
            except:
                pass
        
        print(f"Closed all {len(connections)} connections")

def cgi_specific_test(url, num_requests, parallel=5):
    """Test specifically CGI handling which seems to be problematic."""
    print(f"\nTesting CGI handling with {num_requests} requests ({parallel} in parallel)")
    
    cgi_paths = [
        "/cgi-bin/test.py", 
        "/cgi-bin/env.py"
    ]
    
    def worker(thread_id, requests_per_thread):
        success = 0
        failure = 0
        
        for i in range(requests_per_thread):
            path = cgi_paths[i % len(cgi_paths)]
            try:
                session = requests.Session()
                start_time = time.time()
                r = session.get(f"{url}{path}", timeout=10)
                elapsed = time.time() - start_time
                session.close()
                
                if r.status_code < 400:
                    success += 1
                else:
                    failure += 1
                    print(f"Thread {thread_id}, Request {i+1}: Got status {r.status_code}")
                    
                # Brief delay to avoid overwhelming the server
                time.sleep(0.1)
                    
            except Exception as e:
                failure += 1
                print(f"Thread {thread_id}, Request {i+1} failed: {str(e)}")
        
        return success, failure
    
    start_time = time.time()
    threads = []
    requests_per_thread = num_requests // parallel
    
    # Create and start threads
    for i in range(parallel):
        thread = threading.Thread(target=worker, args=(i, requests_per_thread))
        threads.append(thread)
        thread.start()
    
    # Wait for all threads to complete
    results = []
    for thread in threads:
        thread.join()
    
    total_time = time.time() - start_time
    
    print("\nCGI Test Results:")
    print(f"Total time: {total_time:.2f} seconds")
    print(f"Requests per second: {num_requests / total_time:.2f}")

def connection_recycling_test(url, num_requests, reuse_connection=True):
    """Test if the issue is related to connection reuse/recycling."""
    print(f"\nTesting connection {'reuse' if reuse_connection else 'creation'} with {num_requests} requests")
    
    success = 0
    failure = 0
    path = "/"
    
    # Create a single session if we're reusing the connection
    session = requests.Session() if reuse_connection else None
    
    start_time = time.time()
    
    for i in range(num_requests):
        try:
            if reuse_connection:
                r = session.get(f"{url}{path}", timeout=5)
            else:
                # Create a new session for each request
                temp_session = requests.Session()
                r = temp_session.get(f"{url}{path}", timeout=5)
                temp_session.close()
            
            if r.status_code < 400:
                success += 1
            else:
                failure += 1
                
            if (i + 1) % 10 == 0:
                print(f"Completed {i + 1}/{num_requests} requests. Success: {success}, Failure: {failure}")
                
        except Exception as e:
            failure += 1
            print(f"Request {i + 1} failed: {str(e)}")
    
    # Close the session if we were reusing it
    if reuse_connection:
        session.close()
    
    total_time = time.time() - start_time
    
    print(f"\nConnection {'Reuse' if reuse_connection else 'Creation'} Test Results:")
    print(f"Total time: {total_time:.2f} seconds")
    print(f"Requests per second: {num_requests / total_time:.2f}")
    print(f"Successful requests: {success} ({success / num_requests * 100:.2f}%)")
    print(f"Failed requests: {failure} ({failure / num_requests * 100:.2f}%)")

def main():
    parser = argparse.ArgumentParser(description="Test resource management in a web server")
    parser.add_argument("--url", default="http://localhost:8080", help="Base URL of the server to test")
    parser.add_argument("--pid", type=int, default=0, help="PID of the server process to monitor (0 to skip)")
    parser.add_argument("--test", choices=["all", "sequential", "concurrent", "cgi", "connection"], 
                        default="all", help="Test to run")
    parser.add_argument("--requests", type=int, default=50, help="Number of requests for sequential/CGI tests")
    parser.add_argument("--connections", type=int, default=100, help="Max concurrent connections to test")
    parser.add_argument("--hold-time", type=int, default=5, help="Time to hold connections open (seconds)")
    args = parser.parse_args()
    
    try:
        # Check if server is running
        try:
            requests.get(args.url, timeout=2)
        except requests.exceptions.ConnectionError:
            print(f"ERROR: Could not connect to {args.url}")
            sys.exit(1)
        
        # Set higher limits for file descriptors if possible
        try:
            soft, hard = resource.getrlimit(resource.RLIMIT_NOFILE)
            resource.setrlimit(resource.RLIMIT_NOFILE, (min(8192, hard), hard))
            print(f"Increased file descriptor limit from {soft} to {min(8192, hard)}")
        except Exception as e:
            print(f"Note: Could not increase file descriptor limit: {e}")
        
        # Start a monitoring thread if PID was provided
        if args.pid > 0:
            monitor_thread = threading.Thread(
                target=monitor_server_resources, 
                args=(args.pid, 0.5, 60),
                daemon=True
            )
            monitor_thread.start()
        
        # Run selected tests
        if args.test in ["all", "sequential"]:
            sequential_requests(args.url, args.requests)
        
        if args.test in ["all", "concurrent"]:
            concurrent_connections(args.url, args.connections, args.hold_time)
        
        if args.test in ["all", "cgi"]:
            cgi_specific_test(args.url, args.requests)
            
        if args.test in ["all", "connection"]:
            # Test with connection reuse
            connection_recycling_test(args.url, args.requests, True)
            # Test with new connection per request
            connection_recycling_test(args.url, args.requests, False)
        
        # Wait for monitoring thread to complete
        if args.pid > 0:
            monitor_thread.join(timeout=1)
        
    except KeyboardInterrupt:
        print("\nTests interrupted!")
        sys.exit(0)
    except Exception as e:
        print(f"ERROR: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
