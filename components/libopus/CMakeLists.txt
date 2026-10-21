file(GLOB_RECURSE OPUS_SRC_FILES
    "src/*.c"
    "celt/*.c"
    "silk/*.c"
)

# Exclude demos, tests, host tools, non-target architectures, and floating-point SILK files
list(FILTER OPUS_SRC_FILES EXCLUDE REGEX ".*(_demo|_compare|test_|fuzzer|dump_|write_|bwe_).c$")
list(FILTER OPUS_SRC_FILES EXCLUDE REGEX "/(x86|arm|mips|dump_modes|tests|float)/")

idf_component_register(
    SRCS ${OPUS_SRC_FILES}
    INCLUDE_DIRS
        "include"
        "celt"
        "silk"
    PRIV_INCLUDE_DIRS
        "src"
        "silk/fixed"
)

target_compile_options(${COMPONENT_TARGET} PRIVATE
    -DOPUS_BUILD
    -DFIXED_POINT
    -DUSE_ALLOCA
    -DHAVE_CONFIG_H=1
    -Wno-unused-function
    -Wno-sign-compare
    -Wno-uninitialized
    -Wno-unused-variable
)
