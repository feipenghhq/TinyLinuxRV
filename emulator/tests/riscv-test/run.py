import subprocess
from pathlib import Path

# import shlex

RED = "\033[1;31m"
GREEN = "\033[1;32m"
RESET = "\033[0m"

MAX_INSTRUCTIONS = 100000

repo_path = subprocess.check_output(
    ["git", "rev-parse", "--show-toplevel"], text=True
).strip()
emulator_path = Path(repo_path) / "emulator"
riscv_tests_path = emulator_path / "tests/riscv-test"
rvemu = Path(emulator_path) / "rvemu"


class TestSuite:
    def __init__(self, suite, env, skip_list=()):
        self.suite = suite
        self.env = env
        self.skip_list = skip_list

    def run(self):
        self.pass_list = []
        self.fail_list = []
        self.count = 0
        print(f"Running test suite: {self.suite}-{self.env}")
        with open(riscv_tests_path / f"build/{self.env}/{self.suite}/tests.txt") as tests_manifest:
            for line in tests_manifest:
                test = Path(line.strip()).stem
                if test in self.skip_list:
                    print(f"- Skip test: {test}")
                    continue
                full_path = riscv_tests_path / line.strip()
                full_path = full_path.with_suffix(".elf")
                cmd = [
                    str(rvemu),
                    "--max-instruction",
                    str(MAX_INSTRUCTIONS),
                    "--format",
                    "elf",
                    "--riscv-tests",
                    "--dram-size",
                    "1",
                    str(full_path),
                ]
                # print(shlex.join(cmd))
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    check=False,
                )
                if result.returncode == 0:
                    self.pass_list.append(test)
                else:
                    self.fail_list.append(test)
                    print(result.stderr.strip())
                self.count += 1
        return len(self.fail_list) == 0

    def summary(self):
        passed = len(self.pass_list)
        failed = len(self.fail_list)
        details = []

        if failed:
            details.append(f"failed: {', '.join(self.fail_list)}")
        if self.skip_list:
            details.append(f"skipped: {', '.join(self.skip_list)}")
        detail_str = f"  {'  '.join(details)}" if details else ""
        status = f"{GREEN}[PASS]{RESET}" if failed == 0 else f"{RED}[FAIL]{RESET}"
        print(f"{status} {self.suite}-{self.env} {passed:>2}/{self.count:<2}{detail_str}")


def print_test_result(passed):
    if passed:
        print(
            GREEN
            + "╔══════════════════════════════════════╗\n"
            + "║           ALL TESTS PASSED           ║\n"
            + "╚══════════════════════════════════════╝"
            + RESET
        )
    else:
        print(
            RED
            + "╔══════════════════════════════════════╗\n"
            + "║             TESTS FAILED             ║\n"
            + "╚══════════════════════════════════════╝"
            + RESET
        )


def run_all_suites():
    tests = []
    tests.append(TestSuite("rv64ui", 'p', skip_list=("ma_data",)))
    tests.append(TestSuite("rv64um", 'p'))
    tests.append(TestSuite("rv64ua", 'p'))
    tests.append(TestSuite("rv64mi", 'p', skip_list=("pmpaddr", "breakpoint")))
    tests.append(TestSuite("rv64si", 'p', skip_list=("dirty", "icache-alias")))

    tests.append(TestSuite("rv64ui", 'v', skip_list=("ma_data",)))
    tests.append(TestSuite("rv64um", 'v'))
    tests.append(TestSuite("rv64ua", 'v'))
    tests.append(TestSuite("rv64mi", 'v', skip_list=("pmpaddr", "breakpoint")))
    tests.append(TestSuite("rv64si", 'v', skip_list=("dirty", "icache-alias")))

    passed = True
    for test in tests:
        passed &= test.run()

    print_test_result(passed)
    for test in tests:
        test.summary()

    if passed:
        return 0
    else:
        return -1


if __name__ == "__main__":
    raise SystemExit(run_all_suites())
