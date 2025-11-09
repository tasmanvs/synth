## WORKSPACE
workspace(name = "tasman_bazel_testing")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Emscripten SDK
http_archive(
    name = "emsdk",
    sha256 = "6df4b778f52e0e86c721cc6f97e7ad8fd6c842be2451e39f2bc528facb5d4cde",
    strip_prefix = "emsdk-3.1.56/bazel",
    url = "https://github.com/emscripten-core/emsdk/archive/refs/tags/3.1.56.tar.gz",
)

load("@emsdk//:deps.bzl", emsdk_deps = "deps")
emsdk_deps()

load("@emsdk//:emscripten_deps.bzl", emsdk_emscripten_deps = "emscripten_deps")
emsdk_emscripten_deps(emscripten_version = "3.1.56")

load("@emsdk//:toolchains.bzl", "register_emscripten_toolchains")
register_emscripten_toolchains()

# FLAC audio codec library
http_archive(
    name = "flac",
    sha256 = "aea54ed186ad07a34750399cb27fc216a2b62d0ffcd6dc2e3064a3518c3146f8",
    strip_prefix = "flac-1.5.0",
    url = "https://github.com/xiph/flac/archive/refs/tags/1.5.0.tar.gz",
    build_file = "@//third_party:flac.BUILD",
)
