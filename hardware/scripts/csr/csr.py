"""
Copyright (c) 2026 Heqing Huang
SPDX-License-Identifier: MIT
"""

import yaml
import subprocess

repo_path = subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()

CSR_YAML = f"{repo_path}/hardware/scripts/csr/csr.yaml"
CSR_RTL  = f"{repo_path}/hardware/rtl/include/cpu/csr.svh"

def generate_rtl_header():
    rtl = \
"""
/*
 * Copyright (c) 2026 Heqing Huang
 * SPDX-License-Identifier: MIT
 */

`ifndef _CSR_H_
`define _CSR_H_

// --------------------------
// CSR address
// --------------------------

enum logic [11:0] {
    CSR_SSTATUS       = 12'h100,
    CSR_SIE           = 12'h104,
    CSR_STVEC         = 12'h105,
    CSR_SCOUNTEREN    = 12'h106,
    CSR_SSCRATCH      = 12'h140,
    CSR_SEPC          = 12'h141,
    CSR_SCAUSE        = 12'h142,
    CSR_STVAL         = 12'h143,
    CSR_SIP           = 12'h144,
    CSR_SATP          = 12'h180,

    CSR_MSTATUS       = 12'h300,
    CSR_MISA          = 12'h301,
    CSR_MEDELEG       = 12'h302,
    CSR_MIDELEG       = 12'h303,
    CSR_MIE           = 12'h304,
    CSR_MTVEC         = 12'h305,
    CSR_MCOUNTEREN    = 12'h306,
    CSR_MSCRATCH      = 12'h340,
    CSR_MEPC          = 12'h341,
    CSR_MCAUSE        = 12'h342,
    CSR_MTVAL         = 12'h343,
    CSR_MIP           = 12'h344,
    CSR_MCOUNTINHIBIT = 12'h320,
    CSR_MCYCLE        = 12'hB00,
    CSR_MINSTRET      = 12'hB02,
    CSR_MVENDORID     = 12'hF11,
    CSR_MARCHID       = 12'hF12,
    CSR_MIMPID        = 12'hF13,
    CSR_MHARTID       = 12'hF14,

    CSR_CYCLE         = 12'hC00,
    CSR_TIME          = 12'hC01,
    CSR_INSTRET       = 12'hC02
} csr_addr_enum;

"""
    return rtl


def generate_rtl_end():
    rtl = \
"""
`endif
"""
    return rtl


def gen_struct(csr_name, csr_info):
    """Generate the struct definition for a CSR register
    """
    csr_fields = csr_info["fields"]

    rtl = "\n"
    rtl +=  "// --------------------------\n"
    rtl += f"// {csr_name}\n"
    rtl +=  "// --------------------------\n"
    rtl +=  "typedef struct packed {\n"
    for field in csr_fields:
        if "bit" in csr_fields[field]:
            rtl +=  f"    logic {field};\n"
        else:
            range = list(csr_fields[field]["bits"])
            width = range[0] - range[1] + 1
            rtl +=  f"    logic [{width-1}:0] {field};\n"
    rtl += f"}} {csr_name}_t;\n"
    return rtl

def gen_write(csr_name, csr_info):
    """Generate the CSR write function
    """
    rtl_field = ""
    # read write register
    # Create write logic to each field
    rtl_field +=  "    // CPU write field\n"
    csr_fields = csr_info["fields"]
    for field in csr_fields:
        access = csr_fields[field]["access"]
        if "bit" in csr_fields[field]:
            range = csr_fields[field]["bit"]
        else:
            range = list(csr_fields[field]["bits"])
            range = f"{range[0]}:{range[1]}"
        # FIXME: WARL
        if access == "rw" or access == "warl":
            rtl_field += f"    data.{field} = wdata[{range}];\n"
    # Shadow writes preserve fields outside the writable view.
    if "shadow" in csr_info:
        csr_name_final = csr_info["shadow"]
        func_input = f"    input {csr_name_final}_t csr,\n"
        func_input += "    input [63:0] wdata"
        initial_value = "csr"
    else:
        csr_name_final = csr_name
        func_input = "    input [63:0] wdata"
        initial_value = "0"
    # Create write function
    rtl = \
f"""
function automatic {csr_name_final}_t {csr_name}_write(
{func_input}
);
    {csr_name_final}_t data = {initial_value};
{rtl_field}
    return data;
endfunction
"""
    access = csr_info["access"]
    if access == "rw":
        return rtl
    else:
        return ""

def gen_read(csr_name, csr_info):
    """Generate the CSR read function
    """
    csr_fields = csr_info["fields"]

    # Create read logic to each field
    rtl_field = ""

    for field in csr_fields:
        if "bit" in csr_fields[field]:
            range = csr_fields[field]["bit"]
        else:
            range = list(csr_fields[field]["bits"])
            range = f"{range[0]}:{range[1]}"
        # all the fields are always be able to read
        if "fixed" in csr_fields[field]:
            value = csr_fields[field]["fixed"]
            rtl_field += f"    data[{range}] = {value}; // Fixed: {csr_name}.{field}\n"
        elif "source" in csr_fields[field]:
            source = csr_fields[field]["source"]
            rtl_field += f"    data[{range}] = csr.{source};\n"
        else:
            rtl_field += f"    data[{range}] = csr.{field};\n"

    # Check shadow
    if "shadow" in csr_info:
        csr_name_final = csr_info["shadow"]
    else:
        csr_name_final = csr_name

    # Create read function
    rtl = \
f"""
function automatic logic [63:0] {csr_name}_read(
    input {csr_name_final}_t csr,
);
    logic [63:0] data = 0;
{rtl_field}
    return data;
endfunction
"""
    return rtl

def generate_rtl():
    with open(CSR_YAML, "r") as ym, open(CSR_RTL, "w") as rtl:
        rtl.write(generate_rtl_header())
        csrs = yaml.safe_load(ym)
        for csr_name in csrs:
            rtl.write(gen_struct(csr_name, csrs[csr_name]))
            rtl.write(gen_write(csr_name, csrs[csr_name]))
            rtl.write(gen_read(csr_name, csrs[csr_name]))
        rtl.write(generate_rtl_end())

generate_rtl()
