# #!/usr/bin/python3

# import os
# import sys
# import time

# # Create directory for debug logs
# log_dir = './www/cgi-bin/tmp'
# if not os.path.exists(log_dir):
#     os.makedirs(log_dir)

# # Log environment variables
# with open(os.path.join(log_dir, 'env_debug.log'), 'w') as f:
#     f.write("--- CGI Environment Variables ---\n")
#     for key, value in os.environ.items():
#         f.write(f"{key}: {value}\n")

# # Read request body from standard input
# try:
#     body_data = sys.stdin.buffer.read()
#     body_size = len(body_data)

#     # Log request body information
#     with open(os.path.join(log_dir, 'request_debug.log'), 'wb') as f:
#         f.write(f"Request body size: {body_size} bytes\n".encode('utf-8'))
#         f.write(b"Request body content (first 100 bytes):\n")
#         f.write(body_data[:100])

#     # Save the uploaded file
#     upload_path = os.path.join(log_dir, f"uploaded_file_{int(time.time())}")
#     with open(upload_path, 'wb') as f:
#         f.write(body_data)

#     # Generate HTML response
#     print("Content-Type: text/html\n")
#     print("""
#     <!DOCTYPE html>
#     <html>
#     <head>
#         <title>Simple Upload Result</title>
#         <style>
#             body { font-family: Arial, sans-serif; margin: 20px; }
#             .success { color: green; }
#             .info { background: #f0f0f0; padding: 10px; }
#         </style>
#     </head>
#     <body>
#         <h1>File Upload Result</h1>
#     """)

#     print(f"<p class='success'>Upload successful! Received data: {body_size} bytes</p>")
#     print(f"<p>Saved at: {upload_path}</p>")

#     print("""
#         <div class="info">
#             <h3>Debug Information:</h3>
#             <p>1. Environment variables are saved in env_debug.log</p>
#             <p>2. Request body information is saved in request_debug.log</p>
#             <p>3. The uploaded raw data is saved at the path shown above</p>
#         </div>
#         <p><a href="/">Back to Home</a></p>
#     </body>
#     </html>
#     """)

# except Exception as e:
#     # Error handling
#     print("Content-Type: text/html\n")
#     print(f"""
#     <!DOCTYPE html>
#     <html>
#     <head>
#         <title>Upload Error</title>
#         <style>
#             body {{ font-family: Arial, sans-serif; margin: 20px; }}
#             .error {{ color: red; }}
#         </style>
#     </head>
#     <body>
#         <h1>File Upload Error</h1>
#         <p class="error">An error occurred during processing: {str(e)}</p>
#         <p><a href="/">Back to Home</a></p>
#     </body>
#     </html>
#     """)

#     # Log the error
#     with open(os.path.join(log_dir, 'error.log'), 'w') as f:
#         import traceback
#         f.write(traceback.format_exc())



