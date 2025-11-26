cc_binary(
    name = "hello_world",
    srcs = ["hello_world.cc"],
    deps = [
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:globals",
        "@abseil-cpp//absl/log:initialize",
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "flac_test",
    srcs = ["flac_test.cc"],
    deps = [
        "@flac//:flac",
    ],
    visibility = ["//visibility:public"],
)
