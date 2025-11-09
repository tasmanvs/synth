# Build the WebGL target first
Write-Host "Building WebGL application..." -ForegroundColor Cyan
& bazelisk build //:imgui_webgl --platforms=@emsdk//:platform_wasm

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green

# Pre-compress files with gzip for better performance
Write-Host "Compressing files with gzip..." -ForegroundColor Cyan
$files = @("bazel-bin\imgui_webgl.js", "bazel-bin\imgui_webgl.wasm")
foreach ($file in $files) {
    if (Test-Path $file) {
        $outFile = "$file.gz"
        $input = [System.IO.File]::ReadAllBytes($file)
        $output = New-Object System.IO.MemoryStream
        $gzip = New-Object System.IO.Compression.GZipStream($output, [System.IO.Compression.CompressionMode]::Compress)
        $gzip.Write($input, 0, $input.Length)
        $gzip.Close()
        [System.IO.File]::WriteAllBytes($outFile, $output.ToArray())
        $originalSize = $input.Length / 1024
        $compressedSize = $output.Length / 1024
        $ratio = [Math]::Round(($compressedSize / $originalSize) * 100, 1)
        Write-Host "  $(Split-Path $file -Leaf): $([Math]::Round($originalSize, 1))KB -> $([Math]::Round($compressedSize, 1))KB ($ratio%)" -ForegroundColor Gray
    }
}

# Create HTML file if it doesn't exist
$htmlFile = "bazel-bin\imgui_webgl.html"
if (-not (Test-Path $htmlFile)) {
    Write-Host "Creating HTML file..." -ForegroundColor Cyan
    @"
<!DOCTYPE html>
<html lang="en-us">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
    <title>ImGui - Bazel + Emscripten + WebGL</title>
    <style>
      body { margin: 0; padding: 0; background-color: #222; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; }
      .header { color: #fff; text-align: center; padding: 20px; }
      canvas.emscripten { border: 1px solid black; background-color: #333; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
      #status { color: #fff; margin: 10px; text-align: center; }
    </style>
  </head>
  <body>
    <div class="header">
      <h1>Dear ImGui - WebGL Demo</h1>
      <p>Built with Bazel + Emscripten</p>
    </div>
    <div id="status">Loading...</div>
    <canvas class="emscripten" id="canvas" oncontextmenu="event.preventDefault()" tabindex=-1></canvas>
    <script type='text/javascript'>
      var Module = {
        canvas: document.getElementById('canvas'),
        setStatus: function(text) { document.getElementById('status').innerHTML = text; }
      };
      Module.setStatus('Loading...');
    </script>
    <script async type="text/javascript" src="imgui_webgl.js"></script>
  </body>
</html>
"@ | Out-File -FilePath $htmlFile -Encoding UTF8
}

# Start HTTP server
Write-Host ""
Write-Host "Starting HTTP server..." -ForegroundColor Cyan
Write-Host "Open http://localhost:8000/imgui_webgl.html in your browser" -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop the server" -ForegroundColor Yellow
Write-Host ""

cd bazel-bin

try {
    python -m http.server 8000
} catch {
    # If Python doesn't work, use PowerShell HTTP server
    $listener = New-Object System.Net.HttpListener
    $listener.Prefixes.Add('http://localhost:8000/')
    $listener.Start()
    Write-Host 'HTTP Server running at http://localhost:8000/' -ForegroundColor Green
    
    while ($listener.IsListening) {
        $context = $listener.GetContext()
        $request = $context.Request
        $response = $context.Response
        
        $file = if ($request.Url.LocalPath -eq '/') { 'imgui_webgl.html' } else { $request.Url.LocalPath.TrimStart('/') }
        
        if (Test-Path $file) {
            $content = [System.IO.File]::ReadAllBytes((Resolve-Path $file).Path)
            $response.ContentType = if ($file -like '*.html') { 'text/html' } elseif ($file -like '*.js') { 'application/javascript' } elseif ($file -like '*.wasm') { 'application/wasm' } else { 'application/octet-stream' }
            $response.ContentLength64 = $content.Length
            $response.OutputStream.Write($content, 0, $content.Length)
        } else {
            $response.StatusCode = 404
        }
        $response.Close()
    }
}
