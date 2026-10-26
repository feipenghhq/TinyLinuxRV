"""
Copyright (c) 2026 Heqing Huang
SPDX-License-Identifier: MIT
"""

# Main Test bench for CPU

import cocotb
from cpu import CPU

EMULATOR_PATH = '../../../emulator'

@cocotb.parametrize(
    file=[
        cocotb.Param("rv64i.bin", name="rv64i"),
        cocotb.Param("rv64m.bin", name="rv64m"),
        cocotb.Param("rv64a.bin", name="rv64a"),
    ]
)
async def sanity(dut, file='rv64i.bin' ):
    cpu = CPU(dut)
    cpu.load_bin(f'{EMULATOR_PATH}/tests/sanity/build/{file}')
    await cpu.run()
