# CPU Microarchitecture

## 5 Stage Pipeline

The TinyLinuxRV CPU implements the classic 5 stage pipeline with IF, ID, EX, MEM, and WB stage.

----------------------------------------

### IF stage

![pipeline control](assets/cpu/IF.drawio.svg)

IF stage is responsible for fetching the instruction. It contains the following components:
- PC
- Skid buffer
- Instruction Memory Control logic

#### PC

- PC stands for program counter, it's the address for the next instruction to be executed.
- In usual case, PC advance to next instruction which is `PC + 4`. However, a jump, taken branch, or exception/interrupt might alter PC to another target address.
- The PC register will only be updated when IF stage is NOT stalled.
- A PC skid buffer is added to handled the case when jump/taken branch/exception/interrupt is updating PC while stall is requested by IF stage itself.

#### Skid buffer

The skid buffer is used to temporarily hold the previous instruction when the pipeline is stalled.
- When pipeline is running, it will forward the received instruction to the ID stage.
- When pipeline is stalled, it will hold the instruction it received and forward to ID stage when pipeline is running again.
- Skid buffer will only hold ONE instruction as the design make sure only 1 pending instruction need to be stored.

Flush should not affect skid buffer. As when flush is requested, the skid buffer must be empty. (Put an assertion)

#### Instruction Memory Control Logic

Assuming the memory read latency is 1 (This is the normal case for BRAM in FPGA) and without any pipeline flushing:

- The IF stage will always read the instruction memory with address being PC.
- The read data (instruction) should be available in the next cycle and be forwarded to ID stage directly without going through the pipeline stages by the skid buffer.

#### Pipeline Stall

There are 3 possible cases:

- Case 1: Stall requested from IF stage
  - If the read data is NOT available in next cycle, IF stage will stall itself and set `ID_valid` to 0
  - A **stall requested flag** should be implemented to tell this is the NOT the first cycle of a new memory access. (Needed by flush logic)
- Case 2: Stall requested from downstream stage.
  - **If downstream stage is requesting stall, IF stage should NOT assert the read request again once the current request has been accepted.**
  - A **request accepted flag** should be implemented to indicate that the current read request has been accepted. If `flag & !ID_ready` then it should not assert the read request
  - When the instruction is available from the memory, if stall is no longer requested, skid buffer will forward it to ID stage. Otherwise skid buffer will store the instruction.
- Case 3: Stall requested from both IF and downstream stage.
  - This is first handled with case 2 situation, IF should wait till the stall request from downstream is resolved. If by this time the read data is available, then proceed with next instruction. Otherwise, continue as case 1.

TODO: Confirm ABH wait state

#### Pipeline Flush

Flushing IF stage should in general invalidate the instruction received.

There are 2 possible cases for pipeline flush in IF stage.

- Case 1: Flush without IF requesting stall. (stall requested flag = 0)
  - This is the very first cycle we assert a new read request, just cancel the request.
- Case 2: Flush while IF is stalled. (stall requested flag = 1)
  - In this case, the read request has already been sent so we can NOT cancel it. We have to wait till this request has been completed.
  - A **flush request flag** should be implemented to indicate that the instruction has been flushed.

----------------------------------------

### ID stage

![pipeline control](assets/cpu/ID.drawio.svg)

ID stage is contains the following components:
- Instruction Decoder
- Register file
- Data forwarding Mux

#### Instruction Decoder

- The instruction decoder is responsible for decoding the instruction into different field and control signals to control the subsequence pipeline

#### Register File

- The register file hold 32 XLEN-bit registers.
- The decoder provides the the rs1/rs2 ID and rs1/rs2 data are read from register file in SAME cycle in ID stage
- The write data coming from WB stage is written into register file.
- The write data coming from WB stage is also forwarded into rs1/rs2 data if they match with rd id to resolve data dependence issue
- x0 is always read 0 and write will be ignored.

#### Data forwarding Mux

With 5 stage pipeline there are data hazard where junior instruction could depends on senior instruction.

An instruction in ID stage could depends on the instruction in EX stage:

```asm
addi s0, t0, 1
add  s1, s0, s0
```

Or in MEM stage:

```asm
addi s0, t0, 1
ori  s2, t0, 1
add  s1, s0, s0
```

Or even both:

```asm
addi s0, t0, 1
ori  s1, t0, 1
add  s2, s0, s1
```

In order to resolve this issue, we can forward the result from EX and MEM stage into ID stage.

Once the instruction is decoded, the rs1 and rs2 id will be compared with the rd id in EX and MEM stage, if they matched and
the instruction is a valid instruction that will update rd, then the future rd data will be forwarded to ID stage.
If both EX amd MEM stage match, the data should be forwarded from EX stage

#### Pipeline Stall

In ID stage, there is another data dependent issue that will cause ID pipeline stall.

Basically when the instruction in ID stage depends on a load instruction in EX stage. For example:

```asm
lw a0, 0(t0)
addi a1, a0, 1
```

In this case, the data will not be available in EX stage so we have to wait till the data is available in MEM stage before we can proceed.
So we will need to stall the pipeline till the data is available in MEM stage.

If ID is requesting to stall the pipeline, it will de-assert `ID_ready` and set `EX_valid` to 0.

#### Pipeline Flush

- If ID is to be flushed, it will set `EX_valid` to 0.
- If ID is stalled due to load dependence, flush request will invalidate the current instruction so the stall will no longer be requested
- Register write back still happens as that is associated with the instruction in WB stage.

----------------------------------------

### EX stage

![pipeline control](assets/cpu/EX.drawio.svg)

EX stage is contains the following components:
- ALU (Arithmetic and Logic Unit)
- Multiplier/Divider
- AGU (Address generation Unit)
- BU (Branch Unit)
- Memory Control

#### ALU

ALU is the responsible for performing arithmetic and logic operation. It takes 2 operands and control signal indicating which operation to be performed.

Operand 1 can be one of the following: `PC`, `RS1`, `0`
Operand 2 can be one of the following: `RS2`, `immediate value`

#### Multiplier/Divider

Responsible for RV64M extension.

#### AGU

AGU is responsible for generating the address for memory access as well as jump and branch instruction.
The load/store and JALR share almost the same logic for generate target address (with slight different)
The JAL use a slightly different logic. But essentially they are both a XLEN-bit adder.

We could use ALU to calculate the address but to achieve better timing, we choose to introduce a dedicated AGU to generate the address.

#### BU

Branch Unit is responsible for checking the condition for branch instruction.

#### Memory Control

The memory control is responsible for generating control signal for access data memory.

TODO: The AMO instruction need to read and write memory in one instruction. Will probably need a small state machine to achieve this.

#### Pipeline Stall

- For divide instruction, it can't be finished within one cycle so EX will need to be stalled.
- For a load/store instruction, if the memory does not accept the request in this cycle, EX stage will stall the pipeline till the memory is ready to accept the request.
- Similar to IF stage, a stall requested flag will be implemented to help flush logic.
- If this is requested, it will assert `EX_ready` to be 0 and set `MEM_valid` to 0

TODO: Confirm ABH wait state

#### Pipeline Flush

Flushing EX stage should in general invalidate the instruction in this stage.

Unlike IF stage, flushing EX stage must be requested before the memory request becomes irrevocable. This is usually the very first cycle of the EX stage.
This is because once we send the request to memory, we can NOT revoke this request. This requirement is in general met. This will be covered by
exception/interrupt session. (To make sure this work, the pending exception should flush all the younger pipeline stage. Interrupt is a bit tricky because  it need to know the next PC)

----------------------------------------

### MEM stage

![pipeline control](assets/cpu/MEM.drawio.svg)

Memory stage is in general vary simple, it takes the read data coming back from the memory and post process it based on the read instruction type.

There is also a mux to select the data from ALU (pipeline stage) and the memory read data. If the instruction is a memory read, then use the read data. Otherwise use the ALU output.

#### Pipeline Stall

MEM stage will not be stalled.

#### Pipeline Flush

If MEM stage is flushed, the instruction will be invalidated.

----------------------------------------

### WB stage

![pipeline control](assets/cpu/WB.drawio.svg)

WB stages contains the following components:
- Register file write back
- CSR
- Exception/Interrupt (Trap) handling logic

#### Register file write back

WB stage will write the data back to register file is applicable.


#### CSR

TODO

#### Trap

TODO

----------------------------------------

### Pipeline Control

To simplify the design, instead of using a centralized pipeline control, we use valid/ready control for pipeline

![pipeline control](assets/cpu/pipeline_control.drawio.svg)


The next pipeline stage only accept data from previous stage when both valid and ready are asserted.
```c
valid_B = valid_A & ready_B
```

The pipeline is ready to accept data from previous stage if current stage is empty or next stage is also ready to accept data
```
ready_A = !valid_A | ready_B
```
