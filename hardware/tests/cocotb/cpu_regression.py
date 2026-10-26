"""
Copyright (c) 2026 Heqing Huang
SPDX-License-Identifier: MIT
"""

# Main Test bench for CPU

import subprocess
from pathlib import Path

import cocotb
from cpu import CPU

REPO_PATH     = subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()
EMULATOR_PATH = f'{REPO_PATH}/emulator'


@cocotb.parametrize(
    file=[
        cocotb.Param("rv64i.bin", name="rv64i"),
        cocotb.Param("rv64m.bin", name="rv64m"),
        cocotb.Param("rv64a.bin", name="rv64a"),
    ]
)
async def sanity(dut, file='rv64i.bin' ):
    cpu = CPU(dut, ram_size=1)
    cpu.load_bin(f'{EMULATOR_PATH}/tests/sanity/build/{file}')
    await cpu.run()

rv64ui = Path(f"{REPO_PATH}/third-party/riscv-tests/isa/rv64ui")
@cocotb.parametrize(
    file=[cocotb.Param(f"{f.stem}.bin", f.stem) for f in rv64ui.rglob("*.S")]
)
async def riscv_test_rv64i(dut, file='add.bin' ):
    cpu = CPU(dut, ram_size=1)
    try:
        cpu.load_bin(f'{EMULATOR_PATH}/tests/riscv-test/build/p/rv64ui/{file}')
        await cpu.run(timeout=10000)
    finally:
        cpu.close()
