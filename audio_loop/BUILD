cc_library(
	name = "sine_wave_buffer",
	srcs = ["sine_wave_buffer.cc"],
	hdrs = ["sine_wave_buffer.h"],
	visibility = ["//visibility:public"],
)

cc_test(
	name = "sine_wave_buffer_test",
	srcs = ["sine_wave_buffer_test.cc"],
	deps = [
		":sine_wave_buffer",
		"@googletest//:gtest_main",
	],
)
