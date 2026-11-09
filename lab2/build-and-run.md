# Build and Run

Server access and environment details are in [server-setup.md](server-setup.md).

## Hardware flow

### On the VM, inside tmux

Copy the version you want into `pl.cpp` and build it:

```bash
cd ~/lab2_src
source env.sh
cp src/pl_v0.cpp src/pl.cpp
make all
```

Replace `pl_v0` with the version being built. The corrected V2 edits were made directly in `src/pl.cpp`, so make sure they are saved in `src/pl_v2.cpp` before this `cp` overwrites them.

### Reading build progress

The final tasks in the log look like this:

```text
Finished 5th of 6 tasks (FPGA routing).
Finished 6th of 6 tasks (FPGA bitstream generation).
```

- After the 6th task, Vitis may still package, link, and copy files before returning to the shell, with no output for a while.
- The likely success message is something like `pl_hw.xclbin compilation done!`, followed by the shell prompt.
- Do not press Ctrl+C because it has gone quiet. Check from a second terminal instead: `ssh vm1`, then `tmux attach -t <session>`.

The corrected V2 build took 13m 03s for routing and 12m 33s for bitstream generation.

### Copy to the VCK

From the VM:

```bash
ls -lh host.exe pl_hw.xclbin
scp host.exe pl_hw.xclbin vck5000:~/
```

### Run on the VCK

```bash
ssh vck5000
source /opt/xilinx/xrt/setup.sh
./host.exe pl_hw.xclbin
```

A good run ends with `TEST PASSED!`. Record the execution time and GOPS.

If it fails, do not change the source yet. Capture the complete output, especially the first mismatch, the error count, and the time/GOPS.

## Resource and timing reports

After a hardware build, under `build_dir.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/reports/link/imp/`:

| Report | File |
|---|---|
| Resources | `impl_1_full_util_routed.rpt` |
| Timing | `impl_1_xilinx_vck5000_gen4x8_qdma_2_202220_1_bb_locked_timing_summary_routed.rpt` |

Extract these for the report table:

- CLB LUTs
- CLB Registers
- DSP Slices, and DSP58 if listed
- Frequency, from the clock `clkwiz_aclk_kernel_00_clk_out1` (this is the clock used for V1)
- WNS

**These paths are the same for every version.** A new build in the same directory likely replaces the previous version's reports and `pl_hw.xclbin`, so save copies per version before rebuilding.

## Hardware emulation (fallback)

Use this to debug functional correctness. It does not replace final hardware measurements.

```bash
cd ~/lab2_src
source env.sh
make hw_emu
make build TARGET=hw_emu
make host TARGET=hw_emu
```

Or as one command:

```bash
make all TARGET=hw_emu
```

Then run:

```bash
export XCL_EMULATION_MODE=hw_emu
./host.exe pl_hw_emu.xclbin
```

If you shrink the matrices for V2 emulation, keep N and M multiples of 16, because the V2 addressing divides them by `DTYPE_PER_PORT` (see [v2-debugging.md](v2-debugging.md)).

## Testing locally

- A normal CPU implementation on the Mac can validate matrix-multiply logic and indexing.
- It cannot reproduce VCK5000 execution time, HLS resource usage, FPGA timing, or xclbin behavior.
- `hw_emu` on the Brown VM is the more relevant fallback for testing the Vitis kernel.
