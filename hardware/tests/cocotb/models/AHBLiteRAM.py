"""
Copyright (c) 2026 Heqing Huang
SPDX-License-Identifier: MIT
"""


# Cocotb Model for AHB-Lite Memory

import cocotb
from .AHBLite import AHBLiteBus, AHBLiteDevice

class AHBLiteRAM(AHBLiteDevice):

    def __init__(self, dut, nbytes, base_addr, size, name, signals=AHBLiteBus._SIGNALS, clock = None, bus_separator='_'):
        """
        Args:
            dut (_type_): Cocotb DUT
            nbytes (_type_):  AHB bus width as bytes, support 1 , 2, 4, 8
            base_addr (_type_): Base address of the device
            size (_type_): RAM size (in MiB)
            name (_type_): AHB bus name (prefix)
            signals (_type_, optional): AHB bus signal name. Defaults to _SIGNALS.
            clock (_type_, optional): Clock. If None, use dut.clk
            bus_separator (str, optional): separator between prefix and bus signal. Defaults to '_'.
        """
        super().__init__(dut, nbytes, base_addr, name, signals, clock, bus_separator=bus_separator)
        self.ram = bytearray(size * 1024 * 1024)

    def load_bin(self, file_name):
        cocotb.log.info(f"Loading binary file: {file_name}")
        with open(file_name, 'rb') as file:
            data = file.read()
            if len(data) > len(self.ram):
                raise ValueError("Files is too large to be loaded into the RAM.")
            self.ram[:len(data)] = data
            cocotb.log.info(f"Loaded {len(data)} bytes")

    def _read(self, haddr, hsize):
        offset = haddr & (self.nbytes - 1)
        addr = (haddr - self.base_addr) - offset
        data = 0
        if hsize == 0: # byte
            data = self.ram[addr] << (8 * offset)
        if hsize == 1: # half word
            data = int.from_bytes(self.ram[addr:addr+2], "little") << (16 * (offset >> 1))
        if hsize == 2: # word
            data = int.from_bytes(self.ram[addr:addr+4], "little") << (16 * (offset >> 2))
        if hsize == 2: # double word
            data = int.from_bytes(self.ram[addr:addr+8], "little")
        return data, 0  # HRESP = 0 (OK)

    def _write(self, haddr, hsize):
        pass
        # TBD
