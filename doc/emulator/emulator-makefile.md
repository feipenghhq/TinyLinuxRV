# Makefile Learning Notes for TinyLinuxRV

This document records the Makefile concepts needed to build the first version
of the TinyLinuxRV emulator. Its purpose is to explain the reasoning behind the
build system, rather than provide a complete Makefile to copy directly.

## 1. Current Build Goals

The Phase 1 build system should eventually support the following workflow:

```text
make
  -> compile each emulator source file
  -> link the emulator executable

make test
  -> build the emulator
  -> build the RV64I sanity program
  -> convert the sanity ELF file to a raw binary
  -> run the raw binary with the emulator
  -> use the emulator exit status to determine pass or fail

make clean
  -> remove only generated emulator and sanity-test files
```

The first version should stay small. It does not need separate release/debug
configurations, platform detection, package installation, or a general-purpose
build framework.

## 2. The Core Mental Model

Make is primarily a dependency-graph evaluator, not a shell script that simply
runs from top to bottom.

A rule has three parts:

```make
target: prerequisites
	command that creates or updates the target
```

The command line must begin with a tab.

Make runs the command when:

1. The target does not exist.
2. A prerequisite is newer than the target.
3. The target is phony and therefore represents an action instead of a file.

Otherwise, Make considers the target up to date.

For TinyLinuxRV, the dependency graph should eventually resemble:

```text
source files and headers
          |
          v
      object files
          |
          v
        rvemu
          |
          +-------------------+
                              |
RV64 assembly                 |
      |                       |
      v                       |
RV64 object                   |
      |                       |
      v                       |
RV64 ELF                      |
      |                       |
      v                       |
RV64 raw binary --------------+
                              |
                              v
                         test result
```

## 3. Compilation and Linking Are Different Steps

The current emulator can be built with one compiler command, but separating
compilation from linking enables useful incremental builds.

### Compilation

Compilation converts one C source file into one object file:

```text
source.c -> source.o
```

The compiler is invoked with `-c`. No executable is produced at this stage.

### Linking

Linking combines all object files into the final executable:

```text
main.o + cpu.o + memory.o -> rvemu
```

The C compiler driver should normally perform this link instead of invoking the
host `ld` directly. The compiler driver knows how to include the required C
runtime and standard libraries.

The sanity test is different: it is a freestanding RISC-V assembly program, so
using the cross-toolchain assembler and linker directly is reasonable there.

## 4. Standard Build Variables

Using conventional variable names makes a Makefile easier to understand and
override.

### `CC`

The host C compiler used to build the emulator.

### `CPPFLAGS`

Options for the C preprocessor. Typical examples are:

- Header search paths such as `-Iinclude`.
- Preprocessor definitions such as `-DLOG_LEVEL=...`.

TinyLinuxRV only needs its public `include/` directory. Searching every
directory under the repository introduces unrelated test and build directories
and can accidentally select a wrong same-named header.

### `CFLAGS`

Options that control C compilation, for example:

- `-std=c99`
- `-Wall`
- `-Wextra`
- `-Wpedantic`
- `-Wshadow`
- `-Wformat=2`
- `-Wconversion`
- Debug or optimization settings

### `LDFLAGS`

Options passed during the host link step, such as sanitizer link options or
library search paths.

### `LDLIBS`

Libraries linked into the host executable, such as `-lm`. The emulator does not
currently need an extra library.

Keeping preprocessor, compiler, and linker options separate prevents a
command-line override of one category from accidentally removing another.

## 5. Make Variable Assignment

Make supports several assignment styles:

| Syntax | Meaning |
| --- | --- |
| `=` | Recursively expanded; evaluated when used |
| `:=` | Simply expanded; evaluated when defined |
| `?=` | Assign only when the variable is not already defined |
| `+=` | Append to the existing value |

For fixed paths and source lists, `:=` is often easiest to reason about.

For a user-selectable setting such as `LOG_LEVEL`, `?=` allows an invocation
like:

```shell
make LOG_LEVEL=LOG_LEVEL_INFO
```

Command-line variable assignments normally take precedence over ordinary
assignments in the Makefile.

## 6. Source, Object, and Dependency Lists

It is useful to think in terms of three lists:

1. C source files.
2. Object files derived from those source files.
3. Dependency files derived from those object files.

A possible directory mapping is:

```text
src/main.c          -> build/main.o
src/cpu/cpu.c       -> build/cpu/cpu.o
src/memory/memory.c -> build/memory/memory.o
```

Mirroring the source hierarchy under `build/` prevents collisions if different
source directories later contain files with the same name.

Before writing the Make expression that performs this mapping, answer:

- What prefix is removed from every source path?
- What prefix is added to every object path?
- What suffix changes from `.c` to `.o`?
- Does the relative subdirectory need to be preserved?

Functions such as `patsubst` can express that transformation once the mapping
is clear.

## 7. Pattern Rules

A pattern rule describes how a category of targets is generated from a
corresponding category of prerequisites.

Conceptually, TinyLinuxRV needs a rule meaning:

```text
every build/%.o is compiled from src/%.c
```

For `src/cpu/cpu.c`, the `%` stem is `cpu/cpu`, producing
`build/cpu/cpu.o`.

When designing the rule, determine:

1. The target pattern.
2. The source pattern.
3. Which compiler variables belong in the command.
4. How the output file is named.
5. How the output directory is created first.

## 8. Automatic Variables

Make supplies automatic variables inside recipes:

| Variable | Meaning |
| --- | --- |
| `$@` | Current target |
| `$<` | First prerequisite |
| `$^` | All prerequisites, with duplicates removed |
| `$(@D)` | Directory containing the current target |

Typical uses are:

- Compilation: the input is the first prerequisite and the output is the
  current target.
- Linking: all object prerequisites are passed to the compiler driver and the
  output is the current target.
- Directory creation: create the directory containing the current target.

Automatic variables avoid repeating filenames in recipes and make rules easier
to rename.

## 9. Build Directories and Order-Only Prerequisites

The sanity Makefile already uses an order-only prerequisite:

```make
target: normal-prerequisite | order-only-prerequisite
```

An order-only prerequisite must exist before the target is built, but its
timestamp does not decide whether the target is out of date.

This is useful for directories. A directory timestamp changes whenever entries
inside it are added or removed. Treating it as a normal prerequisite can cause
unnecessary rebuilds.

For the emulator object rule, either:

- Express the required build directory as an order-only prerequisite, or
- Create `$(@D)` immediately before compiling the object.

The first approach emphasizes the dependency graph. The second is sometimes
simpler when every object can live in a different subdirectory.

## 10. Header Dependencies

A rule that says only:

```text
cpu.o depends on cpu.c
```

does not tell Make that `cpu.o` must also be rebuilt after `cpu.h` or `log.h`
changes.

Manually listing every header works for a very small project, but it is easy to
miss indirect dependencies such as:

```text
main.c -> cpu.h -> memory.h
```

GCC can generate dependency files while compiling:

- `-MMD` generates dependencies on project headers while excluding system
  headers.
- `-MP` creates dummy header targets so removing a header does not immediately
  produce a confusing "No rule to make target" error.

The build directory then contains pairs such as:

```text
build/cpu/cpu.o
build/cpu/cpu.d
```

The `.d` file records the source and headers required by the object. The main
Makefile must include these generated files. On the first clean build they do
not exist yet, so including them must tolerate missing files.

To implement this, derive the answers to three questions:

1. How is the dependency-file list derived from the object-file list?
2. Which compiler options generate each dependency file?
3. How can Make read the files without failing before the first build?

Expected behavior after this is implemented:

- Changing `cpu.c` rebuilds only the CPU object and then relinks.
- Changing `cpu.h` rebuilds the CPU and main objects.
- Changing `log.h` rebuilds every object that includes it.
- Changing nothing performs no compilation or linking.

## 11. Real File Targets and Phony Targets

Targets such as `rvemu`, `.o`, `.elf`, and `.bin` represent real files. Their
timestamps are meaningful.

Targets such as these represent actions:

- `all`
- `clean`
- `sanity`
- `test`

They should be declared phony. Otherwise, a real file named `clean` or `test`
could make Make incorrectly decide that the action is already complete.

The first target in a Makefile is the default target. Keeping `all` first makes
plain `make` intuitive. `all` normally depends on the desired build products and
does not need its own recipe.

## 12. Connecting the Sanity Build

The intended test dependency graph is:

```text
test
 ├─ host emulator
 └─ sanity raw binary
      └─ built by tests/sanity/Makefile
```

The outer Makefile should invoke the nested Makefile using `$(MAKE)`, rather
than hard-coding `make`. This preserves:

- Parallel-build job information.
- Command-line variables and Make flags.
- Correct recursive-build behavior.
- Failure propagation.

At the current project scale, a phony outer `sanity` action can invoke the
nested Makefile every time. The nested Makefile will still perform its own
incremental dependency checks.

The `test` recipe then runs the generated raw binary with the host emulator.
Make treats a command exit status of zero as success and a nonzero status as
failure. Do not prefix the test command with `-` or append `|| true`, because
those forms would hide a real failure.

At present, `ebreak` means that the guest stopped normally. Later, the sanity
program can use `a0 == 0` for success and a nonzero value for a failed test ID.

## 13. Clean Rules

A clean rule should delete exactly the files generated by the build system:

- The host object/dependency build directory.
- The emulator executable.
- The sanity-test build directory, usually through its own nested clean rule.

Avoid searching for and deleting every `.o` or `.bin` in the repository. Such a
rule can eventually remove inputs or artifacts owned by a different workflow.

The clean target should have no file prerequisites and should be phony.

## 14. Recommended Implementation Checkpoints

Implement the Makefile incrementally instead of introducing all features at
once.

### Checkpoint 1: Organize Variables

Goals:

- Select the host compiler.
- Separate `CPPFLAGS` and `CFLAGS`.
- Use only the required public include directory.
- Define the source list and final executable name.

Verification:

```shell
make -n
```

The printed compiler command should not contain test or generated build
directories as include paths.

### Checkpoint 2: Introduce Object Files

Goals:

- Compile each C source into its own object.
- Place objects in a dedicated build directory.
- Link the emulator from object files.

Verification:

- A clean build creates all expected objects.
- Editing only `memory.c` recompiles only the memory object and then relinks.
- Running Make again without changes does nothing.

### Checkpoint 3: Generate Header Dependencies

Goals:

- Generate `.d` files next to object files.
- Include the generated dependency files.
- Permit a first build where the dependency files do not yet exist.

Verification:

- Editing `cpu.h` rebuilds the appropriate objects.
- Editing `log.h` rebuilds every C file that includes it.
- Removing all build output still permits a clean first build.

### Checkpoint 4: Add Action Targets

Goals:

- Declare `all`, `clean`, `sanity`, and `test` as phony.
- Build the sanity binary through its nested Makefile.
- Run the emulator from the `test` target.

Verification:

- `make test` works from a clean tree.
- A nested assembler or linker failure fails the outer Make.
- A nonzero emulator exit status fails the outer Make.

### Checkpoint 5: Add Optional Diagnostic Builds

Only after the basic dependency graph works, consider a sanitizer target or a
documented way to override compilation options.

Avoid adding a large debug/release configuration system during the initial
emulator phase.

## 15. Useful Make Debugging Commands

### Preview commands

```shell
make -n
```

Shows commands without executing them.

### Explain rebuild decisions

```shell
make --trace
```

Shows which rule was selected and why it was considered out of date.

### Force rebuilding

```shell
make -B
```

Useful for checking recipes independently of timestamps.

### Test parallel safety

```shell
make -j
```

If this fails while a serial build succeeds, the dependency graph is probably
missing an ordering relationship.

### Verify from a clean state

```shell
make clean
make
make test
```

Changing compiler flags does not normally change any file timestamp known to
Make. After changing important flags, perform a clean rebuild.

## 16. Common Mistakes

- Treating the Makefile as a sequential shell script.
- Using spaces instead of a tab at the start of a recipe.
- Searching all directories for include paths.
- Compiling headers as if they were source files.
- Omitting header dependencies.
- Marking real output files as phony.
- Forgetting to mark action targets as phony.
- Using `make` instead of `$(MAKE)` for a nested build.
- Hiding test failures with `-` or `|| true`.
- Using direct `ld` for an ordinary hosted C executable.
- Deleting files that the Makefile did not generate.
- Expecting Make to notice a changed compiler flag automatically.
- Assuming commands on separate recipe lines share shell state.

## 17. Questions to Answer Before Considering the Build Complete

1. What exact file does each rule produce?
2. Are all inputs needed to produce that file declared as prerequisites?
3. Which targets are real files and which are actions?
4. Does changing one C source rebuild only the necessary object?
5. Does changing a header rebuild every affected object?
6. Does the build work from a completely clean directory?
7. Does a failed compiler, assembler, linker, or emulator command propagate a
   nonzero status?
8. Does `clean` remove only generated files?
9. Can a caller override the log level or compiler without editing the
   Makefile?
10. Does `make -j` respect every required dependency?

Once these questions have clear answers, the Makefile is already strong enough
for the current Phase 1 emulator skeleton.
