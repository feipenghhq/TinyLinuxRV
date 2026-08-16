from pathlib import Path
import subprocess
#import shlex

MAX_INSTRUCTIONS = 10000

repo_path = subprocess.check_output(
    ["git", "rev-parse", "--show-toplevel"],
    text=True
).strip()
emulator_path = Path(repo_path) / "emulator"
riscv_tests_path = emulator_path / "tests/riscv-test"
rvemu = Path(emulator_path) / "rvemu"

# -----------------------------------
# Common test function
# -----------------------------------
def run_suite(suite, skip_list=()):
    pass_list = []
    fail_list = []
    count = 0
    print(f"Running test suite: {suite}")
    with open(riscv_tests_path / f"build/{suite}/tests.txt") as tests_manifest:
        for line in tests_manifest:
            test = Path(line.strip()).stem
            if test in skip_list:
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
                pass_list.append(test)
            else:
                fail_list.append(test)
                print(result.stderr.strip())
            count += 1

    if len(fail_list) == 0:
        print(f"- Congratulations: Complete {count} tests. All tests passed!")
        return 0
    else:
        print(f"- Complete {count} tests. Passed {len(pass_list)} tests. Failed {len(fail_list)} tests")
        print(f"- The following test failed: {fail_list}")
        return 1


def run_all_suites():
    result = 0
    result |= run_suite("rv64ui", skip_list=("ma_data"))
    return result

if __name__ == "__main__":
    raise SystemExit(run_all_suites())
