# BUILD file for FLAC library
# https://github.com/xiph/flac

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "flac",
    srcs = glob([
        "src/libFLAC/*.c",
        "src/libFLAC/include/private/*.h",
        "src/libFLAC/include/protected/*.h",
        "src/share/win_utf8_io/*.c",  # Windows UTF-8 I/O support
    ], exclude = [
        "src/libFLAC/ogg*.c",  # Ogg support - requires libogg
        # Exclude ARM-specific intrinsics
        "src/libFLAC/*_intrin_neon.c",
    ]),
    textual_hdrs = glob(["src/libFLAC/deduplication/*.c"]),  # Code fragments included by other files
    hdrs = glob([
        "include/FLAC/*.h",
        "include/share/*.h",
    ]),
    includes = [
        "include",
        "src/libFLAC/include",
        "src/share/win_utf8_io",
    ],
    defines = [
        "FLAC__NO_DLL",
        "FLAC__HAS_OGG=0",
        "FLAC__CPU_X86_64=1",
        "FLAC__HAS_X86INTRIN=1",
        "HAVE_STDINT_H=1",
        "HAVE_INTTYPES_H=1",
        'PACKAGE_VERSION=\\"1.5.0\\"',
        'VERSION=\\"1.5.0\\"',
    ] + select({
        "@platforms//os:windows": [
            "_CRT_SECURE_NO_WARNINGS",  # Disable MSVC secure warnings  
            "FLAC__NO_UTF8_FILENAMES",  # Disable UTF8 filename support to avoid missing symbols
        ],
        "//conditions:default": [
            "HAVE_BSWAP16=1",
            "HAVE_BSWAP32=1",
        ],
    }),
    local_defines = select({
        "@platforms//os:windows": [
            "inline=__inline",
        ],
        "//conditions:default": [],
    }),
    copts = select({
        "@platforms//os:windows": [
            "/wd4267",  # Disable size_t conversion warnings
            "/wd4244",  # Disable int conversion warnings
            "/wd4018",  # Signed/unsigned mismatch
        ],
        "//conditions:default": [
            "-Wno-unused-function",
        ],
    }),
)
