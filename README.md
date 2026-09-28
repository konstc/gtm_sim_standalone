# Standalone simulation tool for the Bosch GTM SystemC model

Runs the Bosch GTM (Generic Timer Module) virtual model that COSEDA
distributes as a COSIDE bundle **without COSIDE**: the model is built into
ordinary SystemC executables with CMake, using the Accellera SystemC kernel
and COSEDA's SystemC-AMS proof-of-concept library. Linux and Windows
(MinGW-w64) are supported.

The bundle contains the model only as prebuilt static libraries, which
depend on COSIDE's runtime library. This project provides:

* `cos_compat/` – independent compatibility implementation of the runtime interface
  required by the prebuilt GTM model libraries;
* `tb_lib/` – building blocks for test benches (GTM harness, stimulus sources,
  bit-level connections, tracing helpers);
* `examples/` – example simulations with automatic waveform checks, each
  with its own README and waveform plots:
  [atom_somp](examples/atom_somp/README.md) (ATOM PWM, adaptation of the bundle's
  example) and [tom_pwm_irq](examples/tom_pwm_irq/README.md) (TOM PWM with an
  interrupt service routine);
* `tools/` – VCD comparison and plotting, COSIDE waveform-layout conversion and ABI checks.

## Inputs

External inputs are used read-only and referenced through environment
variables. Values in the environment always override values cached by
CMake; without the environment, pass them to CMake as `-D<NAME>=<path>`.

| Variable | Points to |
|---|---|
| `GTM_BUNDLE_DIR` | the Bosch GTM SystemC model bundle from [COSEDA](https://www.coseda-tech.com/bosch-gtm-systemc-model) (the unpacked folder containing `bosch_gtm/` and `cos_gtm_lib/`) |
| `SYSTEMC_AMS_DIR` | the unpacked SystemC-AMS proof-of-concept from [COSEDA](https://www.coseda-tech.com/systemc-ams-proof-of-concept) (the folder containing `src/systemc-ams.h`) |
| `MINGW_DIR` | Windows only: a MinGW-w64 GCC toolchain (the folder containing `bin\g++.exe`) |

The Accellera SystemC kernel is downloaded automatically on the first
configure. SystemC and SystemC-AMS are compiled as part of the project.

Set the variables once for your user account, e.g. in `~/.bashrc`:

```bash
export GTM_BUNDLE_DIR=/path/to/coside-<version>-Bosch_GTM-bundle-<date>
export SYSTEMC_AMS_DIR=/path/to/systemc-ams-<version>
```

or on Windows (PowerShell, takes effect in new terminals):

```powershell
[Environment]::SetEnvironmentVariable('GTM_BUNDLE_DIR', 'C:\tools\coside-<version>-Bosch_GTM-bundle-<date>', 'User')
[Environment]::SetEnvironmentVariable('SYSTEMC_AMS_DIR', 'C:\tools\systemc-ams-<version>', 'User')
[Environment]::SetEnvironmentVariable('MINGW_DIR', 'C:\tools\winlibs\mingw64', 'User')
```

`scripts/env.sh` / `scripts/env.ps1` check that the variables point to the
right folders and report what is missing; the build scripts call them.

## Quick start

**Linux / WSL**

```bash
scripts/install_deps.sh            # once: compiler, cmake, ninja, gtkwave, ... (needs sudo)
. scripts/env.sh
scripts/build.sh --test            # configure + build + test (preset linux-release)

cd build/linux-release/examples/atom_somp
./atom_somp --help
./atom_somp --lab1 --vcd=lab1      # writes lab1.vcd
```

**Windows (MinGW-w64)**

```powershell
. .\scripts\env.ps1
.\scripts\build.ps1 -Test          # configure + build + test (preset windows-release)

cd build\windows-release\examples\atom_somp
.\atom_somp.exe --lab1 --vcd=lab1
```

Notes:

* Presets: `linux-release`, `linux-debug`, `windows-release`, `windows-debug`
  (build directory `build/<preset>`). `--clean` / `-Clean` starts from scratch.
* The compiler is chosen on the first configure of a build directory. On
  Linux, a GCC matching the major version used for the bundle libraries is
  preferred when installed (`g++-9` for the tested bundle).
* Windows: `env.ps1` *appends* `MINGW_DIR\bin` to `PATH`, so the system CMake
  is used (WinLibs also ships a `cmake.exe` that cannot verify HTTPS
  certificates). Executables are linked statically and run without the
  toolchain on `PATH`.
* WSL: building inside the Linux file system is considerably faster than
  under `/mnt/c`.

## Requirements

| Item | Requirement |
|---|---|
| OS | Linux x86_64 (the bundle's `lib-linux64` libraries) or Windows x86_64 (its `lib-mingw-w64` libraries) |
| Compiler | Linux: GCC with C++14 support; ideally the GCC major version used for the bundle. Windows: MinGW-w64 GCC for x86_64 with SEH exceptions; the msvcrt runtime is the closest match to the bundle |
| CMake | ≥ 3.24, with Ninja |
| SystemC | the version (and `SC_CPLUSPLUS` value) the bundle libraries were built against - the build pins it (`GTM_SYSTEMC_URL`, `GTM_SYSTEMC_SHA256`, `GTM_SC_CPLUSPLUS` in the CMake files) |
| SystemC-AMS | the matching COSEDA proof-of-concept release, via `SYSTEMC_AMS_DIR` |
| Python 3 | waveform checks and tools |
| GTKWave | optional, for viewing traces |

## Layout

```
CMakeLists.txt, CMakePresets.json   super-build: SystemC + SystemC-AMS + cos_compat + tb_lib + examples + tests
cmake/Dependencies.cmake            SystemC / SystemC-AMS targets
cmake/GtmBundle.cmake               imported targets for the bundle libraries (via GTM_BUNDLE_DIR)
cmake/toolchains/mingw-w64.cmake    Windows toolchain (via MINGW_DIR)
cos_compat/                         re-implementation of the COSIDE runtime parts used by the libraries
tb_lib/                             test-bench building blocks (namespace gtm_tb)
examples/atom_somp/                 ATOM SOMP example (recreation of the COSIDE example)
examples/tom_pwm_irq/               TOM PWM with interrupt service routine
tests/                              unit test of cos_compat, ABI checks, example runs and checks
tools/                              VCD reader/diff/plotter, COSIDE .wave tools, ABI check tools
docs/ABI_NOTES.md                   how the COSIDE runtime was reconstructed and verified
scripts/                            env/build scripts (Linux: .sh, Windows: .ps1)
```

CMake targets: `gtm::gtm` (model + include paths + defines + runtime) and
`gtm::tb` (test-bench library, links `gtm::gtm`).

## Writing a test bench

```cpp
#include "gtm_tb/gtm_harness.h"          // GTM + clock + reset + a signal per GTM port
#include "cos_gtm_lib/gal/gal_app.h"     // register access: GTM.CLS[0].ATOM.CH[0].SR0 = 15; gal_wait(2);

gtm_tb::gtm_harness::params p;
p.test_program = [] { /* GAL application, runs in the GTM controller thread */ };
p.isr_func     = [] { /* interrupt service routine */ };
gtm_tb::gtm_harness tb("i_my_tb", p);
```

* `gtm_harness` exposes a signal per GTM port (`tb.gtm_atom0_out`,
  `tb.gtm_tim0_in`, ...); derive from it to add stimuli (see
  `examples/atom_somp/atom_somp_tb.cpp`). Clock period, reset time, device
  configuration and interrupt table are parameters.
* `gtm_tb::pwc_source`, `constant_source`: stimulus sources;
  `bit_extract`, `bv_combiner`: bit-level connections (replacing COSIDE's `sc_slice`).
* Tracing: the bundle's trace functions (`trace_atom()`, `trace_atom_ch()`,
  `trace_tom()`, ... in `cos_gtm_lib/traces/gtm_trace_functions.h`),
  `sc_object_trace(tf, "*pattern*", "name")` and
  `gtm_tb::trace_bit(tf, signal, bit, "name")`.
* `cos_enable_gtm_secondary_variables()` (before creating the GTM) makes the
  model register and trace additional internal variables.
* The GTM controller stops the simulation when the test program returns.
* The model reads `axim.mem` from the working directory; `gtm_add_example()` copies it.

Add an example with `gtm_add_example(<name> SOURCES ...)` in
`examples/<name>/CMakeLists.txt`, register it in `examples/CMakeLists.txt`,
and add a waveform check with `gtm_example_test()` in `tests/CMakeLists.txt`.

## Comparing waveforms

* **Between builds, platforms or simulators:** `tools/vcd_diff.py A.vcd B.vcd`
  compares two traces signal by signal (`--ignore GLOB` skips signals).
* **With COSIDE:** the COSIDE `.wave` files are waveform-viewer layouts – they
  list signal names and display settings but contain no sample data.
  `tools/wave_signals.py <file.wave> <trace.vcd>` checks that a trace contains
  the signals of a layout under the same hierarchical names, and
  `tools/wave2gtkw.py <file.wave> <trace.vcd>` converts the layout into a
  GTKWave save file for a visual comparison.
* **As a picture:** `tools/vcd_plot.py <plots.json> --vcd-dir <dir>` renders
  selected signals of one or more VCD files as SVG waveform plots (used for the
  plots in the example READMEs, e.g. `examples/atom_somp/doc/plots.json`).
* **Against expected behaviour:** each example has a check script (e.g.
  `examples/atom_somp/check_atom_somp.py`, based on the ATOM SOMP application
  note in the bundle) that CTest runs after the simulation.

## Differences to the COSIDE examples

* Test benches are plain SystemC (`gtm_tb`) instead of generated from COSIDE
  schematics; the atom_somp test bench keeps the COSIDE instance and signal
  names, so trace names are identical.
* The interrupt table is `gtm_tb::linear_port_irq_table()` (consecutive
  interrupt numbers) instead of the device-specific numbering of the COSIDE
  example; this only changes bit positions in the `irqs` vector of the
  interrupt controller.
* Enum-valued traces are written as ASCII bit vectors (8 bits per character);
  show them with GTKWave's "ASCII" data format.

## Compatibility checks and known issues

* `cos_compat` must match the class layouts the prebuilt libraries were
  compiled with. The tests `abi.layout` and `abi.vtables` verify this on every
  platform; keep them passing after changing the compiler, SystemC or the
  bundle. docs/ABI_NOTES.md describes the reconstruction.
* `cos_compat` implements the runtime functions the libraries need, not the
  whole COSIDE library (no `const_src_sc`, `pwc_src_sc`, `sc_slice`,
  simulation control GUI, ...).
* Defects found in the prebuilt libraries (details in docs/ABI_NOTES.md):
  * an uninitialized member in the ATOM output path – worked around by
    zero-initialized heap allocations (CMake option `GTM_ZERO_INIT_HEAP`,
    default ON), which also makes simulations deterministic;
  * trace code that records stack temporaries for some secondary variables
    (`*_reg`, `m_somp_*_tick`, `m_name`) – ignore those signals.

## Licence and trademarks

Copyright 2026 Konstantin Chernyshov. The code in this repository is
licensed under the Apache License 2.0 (see `LICENSE` and `NOTICE`).

This repository contains **no** part of the Bosch GTM model, of COSIDE or of
SystemC-AMS. They are obtained separately:

| Component | Source | Licence |
|---|---|---|
| Bosch GTM SystemC model | free download (registration) from [COSEDA](https://www.coseda-tech.com/bosch-gtm-systemc-model) | Apache License 2.0 |
| SystemC-AMS proof-of-concept | free download (registration) from [COSEDA](https://www.coseda-tech.com/systemc-ams-proof-of-concept) | Apache License 2.0 |
| SystemC | downloaded from Accellera (GitHub) during the build | Apache License 2.0 |

`cos_compat` is an independent implementation of the runtime interface the
prebuilt GTM libraries require (a part of COSIDE that is not included in the
model download), written to make the model usable with standard SystemC.

This project is not affiliated with, endorsed or supported by Robert Bosch
GmbH or COSEDA Technologies GmbH. "Bosch", "GTM", "COSIDE" and "COSEDA" are
used only to describe compatibility; they are trademarks or names of their
respective owners.

## Tested versions

The project was developed and tested with the following versions. Other
versions may need changes to the pinned SystemC version or to `cos_compat`;
the ABI tests show whether the layouts still match.

**COSEDA inputs**

| Input | Version / details |
|---|---|
| GTM bundle | `coside-3.1.1-Bosch_GTM-bundle-20240313` (COSIDE 3.1.1) |
| GTM model | GTM-RM v4.1.00-000 (GTM 4.1, `GAL_GTM_GEN=4`), configuration `DEBUG` |
| Bundle libraries (Linux) | `lib-linux64`, built with GCC 9.4.0 (COSEDA, RHEL7) |
| Bundle libraries (Windows) | `lib-mingw-w64`, built with MinGW-w64 GCC 9.4.0 (COSEDA), SEH exceptions, msvcrt |
| SystemC API of the libraries | SystemC 2.3.4, `SC_CPLUSPLUS=201103L` (C++11) |
| SystemC-AMS | 2.3.4 proof-of-concept (COSEDA) |

**Other components and tools**

| Component | Linux (WSL2, Ubuntu 24.04) | Windows 10 |
|---|---|---|
| SystemC | Accellera 2.3.4 (GitHub tag `2.3.4`) | same |
| Compiler | GCC 9.5.0 (also tested: GCC 13.3.0) | WinLibs GCC 14.2.0, posix/SEH/msvcrt, MinGW-w64 12.0.0, release r3 |
| CMake / Ninja | 3.28.3 / 1.11.1 | 3.31.5 / Ninja from WinLibs |
| Python | 3.12 | 3.11 |

**Results with these versions**

* All tests pass on both platforms; Linux and Windows produce identical
  waveforms (apart from the defective secondary-variable traces).
* ABI checks: 40 SystemC/TLM vtables compared on Linux, 30 on Windows, no
  mismatches; all class layouts match.
* `cos_gtm_example/atom_somp/atom_somp_cnt_out.wave`: 431 of 439 signals are
  present with `--secondary_variables`; the missing
  `atom0.tim_ext_capture_sig(i)` are not traced by any trace function of these
  libraries.
