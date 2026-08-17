from pathlib import Path
import subprocess

RED = "\033[1;31m"
GREEN = "\033[1;32m"
RESET = "\033[0m"

REG_LIST = (
    "sanity-test",
    "riscv-tests",
    "run-fib",
    "run-baremetal-test",
)


class RunRegression:
    def __init__(self, name):
        self.name = name

    def run(self):
        print(f"Running regression test: {self.name}")
        cmd = ["make", "--no-print-directory", self.name]
        result = subprocess.run(
            cmd,
            cwd=Path(__file__).resolve().parents[2],
            capture_output=True,
            text=True,
            check=False,
        )
        self.returncode = result.returncode
        return self.returncode == 0

    def summary(self):
        if self.returncode == 0:
            print(f"{GREEN}[PASS]{RESET} {self.name}")
        else:
            print(f"{RED}[FAIL]{RESET} {self.name}")


def print_result(passed):
    if passed:
        print(
            GREEN
            + "╔═══════════════════════════════════════════╗\n"
            + "║           ALL REGRESSION PASSED           ║\n"
            + "╚═══════════════════════════════════════════╝"
            + RESET
        )
    else:
        print(
            RED
            + "╔═══════════════════════════════════════════╗\n"
            + "║             REGRESSION FAILED             ║\n"
            + "╚═══════════════════════════════════════════╝"
            + RESET
        )


def run_all_test():

    passed = True
    regression = []
    for test in REG_LIST:
        run = RunRegression(test)
        passed &= run.run()
        regression.append(run)

    print_result(passed)
    for reg in regression:
        reg.summary()

    if passed:
        return 0
    else:
        return -1


if __name__ == "__main__":
    raise SystemExit(run_all_test())
