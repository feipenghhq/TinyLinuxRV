"""
Copyright (c) 2026 Heqing Huang
SPDX-License-Identifier: MIT
"""

# Report Test result from result.xml

RED = "\033[1;31m"
GREEN = "\033[1;32m"
RESET = "\033[0m"

import xml.etree.ElementTree as ET

def print_test_result(passed):
    if passed:
        print(
            GREEN
            + "╔══════════════════════════════════════╗\n"
            + "║          REGRESSION PASSED           ║\n"
            + "╚══════════════════════════════════════╝"
            + RESET
        )
    else:
        print(
            RED
            + "╔══════════════════════════════════════╗\n"
            + "║          REGRESSION FAILED           ║\n"
            + "╚══════════════════════════════════════╝"
            + RESET
        )

def check_result():
    for testsuite in ET.parse("results.xml").iter("testsuite"):
        errors   = int(testsuite.get('errors'))
        failures = int(testsuite.get('failures'))
        skipped  = int(testsuite.get('skipped'))
        tests    = int(testsuite.get('tests'))
        passed   = tests - failures - skipped

        if errors == 0 and failures == 0:
            print_test_result(True)
            print(f"\nTESTS={tests} PASS={passed} FAIL={failures} SKIP={skipped}")
        else:
            print_test_result(False)
            print(f"\nTESTS={tests} PASS={passed} FAIL={failures} SKIP={skipped}")
            report_failure()

def report_failure():
    print(RED)
    print("Failed Tests:")
    for test in ET.parse("results.xml").iter("testcase"):
        for failure in list(test.findall("failure")) + list(test.findall("error")):
            print(f"- {test.get('classname')}.{test.get('name')}: "
                f"{failure.get('message', '')}")
    print(RESET)

check_result()

