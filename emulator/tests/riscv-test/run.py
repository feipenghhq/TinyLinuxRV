from pathlib import Path
import subprocess
#import shlex

RED = "\033[1;31m"
GREEN = "\033[1;32m"
RESET = "\033[0m"

MAX_INSTRUCTIONS = 10000

repo_path = subprocess.check_output(
    ["git", "rev-parse", "--show-toplevel"],
    text=True
).strip()
emulator_path = Path(repo_path) / "emulator"
riscv_tests_path = emulator_path / "tests/riscv-test"
rvemu = Path(emulator_path) / "rvemu"

class TestSuite:

    def __init__(self, suite, skip_list=()):
        self.suite = suite
        self.skip_list = skip_list

    def run(self):
        self.pass_list = []
        self.fail_list = []
        self.count = 0
        print(f"Running test suite: {self.suite}")
        with open(riscv_tests_path / f"build/{self.suite}/tests.txt") as tests_manifest:
            for line in tests_manifest:
                test = Path(line.strip()).stem
                if test in self.skip_list:
                    print(f"- Skip test: {test}")
                    continue
                full_path = riscv_tests_path / line.strip()
                full_path = full_path.with_suffix(".elf")
                cmd = [str(rvemu), "--max-instruction", str(MAX_INSTRUCTIONS), "--format", "elf", "--riscv-tests", str(full_path)]
                #print(shlex.join(cmd))
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

        if failed == 0:
            print(f"{GREEN}[PASS]{RESET} {self.suite:<8} {passed}/{self.count}")
        else:
            fail_names = ", ".join(self.fail_list)
            print(
                f"{RED}[FAIL]{RESET} {self.suite:<8} "
                f"{passed}/{self.count}  failed: {fail_names}"
            )


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
    rv64ui = TestSuite("rv64ui", skip_list=("ma_data"))
    rv64um = TestSuite("rv64um")
    rv64ua = TestSuite("rv64ua")

    passed = True
    passed &= rv64ui.run()
    passed &= rv64um.run()
    passed &= rv64ua.run()

    print_test_result(passed)
    rv64ui.summary()
    rv64um.summary()
    rv64ua.summary()

    if passed:
        return 0
    else:
        return -1

if __name__ == "__main__":
    raise SystemExit(run_all_suites())
