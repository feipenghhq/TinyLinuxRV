from pathlib import Path
import subprocess
import shlex

RED = "\033[1;31m"
GREEN = "\033[1;32m"
RESET = "\033[0m"

MAX_INSTRUCTIONS = 10000

repo_path = subprocess.check_output(
    ["git", "rev-parse", "--show-toplevel"],
    text=True
).strip()
emulator_path = Path(repo_path) / "emulator"
sanity_tests_path = emulator_path / "tests/sanity/build"
rvemu = Path(emulator_path) / "rvemu"

class Test:

    def __init__(self, test):
        self.test = test

    def run(self):
        print(f"Running sanity test: {self.test}")
        full_path = (sanity_tests_path / self.test).with_suffix(".bin")
        cmd = [str(rvemu), "--max-instruction", str(MAX_INSTRUCTIONS), "--format", "bin", str(full_path)]
        #print(shlex.join(cmd))
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            check=False,
        )
        self.returncode = result.returncode
        return self.returncode == 0

    def summary(self):
        if self.returncode == 0:
            print(f"{GREEN}[PASS]{RESET} {self.test}")
        else:
            print(f"{RED}[FAIL]{RESET} {self.test}")

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
    rv64i = Test("rv64i")
    rv64m = Test("rv64m")
    rv64a = Test("rv64a")

    passed = True
    passed &= rv64i.run()
    passed &= rv64m.run()
    passed &= rv64a.run()

    print_test_result(passed)
    rv64i.summary()
    rv64m.summary()
    rv64a.summary()

    if passed:
        return 0
    else:
        return -1

if __name__ == "__main__":
    raise SystemExit(run_all_suites())
