#!/usr/bin/env python3
import http.server
import socketserver
import os
import sys
import threading

PORT = 8080

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers for WebAssembly
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

    def guess_type(self, path):
        mimetype = super().guess_type(path)
        if path.endswith('.wasm'):
            return 'application/wasm'
        return mimetype

if __name__ == '__main__':
    import signal
    
    # Set up signal handler for clean shutdown
    def signal_handler(sig, frame):
        print("\n\nServer stopped by user")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    # Change to the directory where the built files are
    if len(sys.argv) > 1:
        os.chdir(sys.argv[1])
    else:
        # Default to bazel-bin if no argument provided
        if os.path.exists('bazel-bin'):
            os.chdir('bazel-bin')
        else:
            print("Error: bazel-bin directory not found!")
            print("Please run: bazelisk build //:imgui_webgl --platforms=@emsdk//:platform_wasm")
            sys.exit(1)
    
    # Check if required files exist
    if not os.path.exists('imgui_webgl.html'):
        print("Error: imgui_webgl.html not found!")
        print("Creating HTML file...")
        # Create a minimal HTML file
        html_content = '''<!DOCTYPE html>
<html><head><meta charset="utf-8"><title>ImGui WebGL</title></head>
<body><canvas id="canvas"></canvas>
<script>var Module = {canvas: document.getElementById('canvas')};</script>
<script src="imgui_webgl.js"></script></body></html>'''
        with open('imgui_webgl.html', 'w') as f:
            f.write(html_content)
    
    Handler = MyHTTPRequestHandler
    
    # Use ThreadingTCPServer for better Ctrl+C handling
    class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
        daemon_threads = True
        allow_reuse_address = True
    
    httpd = None
    try:
        httpd = ThreadedTCPServer(("", PORT), Handler)
        print(f"Server running at http://localhost:{PORT}/")
        print(f"Serving from: {os.getcwd()}")
        print(f"Open http://localhost:{PORT}/imgui_webgl.html in your browser")
        print("Press Ctrl+C to stop")
        
        # Run server in a thread so Ctrl+C works immediately
        server_thread = threading.Thread(target=httpd.serve_forever)
        server_thread.daemon = True
        server_thread.start()
        
        # Keep main thread alive and wait for Ctrl+C
        try:
            while True:
                threading.Event().wait(1)
        except KeyboardInterrupt:
            print("\n\nShutting down server...")
    except OSError as e:
        if e.errno == 10048 or e.errno == 10013:
            print(f"\nError: Port {PORT} is already in use.")
            print("Either another server is running, or try a different port.")
        else:
            print(f"\nError: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        pass
    finally:
        if httpd:
            httpd.shutdown()
            httpd.server_close()
        print("Server stopped")
