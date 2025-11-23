# Generate compile_commands.json for clangd
# This script generates compile commands for WebAssembly targets only

Write-Host "Generating compile_commands.json for WebAssembly targets..."

# Run bazelisk to generate compile commands
$targets = @(
    "//apps:imgui_webgl_bin",
    "//ui:main_window", 
    "//audio:audio_interface",
    "//audio:audio_node_graph",
    "//audio_loop:audio_loop_lib",
    "//node_editor:imgui_node_editor"
)

$target_string = $targets -join " "

# Generate the compile commands using bazel aquery
$query = "mnemonic('CppCompile',deps($target_string))"

Write-Host "Running bazel aquery..."
& bazelisk aquery $query `
    --output=jsonproto `
    --include_artifacts=true `
    --platforms=@emsdk//:platform_wasm `
    | Out-File -FilePath "aquery_output.json" -Encoding utf8

if ($LASTEXITCODE -eq 0) {
    Write-Host "Query successful! Processing output..."
    # TODO: Parse the output and create compile_commands.json
    Write-Host "Note: Manual parsing required. Consider using hedron_compile_commands with a custom target."
} else {
    Write-Host "Error running bazel aquery"
    exit 1
}
