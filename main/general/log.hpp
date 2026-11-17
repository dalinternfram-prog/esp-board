#pragma once

// Portable, printf-style logging for the general/ layer.
// general/ must not include ESP-IDF headers, so this replaces ESP_LOGx.

#include <cstdio>

#define LOG_E(tag, fmt, ...) std::fprintf(stderr, "E %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) std::fprintf(stderr, "W %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) std::fprintf(stdout, "I %s: " fmt "\n", tag, ##__VA_ARGS__)
