#!/usr/bin/env python3
import http.server
import socketserver
import os
import sys

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
    
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"Server running at http://localhost:{PORT}/")
        print(f"Serving from: {os.getcwd()}")
        print(f"Open http://localhost:{PORT}/imgui_webgl.html in your browser")
        print("Press Ctrl+C to stop")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped")
