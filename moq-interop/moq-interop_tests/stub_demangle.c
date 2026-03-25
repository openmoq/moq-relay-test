/*
 * stub_demangle.c
 *
 * No-op stubs for Rust and C++ demangling functions that folly references when
 * compiled with FOLLY_HAVE_RUST_DEMANGLE=1 and FOLLY_HAVE_CPLUS_DEMANGLE_V3=1.
 * These stubs satisfy the linker without requiring a Rust toolchain or
 * libiberty installation in the Docker build image.
 *
 * Compiled only on Linux (see CMakeLists.txt).  macOS ld resolves these
 * symbols transitively through the system C++ runtime.
 *
 * Neither stub affects correctness of the interop tests: they only control
 * how folly formats stack traces when demangling symbol names — a purely
 * diagnostic path that is never exercised during test execution.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * rust_demangle_callback — normally provided by librustc-demangle-capi.
 * Leaves Rust symbols in mangled form in any stack trace folly may produce.
 */
void rust_demangle_callback(
    const char* mangled,
    int is_punycode,
    void (*cb)(const char* demangled, size_t demangled_len, void* opaque),
    void* opaque)
{
    (void)mangled;
    (void)is_punycode;
    (void)cb;
    (void)opaque;
    /* no-op: Rust demangling not needed for interop tests */
}

/*
 * cplus_demangle_v3_callback — normally provided by libiberty.a.
 * Returns 0 (= could not demangle) so folly falls back to the raw mangled
 * string in any stack trace.  int return matches libiberty's ABI.
 */
int cplus_demangle_v3_callback(
    const char* mangled,
    int          options,
    void (*cb)(const char* demangled, size_t demangled_len, void* opaque),
    void*        opaque)
{
    (void)mangled;
    (void)options;
    (void)cb;
    (void)opaque;
    return 0; /* 0 = demangling failed/not available */
}

#ifdef __cplusplus
}
#endif
