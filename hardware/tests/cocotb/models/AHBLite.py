"""
Copyright (c) 2026 Heqing Huang
SPDX-License-Identifier: MIT
"""

# Cocotb Model for AHB-Lite Bus

from cocotb.triggers import ReadWrite, RisingEdge
from cocotb_bus.bus import Bus


class AHBLiteBus(Bus):

    _SIGNALS = ('haddr', 'hsel', 'htrans', 'hsize', 'hwrite', 'hwdata', 'hrdata', 'hready', 'hresp')


    def __init__(self, dut, nbytes, name, signals=_SIGNALS, bus_separator='_'):
        """_summary_

        Args:
            dut (_type_): DUT
            nbytes (_type_): AHB bus width as bytes, support 1 , 2, 4, 8
            name (_type_): AHB bus name (prefix)
            signals (_type_, optional): AHB bus signal name. Defaults to _SIGNALS.
            bus_separator (str, optional): separator between prefix and bus signal. Defaults to '_'.
        """
        super().__init__(dut, name, signals, bus_separator=bus_separator)
        self.init_bus()
        self.nbytes = nbytes
        assert(nbytes in [1, 2, 4, 8])

    def init_bus(self):
        """AHB-Lite Bus default value"""
        self.hready.value = 1
        self.hrdata.value = 0
        self.hresp.value  = 0


class AHBLiteDevice(AHBLiteBus):

    def __init__(self, dut, nbytes, base_addr, name, signals=AHBLiteBus._SIGNALS, clock = None, bus_separator='_'):
        """
        Args:
            dut (_type_): Cocotb DUT
            nbytes (_type_):  AHB bus width as bytes, support 1 , 2, 4, 8
            base_addr (_type_): Base address of the device
            name (_type_): AHB bus name (prefix)
            signals (_type_, optional): AHB bus signal name. Defaults to _SIGNALS.
            clock (_type_, optional): Clock. If None, use dut.clk
            bus_separator (str, optional): separator between prefix and bus signal. Defaults to '_'.
        """
        super().__init__(dut, nbytes, name, signals, bus_separator=bus_separator)
        self.clock = clock if clock else dut.clk
        self.base_addr = base_addr

        # AHB
        self.req       = False
        self.req_write = False
        self.req_addr  = 0
        self.req_size  = 0

    async def run(self):
        """
        Handling address phase
        """
        clkedge = RisingEdge(self.clock)

        while True:

            await clkedge
            await ReadWrite()

            # Always assert hready for now
            self.hready.value = 1
            # Processing Data Phase
            if self.req and self.hready.value:
                if self.req_write == 1: # write request
                    self.hresp.value = self._write(self.req_addr, self.hwdata.value.to_unsigned(), self.req_size);
                if self.req_write == 0: # read request
                    self.hrdata.value, self.hresp.value = self._read(self.req_addr, self.req_size);
                # clear previously logged addr phase
                self.req = False

            # Processing Address Phase
            if self.htrans.value == 2 and self.hsel.value == 1 and self.hready.value == 1:
                self.req       = True
                self.req_addr  = self.haddr.value.to_unsigned()
                self.req_write = self.hwrite.value
                self.req_size  = self.hsize.value.to_unsigned()

    def _write(self, haddr, hwdata, hsize):
        """
        Device need to override this
        """
        pass

    def _read(self, haddr, hsize):
        """
        Device need to override this
        """
        return (0, 0)
