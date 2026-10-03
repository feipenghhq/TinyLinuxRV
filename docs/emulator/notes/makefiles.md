# TinyLinuxRV Makefile Notes

This is a quick reference for understanding the emulator build system after
time away from the project. It focuses on the dependency graph and the GNU Make
features used by TinyLinuxRV.

## Build Files

- [`emulator/Makefile`](../../../emulator/Makefile) builds `rvemu` and exposes
  the main run and test targets.
- [`tests/sanity/Makefile`](../../../emulator/tests/sanity/Makefile) builds the
  small RV64IMA sanity programs.
- [`tests/riscv-test/Makefile`](../../../emulator/tests/riscv-test/Makefile)
  builds the enabled upstream `riscv-tests` suites.
- [`tests/toolchain/smoke/Makefile`](../../../emulator/tests/toolchain/smoke/Makefile)
  provides a minimal cross-toolchain check.

From the repository root, the main commands are:

```shell
make -C emulator build
make -C emulator sanity-test
make -C emulator riscv-tests
make -C emulator regression
make -C emulator clean
```

## Core Model

Make evaluates a dependency graph. It does not execute the file from top to
bottom like a shell script.

```make
target: prerequisites
	recipe
```

Make runs the recipe when the target is missing or when a prerequisite is
newer. Recipe lines must start with a tab.

The main build graph is:

```text
emulator C sources and headers
              │
              ▼
        build/**/*.o + .d
              │
              ▼
            rvemu

riscv-tests .S source + environment sources
              │
              ▼
             .elf ──→ .bin
              │
              ▼
          test runner
```

The host compiler builds `rvemu`. The RISC-V cross-toolchain builds programs
that run inside the emulator.

## Make Syntax Used Here

| Syntax | Meaning |
| --- | --- |
| `=` | Expand the value when it is used |
| `:=` | Expand the value when it is assigned |
| `?=` | Provide a default that the command line can override |
| `+=` | Append to a variable |
| `$@` | Current target |
| `$<` | First prerequisite |
| `$^` | All prerequisites |
| `$(@D)` | Directory containing the current target |
| `%` | Stem matched by a pattern rule |
| `\|` | Start order-only prerequisites |
| `.PHONY` | Mark action targets that do not represent files |

Common functions in these Makefiles:

- `patsubst` converts one filename pattern into another.
- `wildcard` lists files matching a pattern.
- `addprefix` and `addsuffix` construct output paths.
- `foreach`, `call`, and `eval` instantiate the `riscv-tests` suite template.
- `shell` captures the output of a shell command.

Use `$(MAKE)` for nested builds. It preserves parallel-build settings,
command-line variables, and failure propagation.

## Building the Emulator

The top-level emulator Makefile maps C sources into matching paths under
`build/`:

```make
SOURCES := src/cpu/cpu.c
SOURCES += src/memory/memory.c
SOURCES += src/memory/loader.c
SOURCES += src/main.c

OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
```

For example:

```text
src/cpu/cpu.c       → build/cpu/cpu.o
src/memory/loader.c → build/memory/loader.o
```

The pattern rule creates the matching directory and compiles one source file:

```make
$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
```

`-MMD -MP` generates a `.d` file next to each object. These files record header
dependencies:

```make
-include $(DEPS)
```

The leading `-` allows the first build to proceed before any `.d` files exist.
Afterwards, changing a header rebuilds only the affected objects.

`include/cpu/decode.h` is generated from the instruction-pattern script. The
explicit dependency ensures it exists before `cpu.o` is compiled:

```make
$(BUILD_DIR)/cpu/cpu.o: include/cpu/decode.h
include/cpu/decode.h: scripts/generate_inst_patterns.py
	python3 $<
```

To add another emulator source file, append it to `SOURCES`. The existing
mapping, compile rule, header dependencies, and link rule handle the rest.

Useful overrides include:

```shell
make -C emulator LOG_LEVEL=LOG_LEVEL_DEBUG build
make -C emulator run-elf ELF=path/to/program.elf
```

Changing compiler flags does not update an input timestamp known to Make. Use a
clean rebuild after an important flag change.

## Building Guest Tests

The sanity build uses `assembly → object → ELF → raw binary`.
The `riscv-tests` build compiles and links directly to ELF, then generates
the raw binary; it does not retain a separate object file.

The ELF file retains sections and symbols, so it is useful for loading and
disassembly. The raw binary contains only the bytes placed in guest memory.

The sanity sources are plain assembly and use `as`. Upstream `riscv-tests`
sources use uppercase `.S`, `#include`, and macros, so they are compiled with
the cross-GCC preprocessor instead.

The `v` environment also links `entry.S`, `vm.c`, and `string.c`. Its build
uses Picolibc headers; see [toolchain notes](riscv-toolchain.md).

The current `riscv-tests` prerequisites do not include the linker script or
all shared headers. Force a rebuild after changing these inputs.

## The `riscv-tests` Suite Template

The enabled suites are selected per environment:

```make
SUITES-p ?= rv64ui rv64um rv64ua rv64mi rv64si
SUITES-v ?= rv64ui rv64um rv64ua
SUITES-all ?= $(sort $(SUITES-p) $(SUITES-v))
```

`p` uses physical addresses; `v` exercises Sv39. Architecture flags are
selected per suite and environment, with a shared `MABI := lp64`:

```make
rv64ui_p_MARCH := rv64if
rv64ui_v_MARCH := rv64imaf
```

The upstream `Makefrag` for every enabled suite supplies its test names:

```make
include $(addsuffix /Makefrag,$(addprefix $(ISA_DIR)/,$(SUITES-all)))
```

`SUITE_template` takes the suite as `$(1)` and environment as `$(2)`:

```text
rv64ui/add.S + env/v sources → build/v/rv64ui/add.elf
                            → build/v/rv64ui/add.bin
```

Environment and suite names are included in output paths so identically named tests cannot
overwrite each other.

The template becomes concrete Make syntax through:

```make
$(foreach suite,$(SUITES-p),$(eval $(call SUITE_template,$(suite),p)))
$(foreach suite,$(SUITES-v),$(eval $(call SUITE_template,$(suite),v)))
```

This is evaluated in two stages:

1. `call` substitutes the suite and environment and `eval` parses the generated rules.
2. Make later expands those rules while building a concrete target.

For that reason, values needed in the second stage use `$$` inside the
template:

```make
$$@                  # preserve $@ for the generated rule
$$<                  # preserve $< for the generated rule
$$($(1)_$(2)_MARCH)  # e.g. $(rv64ui_v_MARCH)
```

A useful rule of thumb is: one extra `$` protects an expression from the first
expansion pass. `$(1)` and `$(2)` keep a single `$` because the template arguments
must expand immediately.

Each suite also generates `build/<env>/<suite>/tests.txt`. The Python runner reads
this manifest, replaces each `.bin` suffix with `.elf`, and runs the ELF with
the emulator. The binaries are order-only prerequisites because the manifest
contains their paths, not their contents.

To enable another upstream suite:

1. Add it to `SUITES-p` or `SUITES-v`.
2. Define its per-environment `_MARCH` variable; the shared ABI is `lp64`.
3. Confirm the corresponding upstream `Makefrag` exists.
4. Add the suite to `tests/riscv-test/run.py` if it should run automatically.
5. Verify that the emulator and test environment implement the required ISA or
   privileged behavior.

Adding `rv64mi` or `rv64si`, for example, requires more than compiler flags;
their environment needs CSRs, traps, and privilege-mode transitions.

## Rebuild and Debug Reference

Expected incremental behavior:

- Changing one emulator `.c` file recompiles its object and relinks `rvemu`.
- Changing an emulator header rebuilds the objects listed by generated `.d`
  files.
- Changing a `riscv-tests` `.S` file rebuilds its `.elf` and `.bin`.
- After changing untracked shared headers or `env/link.ld`, run
  `make -B -C emulator/tests/riscv-test` from the repository root.
- Changing nothing runs no build recipes for real file targets.

Useful diagnostics:

```shell
make -n          # preview recipes
make --trace     # explain why targets rebuild
make -B          # force a rebuild
make -j4         # check whether dependencies are parallel-safe
```

When debugging, check:

- Is the expected output a real target or a phony action?
- Are all source files, headers, scripts, and linker scripts prerequisites?
- Is the correct host or cross compiler being used?
- Did a variable expand during `eval` instead of during the generated rule?
- Does a nested test failure return a nonzero status to the outer Make?
- Does `clean` remove only files generated by that Makefile?

Do not hide build or test failures with `-` or `|| true`. A nonzero recipe exit
status lets the regression runner report a failed group after running all groups.
