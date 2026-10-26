"""
Copyright (c) 2026 Heqing Huang
SPDX-License-Identifier: MIT
"""

# Main Test bench for CPU

import cocotb
from cocotb.clock import Clock
from cocotb.triggers import Timer, RisingEdge, First
from cocotb.simtime import get_sim_time
from models.AHBLiteRAM import AHBLiteRAM


class CPU:
    """

    Main test bench for CPU.

    Use separate memory for instruction and data for now
    """

    _CLK_PERIOD = 10
    _RST_TIME   = 20

    def __init__(self, dut):
        self.inst_ram  = AHBLiteRAM(dut, 8, 0x80000000, 128, 'inst')
        self.data_ram  = AHBLiteRAM(dut, 8, 0x80000000, 128, 'data')
        self.dut = dut
        self.finish = False
        self._init_cpu()

    def load_bin(self, file_name):
        self.inst_ram.load_bin(file_name)
        self.data_ram.load_bin(file_name)

    def _init_cpu(self):
        # clear register to deterministic state for cocotb parametrize
        for i in range(32):
            self.dut.u_cpu.u_ID.u_regfile.register_file[i].value = 0

    async def _check_finish(self):
        clkedge = RisingEdge(self.dut.clk)
        while True:
            await clkedge
            if self.dut.test_finish.value == 1:
                self.finish = True
                cocotb.log.info(f"Test Finished at {get_sim_time(unit='ns')} ns")
                return

    def _check_result_rv_test(self):
        if not self.finish:
            cocotb.log.error("Test TIMEOUT!")
            raise ValueError("Test TIMEOUT!")
        else:
            if self.dut.test_result.value == 0:
                cocotb.log.error("Test PASS!")
            else:
                cocotb.log.info("Test FAIL!")
                raise ValueError("Test FAIL!")

    async def run(self, timeout=10000):
        # Initialize
        self._init_cpu()
        self.dut.rst_n.value = 0
        self.inst_ram.init_bus()
        self.data_ram.init_bus()

        # stat clock and tasks
        clock = Clock(self.dut.clk, CPU._CLK_PERIOD, unit="ns")
        clock.start()
        cocotb.start_soon(self.inst_ram.run())
        cocotb.start_soon(self.data_ram.run())
        monitor_finish = cocotb.start_soon(self._check_finish())
        monitor_timeout = Timer(timeout, unit='ns')

        # de-assert reset
        await Timer(CPU._RST_TIME, unit='ns')
        self.dut.rst_n.value = 1

        # wait till test complete
        await First(monitor_finish, monitor_timeout)
        self._check_result_rv_test()
