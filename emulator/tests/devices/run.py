from pathlib import Path
import subprocess

RED = "\033[1;31m"
GREEN = "\033[1;32m"
RESET = "\033[0m"

MAX_INSTRUCTIONS = 10000

emulator_path = Path(__file__).resolve().parents[2]
device_tests_path = emulator_path / "tests/devices"
rvemu = emulator_path / "rvemu"

DEVICE_TESTS = (
    "syscon",
)


class DeviceTest:
    def __init__(self, name):
        self.name = name
        self.returncode = 1

    def run(self):
        print(f"Running device test: {self.name}")
        test_path = device_tests_path / self.name

        build = subprocess.run(
            ["make", "--no-print-directory", "all"],
            cwd=test_path,
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            self.returncode = build.returncode
            self._print_output(build)
            return False

        program = test_path / "build" / f"{self.name}.elf"
        result = subprocess.run(
            [
                str(rvemu),
                "--max-instruction",
                str(MAX_INSTRUCTIONS),
                "--format",
                "elf",
                str(program),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        self.returncode = result.returncode
        if self.returncode != 0:
            self._print_output(result)
        return self.returncode == 0

    @staticmethod
    def _print_output(result):
        if result.stdout:
            print(result.stdout.strip())
        if result.stderr:
            print(result.stderr.strip())

    def summary(self):
        if self.returncode == 0:
            print(f"{GREEN}[PASS]{RESET} {self.name}")
        else:
            print(f"{RED}[FAIL]{RESET} {self.name}")


def print_test_result(passed):
    if passed:
        print(
            GREEN
            + "╔══════════════════════════════════════╗\n"
            + "║      ALL DEVICE TESTS PASSED         ║\n"
            + "╚══════════════════════════════════════╝"
            + RESET
        )
    else:
        print(
            RED
            + "╔══════════════════════════════════════╗\n"
            + "║        DEVICE TESTS FAILED           ║\n"
            + "╚══════════════════════════════════════╝"
            + RESET
        )


def run_all_tests():
    passed = True
    tests = []

    for name in DEVICE_TESTS:
        test = DeviceTest(name)
        passed &= test.run()
        tests.append(test)

    print_test_result(passed)
    for test in tests:
        test.summary()

    if passed:
        return 0
    return -1


if __name__ == "__main__":
    raise SystemExit(run_all_tests())
