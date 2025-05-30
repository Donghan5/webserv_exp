#!/usr/bin/env python3

import os, sys
import cgitb
import time
import mimetypes
import re
import traceback
import tempfile
import json

# Set up project paths
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEBUG_DIR = os.path.join(PROJECT_ROOT, 'www/cgi-bin/tmp')
if not os.path.exists(DEBUG_DIR):
    os.makedirs(DEBUG_DIR, exist_ok=True)

# Enable CGI traceback
cgitb.enable(display=0, logdir=DEBUG_DIR)

def log_progress(message, filename="upload_progress.log"):
    """Log progress information with timestamp."""
    progress_log_path = os.path.join(DEBUG_DIR, filename)
    with open(progress_log_path, 'a') as f:
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        f.write(f"[{timestamp}] {message}\n")

# Initialize log
log_progress("========== NEW UPLOAD REQUEST ==========")
log_progress(f"Project root: {PROJECT_ROOT}")

# Debug environmental information
debug_info = "\n--- CGI env ---\n"
for key, value in os.environ.items():
    debug_info += f"{key}: {value}\n"

# Process multipart form data
content_type = os.environ.get('CONTENT_TYPE', '')
content_length = int(os.environ.get('CONTENT_LENGTH', '0'))

debug_info += "\n--- DIRECT FORM PROCESSING ---\n"
debug_info += f"Content-Type: {content_type}\n"
debug_info += f"Content-Length: {content_length}\n"

# Extract boundary
boundary = None
if 'boundary=' in content_type:
    boundary = '--' + content_type.split('boundary=')[1].strip()
    debug_info += f"Detected boundary: {boundary}\n"
else:
    debug_info += "No boundary detected in Content-Type\n"

uploaded_file = None
filename = None
file_content = None

if boundary:
    try:
        log_progress("Reading raw POST data from stdin")
        # Read all data from stdin
        post_data = sys.stdin.buffer.read(content_length)
        log_progress(f"Read {len(post_data)} bytes from stdin")
        debug_info += f"Raw data size: {len(post_data)} bytes\n"

        if len(post_data) > 0:
            # Split data by boundary
            parts = post_data.split(boundary.encode('utf-8'))
            debug_info += f"Split into {len(parts)} parts\n"

            # Process each part
            for part in parts:
                # Skip empty parts or boundary end markers
                if not part or part == b'--\r\n' or part == b'--':
                    continue

                # Split headers from body
                try:
                    headers_end = part.find(b'\r\n\r\n')
                    if headers_end > 0:
                        headers = part[:headers_end].decode('utf-8', errors='ignore')
                        body = part[headers_end + 4:]  # +4 for '\r\n\r\n'

                        # Look for file upload parts
                        if 'filename=' in headers:
                            log_progress("Found file part in multipart data")
                            # Extract filename
                            filename_match = re.search(r'filename="([^"]*)"', headers)
                            if filename_match:
                                filename = filename_match.group(1)
                                log_progress(f"Extracted filename: {filename}")

                                # Extract form field name
                                name_match = re.search(r'name="([^"]*)"', headers)
                                if name_match:
                                    form_name = name_match.group(1)
                                    log_progress(f"Form field name: {form_name}")

                                # Extract Content-Type
                                content_type_match = re.search(r'Content-Type: ([^\r\n]*)', headers)
                                file_type = "application/octet-stream"
                                if content_type_match:
                                    file_type = content_type_match.group(1)
                                    log_progress(f"File content type: {file_type}")

                                # Store file content
                                file_content = body

                                # Remove trailing \r\n if present
                                if file_content.endswith(b'\r\n'):
                                    file_content = file_content[:-2]

                                log_progress(f"Extracted file content: {len(file_content)} bytes")
                                debug_info += f"File field: {form_name}, Filename: {filename}, Content-Type: {file_type}, Size: {len(file_content)}\n"

                                if len(file_content) > 100:
                                    debug_info += f"File content first 100 bytes (hex): {file_content[:100].hex()[:200]}\n"
                except Exception as e:
                    log_progress(f"Error parsing part: {str(e)}")
                    debug_info += f"Error parsing part: {str(e)}\n"
                    debug_info += traceback.format_exc() + "\n"
        else:
            debug_info += "No data received from stdin\n"
    except Exception as e:
        log_progress(f"Error processing multipart data: {str(e)}")
        debug_info += f"Error processing multipart data: {str(e)}\n"
        debug_info += traceback.format_exc() + "\n"
else:
    debug_info += "Cannot process form data without boundary\n"

# Save debug information
with open(os.path.join(DEBUG_DIR, 'debug.log'), 'w') as f:
    f.write(debug_info)

def get_file_size_str(size_bytes):
    """Convert file size in bytes to human-readable format."""
    if size_bytes < 1024:
        return f"{size_bytes} bytes"
    elif size_bytes < 1024 * 1024:
        return f"{size_bytes/1024:.2f} KB"
    elif size_bytes < 1024 * 1024 * 1024:
        return f"{size_bytes/(1024*1024):.2f} MB"
    else:
        return f"{size_bytes/(1024*1024*1024):.2f} GB"

# HTML template start
html_start = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>File Upload Result</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f9f9f9;
        }
        .result-card {
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            padding: 20px;
            margin-top: 20px;
        }
        .success {
            border-left: 5px solid #4CAF50;
        }
        .error {
            border-left: 5px solid #f44336;
        }
        h1 {
            color: #2c3e50;
            margin-top: 0;
        }
        .file-info {
            background-color: #f5f5f5;
            border-radius: 4px;
            padding: 15px;
            margin: 15px 0;
        }
        .file-preview {
            margin: 20px 0;
            text-align: center;
        }
        .file-preview img {
            max-width: 100%;
            max-height: 300px;
            border-radius: 4px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        }
        .btn {
            display: inline-block;
            background-color: #3498db;
            color: white;
            padding: 10px 15px;
            text-decoration: none;
            border-radius: 4px;
            font-weight: 500;
            margin-top: 15px;
            transition: background-color 0.3s;
        }
        .btn:hover {
            background-color: #2980b9;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 15px 0;
        }
        th, td {
            padding: 10px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background-color: #f2f2f2;
        }
        .debug-info {
            margin-top: 20px;
            border: 1px solid #ddd;
            border-radius: 4px;
            padding: 10px;
        }
        .debug-info h3 {
            margin-top: 0;
            color: #666;
            font-size: 16px;
        }
        details summary {
            cursor: pointer;
            color: #3498db;
            font-weight: 500;
            padding: 5px 0;
        }
        details summary:hover {
            color: #2980b9;
        }
        pre {
            white-space: pre-wrap;
            word-break: break-all;
            font-family: monospace;
            font-size: 12px;
            line-height: 1.4;
        }
    </style>
</head>
<body>
"""

# HTML template end
html_end = """
    <a href="/" class="btn">Back to Home</a>
</body>
</html>
"""

try:
    # Get upload directory from env or use default
    upload_store = os.environ.get('UPLOAD_STORE', '')
    if not upload_store:
        upload_store = os.path.join(PROJECT_ROOT, 'www/uploads')

    log_progress(f"Using upload directory: {upload_store}")

    # Create upload directory if it doesn't exist
    if not os.path.exists(upload_store):
        os.makedirs(upload_store, exist_ok=True)
        log_progress(f"Created upload directory: {upload_store}")
    else:
        log_progress(f"Upload directory exists: {upload_store}")

    # Debug upload directory status
    upload_dir_debug = {
        "directory": upload_store,
        "exists": os.path.exists(upload_store),
        "is_writable": os.access(upload_store, os.W_OK) if os.path.exists(upload_store) else False,
        "parent_exists": os.path.exists(os.path.dirname(upload_store)),
        "parent_is_writable": os.access(os.path.dirname(upload_store), os.W_OK) if os.path.exists(os.path.dirname(upload_store)) else False
    }

    with open(os.path.join(DEBUG_DIR, 'upload_dir_debug.log'), 'w') as f:
        for key, value in upload_dir_debug.items():
            f.write(f"{key}: {value}\n")

    if not filename or not file_content:
        log_progress("Error: No file was provided in the form")
        print("Content-Type: text/html\r\n\r\n")
        print(html_start)
        print('<div class="result-card error">')
        print('<h1>⚠️ Upload Error</h1>')
        print('<p>No file was provided or could not be processed. Please select a file to upload.</p>')
        print('</div>')
        print(html_end)
    else:
        log_progress(f"Successfully extracted file: {filename}")

        # Extract basename only (remove paths)
        if '\\' in filename:
            filename = filename.split('\\')[-1]
        if '/' in filename:
            filename = filename.split('/')[-1]

        log_progress(f"Using basename: {filename}")

        file_size = len(file_content)
        log_progress(f"File content size: {file_size} bytes")
        file_type = mimetypes.guess_type(filename)[0] or "application/octet-stream"
        log_progress(f"File type detected: {file_type}")

        filepath = os.path.join(upload_store, filename)

        # Log upload attempt info
        upload_attempt_info = {
            "time": time.strftime("%Y-%m-%d %H:%M:%S"),
            "filename": filename,
            "filepath": filepath,
            "file_size": file_size,
            "file_type": file_type,
            "upload_dir_exists": os.path.exists(upload_store),
            "upload_dir_writable": os.access(upload_store, os.W_OK) if os.path.exists(upload_store) else False
        }

        with open(os.path.join(DEBUG_DIR, 'upload_attempt.log'), 'a') as f:
            f.write("\n--- New Upload Attempt ---\n")
            for key, value in upload_attempt_info.items():
                f.write(f"{key}: {value}\n")

        # Save the file in chunks and log progress
        log_progress(f"Starting to write file to: {filepath}")
        try:
            with open(filepath, 'wb') as f:
                chunk_size = 4096  # 4KB chunks
                total_written = 0

                # Write file in chunks to track progress
                for i in range(0, len(file_content), chunk_size):
                    chunk = file_content[i:i+chunk_size]
                    f.write(chunk)
                    total_written += len(chunk)

                    # Log progress at 10% intervals
                    if file_size > 0:
                        percent = (total_written * 100) // file_size
                        if percent % 10 == 0 and (total_written - len(chunk)) * 100 // file_size < percent:
                            log_progress(f"Writing progress: {percent}% ({total_written}/{file_size} bytes)")

            # Confirm write completion
            log_progress(f"File successfully written to disk: {filepath}")
        except Exception as e:
            log_progress(f"Error during file writing: {str(e)}")
            log_progress(traceback.format_exc())
            raise

        # Verify file was saved correctly
        upload_success_info = {
            "time": time.strftime("%Y-%m-%d %H:%M:%S"),
            "file_saved": os.path.exists(filepath),
            "actual_size": os.path.getsize(filepath) if os.path.exists(filepath) else 0,
            "expected_size": file_size,
            "size_match": (os.path.getsize(filepath) == file_size) if os.path.exists(filepath) else False
        }

        with open(os.path.join(DEBUG_DIR, 'upload_success.log'), 'a') as f:
            f.write("\n--- Upload Status ---\n")
            for key, value in upload_success_info.items():
                f.write(f"{key}: {value}\n")

        # Generate HTML response
        print("Content-Type: text/html\r\n\r\n")
        print(html_start)
        print('<div class="result-card success">')
        print(f'<h1>✅ File Upload Successful</h1>')
        print(f'<p>Your file has been uploaded successfully.</p>')

        # Show debug info
        print('<div class="debug-info">')
        print('<h3>Debug Information</h3>')
        print('<details>')
        print('<summary>Click to view upload progress logs</summary>')
        print('<pre style="background-color: #f5f5f5; padding: 10px; border-radius: 4px; max-height: 200px; overflow-y: auto;">')

        # Display progress logs
        log_path = os.path.join(DEBUG_DIR, 'upload_progress.log')
        if os.path.exists(log_path):
            with open(log_path, 'r') as f:
                log_content = f.read()
            print(log_content.replace('<', '&lt;').replace('>', '&gt;'))
        else:
            print("No progress log available.")

        print('</pre>')
        print('</details>')
        print('</div>')

        # Show file info table
        print('<div class="file-info">')
        print('<table>')
        print(f'<tr><th>File Name</th><td>{filename}</td></tr>')
        print(f'<tr><th>File Size</th><td>{get_file_size_str(file_size)}</td></tr>')
        print(f'<tr><th>File Type</th><td>{file_type}</td></tr>')
        print(f'<tr><th>Upload Time</th><td>{time.strftime("%Y-%m-%d %H:%M:%S")}</td></tr>')
        print(f'<tr><th>Saved Path</th><td>{filepath}</td></tr>')
        print('</table>')
        print('</div>')

        # Show image preview if it's an image
        if file_type and file_type.startswith('image/'):
            print('<div class="file-preview">')
            print('<h3>File Preview</h3>')

            # Save a copy for preview in the debug directory
            preview_filename = filename
            preview_path = os.path.join(DEBUG_DIR, preview_filename)
            with open(preview_path, 'wb') as f:
                f.write(file_content)

            # Log preview info
            with open(os.path.join(DEBUG_DIR, 'image_preview.log'), 'a') as f:
                f.write(f"\n--- Image Preview Info ---\n")
                f.write(f"Actual file path: {filepath}\n")
                f.write(f"Preview path: {preview_path}\n")
                f.write(f"Preview URL: /cgi-bin/tmp/{preview_filename}\n")
                f.write(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")

            print(f'<img src="/cgi-bin/tmp/{preview_filename}" alt="{filename}">')
            print('</div>')

        print('</div>')
        print(html_end)

except Exception as e:
    # Log error info
    log_progress(f"ERROR: {type(e).__name__}: {str(e)}")
    error_info = {
        "time": time.strftime("%Y-%m-%d %H:%M:%S"),
        "error_type": type(e).__name__,
        "error_message": str(e),
        "upload_dir": upload_store if 'upload_store' in locals() else "Not defined",
        "upload_dir_exists": os.path.exists(upload_store) if 'upload_store' in locals() else "Unknown",
        "traceback": cgitb.text(sys.exc_info())
    }

    with open(os.path.join(DEBUG_DIR, 'error.log'), 'a') as f:
        f.write("\n--- New Error ---\n")
        for key, value in error_info.items():
            if key != "traceback":  # Save traceback separately
                f.write(f"{key}: {value}\n")
        f.write("\n--- Traceback ---\n")
        f.write(error_info["traceback"])

    # Generate error response
    print("Content-Type: text/html\r\n\r\n")
    print(html_start)
    print('<div class="result-card error">')
    print('<h1>❌ Upload Failed</h1>')
    print(f'<p>An error occurred during the upload process: {str(e)}</p>')
    print('<p>Please try again or contact the administrator if the problem persists.</p>')
    print(f'<p>Error details have been logged to: {os.path.join(DEBUG_DIR, "error.log")}</p>')

    # Show debug logs
    print('<div class="debug-info">')
    print('<h3>Debug Progress Logs</h3>')
    print('<details>')
    print('<summary>Click to view upload progress logs</summary>')
    print('<pre style="background-color: #f5f5f5; padding: 10px; border-radius: 4px; max-height: 200px; overflow-y: auto;">')

    # Display progress logs
    log_path = os.path.join(DEBUG_DIR, 'upload_progress.log')
    if os.path.exists(log_path):
        with open(log_path, 'r') as f:
            log_content = f.read()
        print(log_content.replace('<', '&lt;').replace('>', '&gt;'))
    else:
        print("No progress log available.")

    print('</pre>')
    print('</details>')
    print('</div>')

    print('</div>')
    print(html_end)
