# riscv-tests Makefile Guide

English | [中文](#中文版)

This document explains the design and behavior of
`emulator/tests/riscv-test/Makefile`.

The central idea of this Makefile is:

> Describe the build process shared by all ISA test suites in one template,
> then generate the rules for each enabled suite from `SUITES`.

The current high-level flow is:

```text
SUITES := rv64ui
        │
        ▼
include rv64ui/Makefrag
        │
        ▼
obtain test names such as add, addi, and sub
        │
        ▼
SUITE_template generates rules for rv64ui
        │
        ▼
add.S → add.o → add.elf → add.bin
```

## 1. Default goal

```make
.DEFAULT_GOAL := all
```

When you run:

```shell
make
```

Make builds `all`, which is equivalent to:

```shell
make all
```

The suite rules later in the file are generated dynamically with `eval`.
Declaring the default goal explicitly prevents Make from accidentally treating
another target encountered during parsing as the default goal.

## 2. Repository paths

```make
REPO_ROOT := $(shell git rev-parse --show-toplevel)
```

`$(shell ...)` runs a shell command and uses its standard output as the value
of the variable. In this repository, `REPO_ROOT` has a value similar to:

```text
/home/feipenghhq/Desktop/TinyLinuxRV
```

The remaining paths are constructed relative to the repository root:

```make
RISCV_TESTS   := $(REPO_ROOT)/third-party/riscv-tests
ISA_DIR       := $(RISCV_TESTS)/isa
ENV_DIR       := env
BUILD_DIR     := build
LINKER_SCRIPT := $(ENV_DIR)/link.ld
```

These variables refer to:

- the upstream `riscv-tests` repository;
- the upstream ISA test directory;
- the TinyLinuxRV emulator test environment;
- the output directory for generated test files;
- the linker script used by the emulator tests.

Defining paths in one place avoids repeated hard-coded paths and makes future
directory changes easier.

## 3. Header search paths

```make
CPPFLAGS := -I$(ENV_DIR)
CPPFLAGS += -I$(ISA_DIR)/macros/scalar
```

An upstream test source normally contains:

```asm
#include "riscv_test.h"
#include "test_macros.h"
```

These two files come from:

```text
emulator/tests/riscv-test/env/riscv_test.h
third-party/riscv-tests/isa/macros/scalar/test_macros.h
```

`CPPFLAGS` is the conventional Make variable for preprocessor options:

- `-I...` adds a header search path;
- `-D...` defines a preprocessor macro.

Although the inputs here are assembly files, uppercase `.S` files pass through
the C preprocessor, so `CPPFLAGS` is the appropriate variable.

## 4. Header dependencies

```make
ENV_HEADERS := $(wildcard $(ENV_DIR)/*.h)
TEST_MACROS := $(ISA_DIR)/macros/scalar/test_macros.h
```

`wildcard` returns files that match a pattern. For example:

```make
$(wildcard env/*.h)
```

may expand to:

```text
env/encoding.h env/riscv_test.h
```

These files become prerequisites of each test object. If `riscv_test.h`,
`encoding.h`, or `test_macros.h` changes, Make recompiles the affected tests
instead of continuing to use stale `.o` files.

## 5. RISC-V cross-toolchain

```make
CROSS_COMPILE ?= riscv64-linux-gnu-
CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
```

`?=` assigns a default only when the variable has not already been defined.
The default tools are therefore:

```text
riscv64-linux-gnu-gcc
riscv64-linux-gnu-ld
riscv64-linux-gnu-objcopy
```

If another development environment uses `riscv64-unknown-elf-`, override the
prefix from the command line:

```shell
make CROSS_COMPILE=riscv64-unknown-elf-
```

The three tools have different responsibilities:

- `gcc` preprocesses and assembles a `.S` file into a `.o` file;
- `ld` creates an ELF executable according to the linker script;
- `objcopy` extracts a raw binary from the ELF executable.

The upstream `.S` files cannot be compiled directly with `as`, because `as`
does not process C-style `#include` and `#define` directives.

## 6. Linker options

```make
LDFLAGS := -T $(LINKER_SCRIPT)
```

This expands to:

```text
-T env/link.ld
```

The linker script defines:

- the `_start` entry point;
- the `0x80000000` start address;
- the layout of `.text.init`, `.text`, `.data`, and `.bss`.

The linker script should be the single source of truth for the memory layout,
so the build does not also pass `-Ttext 0x80000000`.

## 7. Enabled test suites

```make
SUITES ?= rv64ui
```

Only `rv64ui` is enabled at present:

- `rv64` means 64-bit RISC-V;
- `u` means user-level instruction tests;
- `i` means the base integer instruction set.

After other extensions are implemented, more suites can be enabled:

```make
SUITES := rv64ui rv64um rv64ua
```

The value can also be overridden temporarily from the command line:

```shell
make SUITES=rv64ui
```

## 8. Compiler settings for each suite

```make
rv64ui_MARCH := rv64i
rv64ui_MABI  := lp64
```

Suite-specific variables follow this naming convention:

```text
<suite>_<setting>
```

This allows every suite to provide its own `-march` and `-mabi` values.

For example, after implementing the M extension:

```make
SUITES += rv64um
rv64um_MARCH := rv64im
rv64um_MABI  := lp64
```

After implementing the A extension:

```make
SUITES += rv64ua
rv64ua_MARCH := rv64ia
rv64ua_MABI  := lp64
```

## 9. Loading the upstream Makefrag

```make
include $(addsuffix /Makefrag,$(addprefix $(ISA_DIR)/,$(SUITES)))
```

Assume that:

```make
SUITES := rv64ui
```

First, `addprefix` adds the directory prefix:

```make
$(addprefix $(ISA_DIR)/,$(SUITES))
```

This produces:

```text
.../riscv-tests/isa/rv64ui
```

Next, `addsuffix` appends the Makefrag filename:

```make
$(addsuffix /Makefrag,...)
```

The final result is:

```text
.../riscv-tests/isa/rv64ui/Makefrag
```

The whole expression is logically equivalent to:

```make
include .../riscv-tests/isa/rv64ui/Makefrag
```

That Makefrag provides a list such as:

```make
rv64ui_sc_tests = add addi addiw addw ...
```

TinyLinuxRV therefore does not need to copy and maintain the upstream test
name list. If `SUITES` contains multiple suites, this expression includes all
of their Makefrag files.

## 10. The suite template

```make
define SUITE_template
...
endef
```

`define` creates a multi-line Make variable. In this Makefile, it acts as a
rule generator. It does not become part of Make's dependency graph until it is
instantiated through `call` and `eval`.

Inside the template:

```make
$(1)
```

represents the first argument passed to `call`. For example:

```make
$(call SUITE_template,rv64ui)
```

means that:

```text
$(1) = rv64ui
```

## 11. Generating test names and output lists

The template first obtains the test names provided by the Makefrag:

```make
$(1)_NAMES := $$($(1)_sc_tests)
```

With `rv64ui` as the argument, this is logically equivalent to:

```make
rv64ui_NAMES := $(rv64ui_sc_tests)
```

It then generates three output lists:

```make
$(1)_OBJECTS := ...
$(1)_ELFS    := ...
$(1)_BINS    := ...
```

If the only test names were `add` and `addi`, the expanded lists would look
approximately like this:

```make
rv64ui_OBJECTS := \
    build/rv64ui/add.o \
    build/rv64ui/addi.o

rv64ui_ELFS := \
    build/rv64ui/add.elf \
    build/rv64ui/addi.elf

rv64ui_BINS := \
    build/rv64ui/add.bin \
    build/rv64ui/addi.bin
```

This expression:

```make
$(addsuffix .o,$(rv64ui_NAMES))
```

first produces:

```text
add.o addi.o
```

`addprefix` then adds the output directory:

```text
build/rv64ui/add.o build/rv64ui/addi.o
```

Including the suite name in the output path prevents identically named tests
from different suites from overwriting each other.

## 12. The suite target

```make
$(1): $$($(1)_OBJECTS) $$($(1)_ELFS) $$($(1)_BINS)
```

With `rv64ui` as the argument, this is logically equivalent to:

```make
rv64ui: $(rv64ui_OBJECTS) $(rv64ui_ELFS) $(rv64ui_BINS)
```

As a result:

```shell
make rv64ui
```

generates every `.o`, `.elf`, and `.bin` file in the suite.

Listing all three output types explicitly also prevents Make from treating the
`.o` and `.elf` files as intermediate files and deleting them after the build.

## 13. Building `.o` from `.S`

The pattern rule in the template is:

```make
$(BUILD_DIR)/$(1)/%.o: $(ISA_DIR)/$(1)/%.S $(ENV_HEADERS) $(TEST_MACROS)
	mkdir -p $$(@D)
	$$(CC) -c \
		-march=$$($(1)_MARCH) \
		-mabi=$$($(1)_MABI) \
		$$(CPPFLAGS) \
		$$< -o $$@
```

When the target is:

```text
build/rv64ui/add.o
```

`%` matches `add`, so the first prerequisite is:

```text
third-party/riscv-tests/isa/rv64ui/add.S
```

The complete dependency relationship looks like:

```text
build/rv64ui/add.o
    ├── third-party/riscv-tests/isa/rv64ui/add.S
    ├── env/riscv_test.h
    ├── env/encoding.h
    └── third-party/riscv-tests/isa/macros/scalar/test_macros.h
```

The recipe uses Make's automatic variables:

| Variable | Meaning | Example for `add.o` |
| --- | --- | --- |
| `$@` | Current target | `build/rv64ui/add.o` |
| `$<` | First prerequisite | `.../rv64ui/add.S` |
| `$(@D)` | Directory of the target | `build/rv64ui` |

The first command therefore creates the target directory:

```shell
mkdir -p build/rv64ui
```

The final compiler command is similar to:

```shell
riscv64-linux-gnu-gcc \
    -c \
    -march=rv64i \
    -mabi=lp64 \
    -Ienv \
    -I.../isa/macros/scalar \
    .../rv64ui/add.S \
    -o build/rv64ui/add.o
```

## 14. Linking `.elf` from `.o`

```make
$(BUILD_DIR)/$(1)/%.elf: $(BUILD_DIR)/$(1)/%.o $(LINKER_SCRIPT)
	$$(LD) $$(LDFLAGS) $$< -o $$@
```

For `add`, the dependency relationship is:

```text
build/rv64ui/add.elf
    ├── build/rv64ui/add.o
    └── env/link.ld
```

The actual command is similar to:

```shell
riscv64-linux-gnu-ld \
    -T env/link.ld \
    build/rv64ui/add.o \
    -o build/rv64ui/add.elf
```

Because `link.ld` is a prerequisite, changing the linker script regenerates
the ELF files without recompiling unchanged `.S` files.

An ELF file retains entry-point, section, and symbol information, which makes
it useful for inspection and disassembly:

```shell
riscv64-linux-gnu-objdump -d build/rv64ui/add.elf
```

## 15. Generating `.bin` from `.elf`

```make
$(BUILD_DIR)/$(1)/%.bin: $(BUILD_DIR)/$(1)/%.elf
	$$(OBJCOPY) -O binary $$< $$@
```

The actual command is similar to:

```shell
riscv64-linux-gnu-objcopy \
    -O binary \
    build/rv64ui/add.elf \
    build/rv64ui/add.bin
```

A `.bin` file is a flat raw binary. It does not contain an ELF header, section
table, or symbol table. TinyLinuxRV's current raw-binary loader reads this
file.

## 16. Why the template uses `$$`

This is the most easily misunderstood part of the Makefile.

An ordinary rule can directly use:

```make
$(CC)
$@
$<
```

The template, however, goes through two rounds of Make expansion:

```text
First round:  call + eval expand the template
Second round: Make uses the generated rule to build a target
```

If the template directly contained `$@`, it could expand too early during
`eval`. No target is being built at that point, so the result might be empty.

Writing:

```make
$$@
```

causes `$$` to become `$` during the first round, preserving:

```make
$@
```

During the second round, when Make executes the concrete rule, it becomes:

```text
build/rv64ui/add.o
```

One way to remember this is:

> `$$` inside the template protects one `$` for the next expansion pass.

In contrast, `$(1)` is a parameter to `call` and must expand immediately, so
it uses a single `$`.

For example:

```make
$$($(1)_MARCH)
```

With `rv64ui` as the argument, the first expansion produces:

```make
$(rv64ui_MARCH)
```

The second expansion then produces:

```text
rv64i
```

## 17. Instantiating the template

```make
$(foreach suite,$(SUITES),$(eval $(call SUITE_template,$(suite))))
```

This expression contains three operations:

1. `foreach` iterates over `SUITES`;
2. `call` passes the suite name to the template;
3. `eval` parses the generated text as Makefile syntax.

Assume that:

```make
SUITES := rv64ui rv64um
```

Make performs the logical equivalent of these two instantiations:

```make
$(eval $(call SUITE_template,rv64ui))
$(eval $(call SUITE_template,rv64um))
```

This generates two sets of target lists and pattern rules.

## 18. `.PHONY` targets

```make
.PHONY: all clean $(SUITES)
```

With the current configuration, this expands approximately to:

```make
.PHONY: all clean rv64ui
```

These names represent actions rather than real files. If a file named `all`,
`clean`, or `rv64ui` happens to exist, Make will still process the targets
correctly.

## 19. Top-level target

```make
all: $(SUITES)
```

At present, this is equivalent to:

```make
all: rv64ui
```

If the enabled suites later become:

```make
SUITES := rv64ui rv64um rv64ua
```

the rule automatically becomes equivalent to:

```make
all: rv64ui rv64um rv64ua
```

The `all` rule therefore does not need to be edited whenever a suite is added.

## 20. Clean target

```make
clean:
	rm -rf $(BUILD_DIR)
```

This removes only the `build/` directory generated by this riscv-tests build.

## 21. How Make decides what to rebuild

For `add.bin`, the dependency graph is:

```text
add.S ─────────────┐
riscv_test.h ──────┤
encoding.h ────────┼─→ add.o ─┐
test_macros.h ─────┘          │
                              ├─→ add.elf ─→ add.bin
link.ld ──────────────────────┘
```

Make compares the modification times of targets and prerequisites:

- if `add.S` changes, Make regenerates `.o`, `.elf`, and `.bin`;
- if an environment header changes, Make regenerates `.o`, `.elf`, and `.bin`;
- if `link.ld` changes, Make keeps `.o` and regenerates `.elf` and `.bin`;
- if only `.bin` is deleted, Make only reruns `objcopy`;
- if nothing has changed, Make runs no build commands.

Make is therefore not a shell script executed from top to bottom. It is a
dependency-graph evaluator.

## 22. Common commands

Enter the test directory:

```shell
cd emulator/tests/riscv-test
```

Build every enabled suite:

```shell
make
```

Build in parallel:

```shell
make -j4
```

Build only RV64UI:

```shell
make rv64ui
```

Build one raw binary:

```shell
make build/rv64ui/add.bin
```

Build one ELF executable:

```shell
make build/rv64ui/add.elf
```

Print commands without executing them:

```shell
make -n
```

Remove generated files:

```shell
make clean
```

## 23. Adding a new test suite

After implementing the M extension, the usual configuration change is:

```make
SUITES += rv64um
rv64um_MARCH := rv64im
rv64um_MABI  := lp64
```

The Makefile then automatically:

1. includes `rv64um/Makefrag`;
2. reads `rv64um_sc_tests`;
3. creates output lists under `build/rv64um/`;
4. generates the `.S → .o → .elf → .bin` rules;
5. makes `all` build both the RV64I and RV64M tests.

Note that `rv64mi` and `rv64si` are privileged-architecture tests. They need a
test environment with CSR, trap, and privilege-mode transition support. They
cannot run merely by adding a suite and changing `-march`.

---

<a id="中文版"></a>

[English](#riscv-tests-makefile-guide) | 中文

# riscv-tests Makefile 说明（中文版）

本文解释 `emulator/tests/riscv-test/Makefile` 的设计和工作方式。

这个 Makefile 的核心思想是：

> 使用一份模板描述所有 ISA 测试套件共有的构建流程，再根据
> `SUITES` 自动生成各个测试套件的具体规则。

当前的整体流程如下：

```text
SUITES := rv64ui
        │
        ▼
include rv64ui/Makefrag
        │
        ▼
获得 add、addi、sub 等测试名称
        │
        ▼
SUITE_template 为 rv64ui 生成构建规则
        │
        ▼
add.S → add.o → add.elf → add.bin
```

## 1. 默认目标

```make
.DEFAULT_GOAL := all
```

当直接运行：

```shell
make
```

Make 会构建 `all`，等价于：

```shell
make all
```

后面的 suite 规则由 `eval` 动态生成，因此显式指定默认目标可以避免 Make
把解析过程中遇到的其他目标误认为默认目标。

## 2. 仓库路径

```make
REPO_ROOT := $(shell git rev-parse --show-toplevel)
```

`$(shell ...)` 会执行 shell 命令，并把命令的标准输出作为变量值。
在当前仓库中，`REPO_ROOT` 的值类似：

```text
/home/feipenghhq/Desktop/TinyLinuxRV
```

其他路径以仓库根目录为基础构造：

```make
RISCV_TESTS   := $(REPO_ROOT)/third-party/riscv-tests
ISA_DIR       := $(RISCV_TESTS)/isa
ENV_DIR       := env
BUILD_DIR     := build
LINKER_SCRIPT := $(ENV_DIR)/link.ld
```

这些变量分别指向：

- 上游 `riscv-tests` 仓库；
- 上游 ISA 测试目录；
- TinyLinuxRV 的 emulator 测试环境；
- 当前测试的构建输出目录；
- emulator 测试使用的链接脚本。

集中定义路径可以避免重复硬编码，也方便以后调整目录结构。

## 3. 头文件搜索路径

```make
CPPFLAGS := -I$(ENV_DIR)
CPPFLAGS += -I$(ISA_DIR)/macros/scalar
```

上游测试源文件通常包含：

```asm
#include "riscv_test.h"
#include "test_macros.h"
```

这两个文件分别来自：

```text
emulator/tests/riscv-test/env/riscv_test.h
third-party/riscv-tests/isa/macros/scalar/test_macros.h
```

`CPPFLAGS` 是 Make 中常用于保存预处理器参数的变量：

- `-I...`：增加头文件搜索路径；
- `-D...`：定义预处理宏。

这里编译的虽然是汇编文件，但大写 `.S` 文件需要经过 C 预处理器，所以使用
`CPPFLAGS` 是合适的。

## 4. 头文件依赖

```make
ENV_HEADERS := $(wildcard $(ENV_DIR)/*.h)
TEST_MACROS := $(ISA_DIR)/macros/scalar/test_macros.h
```

`wildcard` 会列出与给定模式匹配的文件。例如：

```make
$(wildcard env/*.h)
```

可能得到：

```text
env/encoding.h env/riscv_test.h
```

这些文件会成为测试目标文件的 prerequisites。这样，修改
`riscv_test.h`、`encoding.h` 或 `test_macros.h` 后，Make 会重新编译受影响的
测试，而不会继续使用过期的 `.o` 文件。

## 5. RISC-V 交叉工具链

```make
CROSS_COMPILE ?= riscv64-linux-gnu-
CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
```

`?=` 表示只在变量尚未定义时赋予默认值。因此默认工具为：

```text
riscv64-linux-gnu-gcc
riscv64-linux-gnu-ld
riscv64-linux-gnu-objcopy
```

如果另一套开发环境使用 `riscv64-unknown-elf-`，可以在命令行覆盖：

```shell
make CROSS_COMPILE=riscv64-unknown-elf-
```

三个工具的职责分别是：

- `gcc`：预处理并汇编 `.S` 文件，生成 `.o`；
- `ld`：根据链接脚本生成 ELF；
- `objcopy`：从 ELF 提取 raw binary。

这里不能直接用 `as` 编译上游 `.S` 文件，因为 `as` 不会处理 C 风格的
`#include` 和 `#define`。

## 6. 链接选项

```make
LDFLAGS := -T $(LINKER_SCRIPT)
```

展开后为：

```text
-T env/link.ld
```

链接脚本负责定义：

- `_start` 入口；
- `0x80000000` 起始地址；
- `.text.init`、`.text`、`.data` 和 `.bss` 的排列方式。

内存布局应由链接脚本统一控制，因此不再同时传递
`-Ttext 0x80000000`。

## 7. 启用的测试套件

```make
SUITES ?= rv64ui
```

当前只启用 `rv64ui`：

- `rv64`：64 位 RISC-V；
- `u`：用户态指令测试；
- `i`：基础整数指令集。

将来实现其他扩展后，可以增加 suite：

```make
SUITES := rv64ui rv64um rv64ua
```

也可以临时从命令行覆盖：

```shell
make SUITES=rv64ui
```

## 8. 每个 suite 的编译参数

```make
rv64ui_MARCH := rv64i
rv64ui_MABI  := lp64
```

变量采用以下命名方式：

```text
<suite>_<setting>
```

因此每个 suite 都可以有自己的 `-march` 和 `-mabi` 参数。

例如，加入 M 扩展时可以写：

```make
SUITES += rv64um
rv64um_MARCH := rv64im
rv64um_MABI  := lp64
```

加入 A 扩展时可以写：

```make
SUITES += rv64ua
rv64ua_MARCH := rv64ia
rv64ua_MABI  := lp64
```

## 9. 加载上游 Makefrag

```make
include $(addsuffix /Makefrag,$(addprefix $(ISA_DIR)/,$(SUITES)))
```

假设：

```make
SUITES := rv64ui
```

首先，`addprefix` 添加目录前缀：

```make
$(addprefix $(ISA_DIR)/,$(SUITES))
```

得到：

```text
.../riscv-tests/isa/rv64ui
```

然后，`addsuffix` 添加文件名后缀：

```make
$(addsuffix /Makefrag,...)
```

最终得到：

```text
.../riscv-tests/isa/rv64ui/Makefrag
```

整行在逻辑上等价于：

```make
include .../riscv-tests/isa/rv64ui/Makefrag
```

这个 Makefrag 提供：

```make
rv64ui_sc_tests = add addi addiw addw ...
```

因此 TinyLinuxRV 不需要复制和维护上游测试名称列表。

如果 `SUITES` 中有多个 suite，这一行会同时包含多个 Makefrag。

## 10. suite 模板

```make
define SUITE_template
...
endef
```

`define` 创建一个多行 Make 变量。这里可以把它理解为一个“规则生成器”。
它本身不是实际的构建规则，只有通过 `call` 和 `eval` 实例化后，才会成为
Make 依赖图的一部分。

模板中的：

```make
$(1)
```

代表传入的第一个参数。例如：

```make
$(call SUITE_template,rv64ui)
```

调用时：

```text
$(1) = rv64ui
```

## 11. 生成测试名称和产物列表

模板首先取得 Makefrag 提供的测试名称：

```make
$(1)_NAMES := $$($(1)_sc_tests)
```

传入 `rv64ui` 后，它在逻辑上等价于：

```make
rv64ui_NAMES := $(rv64ui_sc_tests)
```

随后生成三组产物：

```make
$(1)_OBJECTS := ...
$(1)_ELFS    := ...
$(1)_BINS    := ...
```

假设测试名称只有 `add` 和 `addi`，展开结果大致是：

```make
rv64ui_OBJECTS := \
    build/rv64ui/add.o \
    build/rv64ui/addi.o

rv64ui_ELFS := \
    build/rv64ui/add.elf \
    build/rv64ui/addi.elf

rv64ui_BINS := \
    build/rv64ui/add.bin \
    build/rv64ui/addi.bin
```

其中：

```make
$(addsuffix .o,$(rv64ui_NAMES))
```

先得到：

```text
add.o addi.o
```

再使用 `addprefix` 得到完整输出路径：

```text
build/rv64ui/add.o build/rv64ui/addi.o
```

把 suite 名放进输出路径，可以避免不同测试套件中的同名测试互相覆盖。

## 12. suite 目标

```make
$(1): $$($(1)_OBJECTS) $$($(1)_ELFS) $$($(1)_BINS)
```

传入 `rv64ui` 后，逻辑上等价于：

```make
rv64ui: $(rv64ui_OBJECTS) $(rv64ui_ELFS) $(rv64ui_BINS)
```

因此：

```shell
make rv64ui
```

会生成该 suite 的全部 `.o`、`.elf` 和 `.bin` 文件。

显式依赖三种产物还有一个作用：Make 不会把 `.o` 和 `.elf` 当作临时的
intermediate files，在构建完成后自动删除。

## 13. 从 `.S` 构建 `.o`

模板中的 pattern rule 为：

```make
$(BUILD_DIR)/$(1)/%.o: $(ISA_DIR)/$(1)/%.S $(ENV_HEADERS) $(TEST_MACROS)
	mkdir -p $$(@D)
	$$(CC) -c \
		-march=$$($(1)_MARCH) \
		-mabi=$$($(1)_MABI) \
		$$(CPPFLAGS) \
		$$< -o $$@
```

当目标是：

```text
build/rv64ui/add.o
```

`%` 匹配 `add`，所以第一个 prerequisite 是：

```text
third-party/riscv-tests/isa/rv64ui/add.S
```

完整依赖关系类似：

```text
build/rv64ui/add.o
    ├── third-party/riscv-tests/isa/rv64ui/add.S
    ├── env/riscv_test.h
    ├── env/encoding.h
    └── third-party/riscv-tests/isa/macros/scalar/test_macros.h
```

recipe 中使用了 Make 自动变量：

| 变量 | 含义 | `add.o` 示例 |
| --- | --- | --- |
| `$@` | 当前 target | `build/rv64ui/add.o` |
| `$<` | 第一个 prerequisite | `.../rv64ui/add.S` |
| `$(@D)` | target 所在目录 | `build/rv64ui` |

因此第一条命令会创建目标目录：

```shell
mkdir -p build/rv64ui
```

编译命令最终类似：

```shell
riscv64-linux-gnu-gcc \
    -c \
    -march=rv64i \
    -mabi=lp64 \
    -Ienv \
    -I.../isa/macros/scalar \
    .../rv64ui/add.S \
    -o build/rv64ui/add.o
```

## 14. 从 `.o` 链接 `.elf`

```make
$(BUILD_DIR)/$(1)/%.elf: $(BUILD_DIR)/$(1)/%.o $(LINKER_SCRIPT)
	$$(LD) $$(LDFLAGS) $$< -o $$@
```

对于 `add`，依赖关系是：

```text
build/rv64ui/add.elf
    ├── build/rv64ui/add.o
    └── env/link.ld
```

实际命令类似：

```shell
riscv64-linux-gnu-ld \
    -T env/link.ld \
    build/rv64ui/add.o \
    -o build/rv64ui/add.elf
```

因为 `link.ld` 是 prerequisite，修改链接脚本后，Make 会重新生成 ELF，而
不必重新编译没有改变的 `.S` 文件。

ELF 保留入口、section 和 symbol 等信息，适合检查和反汇编：

```shell
riscv64-linux-gnu-objdump -d build/rv64ui/add.elf
```

## 15. 从 `.elf` 生成 `.bin`

```make
$(BUILD_DIR)/$(1)/%.bin: $(BUILD_DIR)/$(1)/%.elf
	$$(OBJCOPY) -O binary $$< $$@
```

实际命令类似：

```shell
riscv64-linux-gnu-objcopy \
    -O binary \
    build/rv64ui/add.elf \
    build/rv64ui/add.bin
```

`.bin` 是扁平 raw binary，不包含 ELF header、section table 或 symbol table。
TinyLinuxRV 当前的 raw binary loader 读取的就是这个文件。

## 16. 为什么模板中使用 `$$`

这是整个 Makefile 最容易混淆的部分。

普通规则可以直接使用：

```make
$(CC)
$@
$<
```

但是模板会经历两轮 Make 展开：

```text
第一轮：call + eval 展开模板
第二轮：Make 使用生成的规则构建目标
```

如果模板中直接写 `$@`，它可能在 `eval` 阶段过早展开。此时还没有正在构建
的 target，所以结果可能为空。

写成：

```make
$$@
```

第一轮展开时，`$$` 变成 `$`，保留下：

```make
$@
```

第二轮执行具体规则时，它才会变成：

```text
build/rv64ui/add.o
```

可以将其理解为：

> 模板中的 `$$` 用来保护一个 `$`，把它留给下一轮展开。

但是 `$(1)` 是 `call` 的参数，必须在第一轮立即展开，因此使用单个 `$`。

例如：

```make
$$($(1)_MARCH)
```

当参数为 `rv64ui` 时，第一轮得到：

```make
$(rv64ui_MARCH)
```

第二轮再得到：

```text
rv64i
```

## 17. 实例化模板

```make
$(foreach suite,$(SUITES),$(eval $(call SUITE_template,$(suite))))
```

这行由三层操作组成：

1. `foreach` 遍历 `SUITES`；
2. `call` 将 suite 名传给模板；
3. `eval` 把模板生成的文本作为 Makefile 内容重新解析。

假设：

```make
SUITES := rv64ui rv64um
```

Make 会执行逻辑上类似的两次实例化：

```make
$(eval $(call SUITE_template,rv64ui))
$(eval $(call SUITE_template,rv64um))
```

最终生成两组目标列表和 pattern rules。

## 18. `.PHONY` 目标

```make
.PHONY: all clean $(SUITES)
```

当前展开后类似：

```make
.PHONY: all clean rv64ui
```

这些名称表示动作，而不是真实文件。如果目录中意外出现名为 `all`、`clean`
或 `rv64ui` 的文件，Make 仍然会正常处理这些目标。

## 19. 顶层目标

```make
all: $(SUITES)
```

当前等价于：

```make
all: rv64ui
```

以后如果启用：

```make
SUITES := rv64ui rv64um rv64ua
```

它会自动等价于：

```make
all: rv64ui rv64um rv64ua
```

因此增加 suite 时不需要再手动修改 `all` 规则。

## 20. 清理目标

```make
clean:
	rm -rf $(BUILD_DIR)
```

它只删除当前 riscv-tests 构建产生的 `build/` 目录。

## 21. Make 如何判断是否需要重建

以 `add.bin` 为例，依赖图如下：

```text
add.S ─────────────┐
riscv_test.h ──────┤
encoding.h ────────┼─→ add.o ─┐
test_macros.h ─────┘          │
                              ├─→ add.elf ─→ add.bin
link.ld ──────────────────────┘
```

Make 比较 target 和 prerequisites 的修改时间：

- `add.S` 更新：重新生成 `.o`、`.elf` 和 `.bin`；
- 环境头文件更新：重新生成 `.o`、`.elf` 和 `.bin`；
- `link.ld` 更新：保留 `.o`，重新生成 `.elf` 和 `.bin`；
- 只有 `.bin` 被删除：只重新执行 `objcopy`；
- 所有文件都未变化：不执行任何构建命令。

Make 的本质不是从上到下执行的 shell 脚本，而是一个依赖图求值器。

## 22. 常用命令

进入测试目录：

```shell
cd emulator/tests/riscv-test
```

构建所有启用的 suite：

```shell
make
```

并行构建：

```shell
make -j4
```

只构建 RV64UI：

```shell
make rv64ui
```

只构建一个 binary：

```shell
make build/rv64ui/add.bin
```

只构建一个 ELF：

```shell
make build/rv64ui/add.elf
```

显示将要执行的命令，但不真正执行：

```shell
make -n
```

清理构建产物：

```shell
make clean
```

## 23. 添加新测试套件

实现 M 扩展后，通常只需要增加：

```make
SUITES += rv64um
rv64um_MARCH := rv64im
rv64um_MABI  := lp64
```

Makefile 会自动：

1. 包含 `rv64um/Makefrag`；
2. 读取 `rv64um_sc_tests`；
3. 创建 `build/rv64um/` 下的目标列表；
4. 生成 `.S → .o → .elf → .bin` 构建规则；
5. 让 `all` 同时构建 RV64I 和 RV64M 测试。

需要注意，`rv64mi` 和 `rv64si` 属于特权架构测试。它们需要 CSR、trap、
特权级切换等测试环境支持，不能只通过增加 suite 和修改 `-march` 直接运行。
