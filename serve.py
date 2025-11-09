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
        serve_dir = sys.argv[1]
    else:
        # Try to find the apps directory in bazel-bin
        if os.path.exists('bazel-bin/apps'):
            serve_dir = 'bazel-bin/apps'
        elif os.path.exists('bazel-bin'):
            serve_dir = 'bazel-bin'
        else:
            print("Error: bazel-bin directory not found!")
            print("Please build a web application first:")
            print("  bazelisk build //apps:imgui_webgl")
            sys.exit(1)
    
    os.chdir(serve_dir)
    
    # Check if required files exist and provide helpful message
    html_files = [f for f in os.listdir('.') if f.endswith('.html')]
    if not html_files:
        print(f"Error: No HTML files found in {os.getcwd()}")
        print("\nPlease build a web application first with the emscripten platform:")
        print("  bazelisk build //apps:imgui_webgl --platforms=@emsdk//:platform_wasm")
        print("  bazelisk build //apps:audio_webgl --platforms=@emsdk//:platform_wasm")
        print("\nNote: Web apps require --platforms=@emsdk//:platform_wasm to compile to WebAssembly")
        sys.exit(1)
    
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
        
        # List available HTML files
        html_files = [f for f in os.listdir('.') if f.endswith('.html')]
        if html_files:
            print("\nAvailable applications:")
            for html_file in html_files:
                print(f"  http://localhost:{PORT}/{html_file}")
        else:
            print(f"Open http://localhost:{PORT}/imgui_webgl.html in your browser")
        
        print("\nPress Ctrl+C to stop")
        
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
