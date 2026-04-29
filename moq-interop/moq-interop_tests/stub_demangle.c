/*
 * stub_demangle.c
 *
 * No-op stub for the Rust demangling function that folly references when
 * compiled with FOLLY_HAVE_RUST_DEMANGLE=1.  This stub satisfies the linker
 * without requiring the Rust toolchain (librustc-demangle-capi) in the build
 * image.
 *
 * The C++ demangling counterpart (cplus_demangle_v3_callback, normally from
 * libiberty) is NOT stubbed here — libiberty-dev is already installed in the
 * Docker builder and is linked via -liberty instead.
 *
 * Compiled only on Linux (see CMakeLists.txt).  macOS ld resolves the Rust
 * symbol transitively through the system C++ runtime.
 *
 * This stub does not affect test correctness: it only controls how folly
 * formats Rust frames in stack traces — a diagnostic path never exercised
 * during test execution.
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

#ifdef __cplusplus
}
#endif
