# COSIDE runtime compatibility – interface notes

The GTM model in the COSIDE bundle is shipped as prebuilt static libraries.
Besides the C and C++ runtime, SystemC/TLM and SystemC-AMS, they depend on a
set of functions and classes from COSIDE's runtime library. `cos_compat/`
provides an independent implementation of exactly that interface, so that the
libraries can be linked into ordinary SystemC executables.

This document describes the interface requirements the implementation
follows and how compatibility is verified. The exact versions the
statements below were verified with are listed in the README
("Tested versions").

## Build requirements imposed by the libraries

* **SystemC version and C++ level.** The libraries reference SystemC's API
  version check symbol, which encodes the SystemC version and the value of
  `SC_CPLUSPLUS`. SystemC and all code of the project must be built with the
  same version and value; the build pins both (`GTM_SYSTEMC_URL`,
  `GTM_SC_CPLUSPLUS`).
* **SystemC-AMS.** A few tracing internals of SystemC-AMS are referenced; the
  matching COSEDA proof-of-concept release is linked. Its option
  `DISABLE_PARALLEL_TRACING` changes a class layout and is therefore a public
  compile definition of the `systemc-ams` target.
* **SystemC build options.** Simulation phase callbacks are enabled (used by
  the trace pre-processing of the compatibility layer). On Windows the SystemC
  kernel uses Win32 fibers; the unused QuickThreads assembly is excluded from
  the build.
* **Windows.** The MinGW-w64 libraries need an x86_64 GCC with SEH
  exceptions, a statically linked SystemC, and the usual MinGW C runtime; no
  threading or TLS support is required.

## Interface provided by `cos_compat`

The libraries contain inline and template code that uses these classes, so
besides the function signatures, the object sizes, member order and virtual
function order must match. Where the libraries only call out-of-line
functions, only the size of the object matters. All entries are in namespace
`sca_basic_libraries_namespace` unless noted.

### Deferred vector binding – `cos_connectivity_elaborator`, `sc_core::sc_vector_n<T,N>`

`sc_vector_n<T,N>` is an `sc_vector<T>` that is created with `N` elements (or
empty for `N == 0`) and can be bound to other vectors before their size is
known. The GTM netlist uses it for most port vectors.

* Layout: the `sc_vector<T>` base, followed by a `cos_connectivity_elaborator`
  (16 bytes) and a `std::function<void(std::size_t)>` init callback.
* `bind(other)` registers an elaboration function with the elaborator and
  binds the two vectors; if either side already has a size, the elaborator
  runs it with that size (`sc_core::elaboration_data`, derived from
  `cos_connectivity_elaborator::elaboration_data_base`).
* `init(n)` creates the elements (in the vector's parent module) and notifies
  all pending bindings of the vector.
* The elaboration function sizes the still empty side and binds element-wise.
  Each binding must run exactly once – running it twice binds ports twice.

### Variable handles – `cos_sc_variable_handle_base`, `cos_sc_traceable_object`

`cos_sc_variable_handle<T>` (declared in the bundle header
`cos_utilities/cos_sc_variable_handle.h`) makes model member variables
visible as `sc_object`s for tracing. `cos_compat` implements the base class
methods against that declaration. `cos_sc_traceable_object` is a pure
interface with `trace_sc(tf, name) const` and a virtual destructor.

### Enum tracing – `cos_sc_trace_preprocessor_base`, `cos_sc_enum_tracer_base`

Inheritance: `sc_object` ← (public) `cos_sc_trace_preprocessor_base` ←
(private) `cos_sc_enum_tracer_base` ← (private) `cos_sc_enum_tracer<E>`
(template in the libraries). Virtual functions after those of `sc_object`:
`trace_preprocess`, `convert_to_string`. The enum tracer base holds 16 bytes
of state after the `sc_object` part. `cos_compat` traces the string as an
ASCII bit vector and refreshes it from a phase callback before values are
sampled.

### Object access and trace-by-name

* `object_searcher_base(long)` / `get_matching_objects(pattern)`: virtual
  destructor, `check_type`, `push_back`; two `long` counters (so the size
  differs between LP64 Linux and LLP64 Windows). Patterns are globs over full
  hierarchical names.
* `sca_obj_floc_trace(file, line)` with `sc_object_trace_(tf, pattern, name)`:
  a temporary object whose destructor is inline in the libraries; its members
  are a `std::string`, an `int` and a `std::ifstream`.
* `cos_trace_type_registrar_base`: virtual `trace_sc`, `trace_sca`, `is_type`,
  `is_same`, destructor, plus a `bool` ownership flag. After
  `cos_register_sc_object_trace_type(r)`, the caller deletes `r` unless the
  registry has taken ownership (flag set).
* `sca_obj_access_namespace::set_file_location / get_filename / get_lineno`:
  source location for diagnostics.

### Free functions

`request_channel_update(sc_prim_channel&)`, `operator"" _SC_NS(unsigned long long)`,
`cos_get_default_number_of_enum_trace_characters()`, and
`cos_gtm_lib_namespace::aei_read / aei_write` (forwarded to the GTM
controller of the libraries).

### Headers

The public headers of the bundle include
`sca_basic_libraries/utilities/sca_basic_lib_utilities.h`,
`sca_basic_libraries/coside_utilities.h`,
`sca_basic_libraries/vector_utilities/sc_vector_n.h` and
`sca_basic_libraries/generic_tb_utilities/cos_sc_traceable_object.h`;
`cos_compat/include` provides them. `sca_basic_lib_utilities.h` has to make
`sc_bv`, `std::uint32_t` and `std::function` visible.

## Platform differences

The same interface is used on Linux and Windows. Two sizes differ because of
the platform, in the same way in the libraries and in `cos_compat`:

| Item | Linux | Windows | Reason |
|---|---|---|---|
| `object_searcher<T>` | 0x20 bytes | 0x18 bytes | `long` is 32 bit on Windows |
| `sca_obj_floc_trace` | 0x230 bytes | 0x200 bytes | `std::ifstream` is smaller in MinGW's libstdc++ |

## Known defects in the prebuilt libraries

1. **Uninitialized member in the ATOM output path.** An internal state
   object of the `hres8_delay` module is not fully initialized. Depending on
   leftover heap contents, the ATOM outputs then miss their configured reset
   level, and results vary with the environment, parallel runs and platform.
   **Workaround:** `cos_compat/src/cos_zero_init_new.cpp` replaces the global
   `operator new` with a zero-filling version (CMake option
   `GTM_ZERO_INIT_HEAP`, default ON, compiled into every executable linking
   `gtm::gtm`). Heap objects start from zero, as fresh memory does, and the
   simulation becomes deterministic.
2. **Traces of temporaries.** With `cos_enable_gtm_secondary_variables()`,
   some internal variables (e.g. `*_reg`, `m_somp_*_tick`, `m_name`) are
   traced through references to temporary copies, so their traced values are
   meaningless. This cannot be fixed outside the libraries; ignore these
   signals.

With the workaround in place and these signals ignored, Linux and Windows
builds produce identical waveforms (`tools/vcd_diff.py`).

## Verification

Two tests check compatibility on every platform:

* `abi.layout` builds `tools/abi/layout_probe.cpp` and compares the sizes of
  the classes above, as seen through the project's headers, with the values
  the libraries expect (`tools/abi/check_layout.py`, one table per platform).
* `abi.vtables` builds `tools/abi/sc_vtable_probe.cpp` and compares its
  SystemC/TLM vtables slot by slot with those contained in the libraries
  (`tools/abi/vtable_compare.py`, ELF and PE/COFF).

The unit test `tests/test_cos_compat.cpp` covers the behaviour of the
deferred vector binding (both binding orders), object search and tracing.
