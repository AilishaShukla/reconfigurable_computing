# ENGN 2911X Lab Guide

A short guide to the course labs and how to do them.

## About the course

ENGN 2911X at Brown is about building hardware accelerators on FPGAs with high-level synthesis (HLS).

- **Instructor:** Peipei Zhou (peipei_zhou@brown.edu)
- **TA:** Zhuoping Yang (zhuoping_yang@brown.edu)
- **Board:** AMD/Xilinx VCK5000
- **Tools:** Vitis 2022.2, Vitis HLS, XRT
- **Platform:** `xilinx_vck5000_gen4x8_qdma_2_202220_1`

Every lab has the same two parts:

| File | Runs on | What it does |
|---|---|---|
| `src/host.cpp` | CPU (the VCK5000 host) | Sets up data, starts the kernel, times it, checks the answer |
| `src/pl.cpp` | FPGA | The kernel. HLS turns this C++ into hardware |

You write C++ in `pl.cpp`, add `#pragma HLS ...` lines to tell the tool how to build the hardware, then measure how fast it runs and how much of the chip it uses.

## The labs

| Lab | Topic | What you change |
|---|---|---|
| Lab 1 | Vector add (`c = a + b`), learn the flow | Add a timer in `host.cpp` |
| Lab 2 | Tiled matrix multiply, versions V0 to V2 | Add HLS pragmas, then widen ports to 512 bits |
| Lab 3 | Faster matrix multiply, versions V3 and V4 | Transposed A plus wide ports (V3), then dataflow with streams (V4) |

Each lab builds on the one before it. V3 starts from V2, and V4 starts from V3.

---

## Setup (all labs)

### 1. Connect

```text
Mac  --(Brown VPN)-->  arc-server  -->  vm1 (build here)  -->  vck5000 (run here)
```

1. Turn on the Brown VPN (F5 BIG-IP Edge Client).
2. From the Mac: `ssh vm1`
3. Build on the VM. Run on the VCK (`ssh vck5000` from the VM).

Mac `~/.ssh/config`:

```sshconfig
Host arc-server
    HostName 10.9.84.103
    Port 2222
    User ailisha_shukla

Host vm1
    HostName 172.16.100.33
    User ailisha_shukla
    ProxyCommand ssh -W %h:%p arc-server
```

Always check your prompt before running a command:

- VM prompt: `ailisha_shukla@localhost`
- VCK prompt: `ailisha_shukla@arclab0`

### 2. Use tmux

Hardware builds take 30+ minutes. If SSH drops, the build dies unless it is in tmux.

```bash
tmux new -s lab3        # start a session
tmux attach -t lab3     # get back in after a disconnect
tmux ls                 # list sessions
```

Detach with `Ctrl+b` then `d`.

### 3. Load the tools (on the VM)

```bash
cd ~/lab3_src           # your lab folder
source env.sh
```

`env.sh` loads XRT and Vitis 2022.2.

### 4. Folder layout

Every lab uses this layout. Copy the version you are building into `pl.cpp`.

```text
.
├── Makefile
├── env.sh
└── src
    ├── host.cpp
    └── pl.cpp
```

---

## Check the kernel on the Mac first

A full build takes 30+ minutes. Before uploading, check the kernel on the Mac in about 1 second:

```bash
cd ~/Desktop/2911X/local_test
./check.sh ../lab3/Lab3/src/pl_v3.cpp
./check.sh ../lab3/Lab3/src/pl_v4.cpp
```

Add a size at the end to run smaller, for example `./check.sh <file> 256`. It must be a multiple of 256.

It compiles the kernel, runs it on the CPU with the same random data as `host.cpp`, and compares the answer to a plain matrix multiply. You see `ALL LOCAL CHECKS PASSED` or the exact error.

What it catches:

| Problem | Example |
|---|---|
| C++ errors | A missing `;` |
| Wrong answers | The Lab 2 bug, `Bj[j]` instead of `Bj[jj*16 + j]` |
| Bad bit slices | `x(hi, lo)` going past bit 511 |
| Stream count mismatch (V4) | Reading an empty stream (hangs on the FPGA) or data left over at the end |

What it does **not** check:

- `#pragma HLS` lines, timing, resources, or whether `II=1` worked.
- The CPU time it prints is not an FPGA number. Never put it in the report.
- It uses small stand-in headers in `local_test/include/`, not the real Xilinx ones.

For pragmas, run only the HLS step on the VM first. It takes a few minutes, not 30+:

```bash
make hls
```

Check `_x.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/pl.log` for errors. If it is clean, run `make all`.

So the order is:

1. `./check.sh` on the Mac until it passes.
2. Upload, then `make hls` on the VM and check the log.
3. `make all` inside tmux.

The server's own tests can be added to `check.sh` later so they also run on the Mac.

---

## The build and run flow

### Build on the VM (inside tmux)

```bash
cp src/pl_v3.cpp src/pl.cpp
make all
```

`make all` does three steps:

1. **HLS** turns `pl.cpp` into hardware (`_x.hw.../pl.xo`). A few minutes.
2. **Link** places and routes it on the chip (`pl_hw.xsa`). This is the long part.
3. **Package** makes `pl_hw.xclbin`, and the host code compiles into `host.exe`.

Near the end you will see:

```text
Finished 5th of 6 tasks (FPGA routing).
Finished 6th of 6 tasks (FPGA bitstream generation).
```

After that it can go quiet for a while. **Do not press Ctrl+C.** Wait for `pl_hw.xclbin compilation done!` and the prompt.

### Copy and run on the VCK

```bash
# on the VM
scp host.exe pl_hw.xclbin vck5000:~/

# on the VCK
ssh vck5000
source /opt/xilinx/xrt/setup.sh
./host.exe pl_hw.xclbin
```

If you skip `source /opt/xilinx/xrt/setup.sh` you get `libxrt_coreutil.so.2: cannot open shared object file`.

A good run looks like:

```text
Running FPGA MM...
Execution time = 0.0082593
Time: 0.0082593 sec, GOPS: 32.501
TEST PASSED!
```

A build that finishes is not proof the kernel is right. You need `TEST PASSED!`.

### Hardware emulation (for debugging)

Use this to check if the kernel gives the right answer without waiting for a full build. Do not use its times or resources in your report.

```bash
make hw_emu
make all TARGET=hw_emu
export XCL_EMULATION_MODE=hw_emu
./host.exe pl_hw_emu.xclbin
```

You can shrink the matrix (`SIZE` in `host.cpp`) to make emulation faster. Keep it a multiple of 256 (the tile size `M`), and put it back to 512 for the real run.

### Save everything after each version

A new build overwrites the old reports and `pl_hw.xclbin`. Save them right away:

```bash
mkdir -p saved/v3
cp src/pl.cpp saved/v3/pl_v3.cpp
cp host.exe pl_hw.xclbin saved/v3/
cp build_dir.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/reports/link/imp/impl_1_full_util_routed.rpt saved/v3/
cp build_dir.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/reports/link/imp/impl_1_slr_util_routed.rpt saved/v3/
cp build_dir.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/reports/link/imp/impl_1_xilinx_vck5000_gen4x8_qdma_2_202220_1_bb_locked_timing_summary_routed.rpt saved/v3/
```

Also copy your edited `pl.cpp` back to `pl_vX.cpp` before you `cp` the next version over it. In Lab 2 the fixed V2 code almost got lost this way.

---

## Filling in the report table

Every lab asks for a table like this, one column per version:

| | V3 | V4 |
|---|---|---|
| Time (s) | | |
| GOPS | | |
| LUT | | |
| LUTAsMem | | |
| REG | | |
| BRAM | | |
| URAM | | |
| DSP | | |
| Freq (MHz) | | |
| WNS (ns) | | |

Where each number comes from:

| Value | Where to find it |
|---|---|
| Time, GOPS | `host.exe` output on the VCK |
| LUT, LUTAsMem, REG, BRAM, URAM, DSP | `impl_1_full_util_routed.rpt` and `impl_1_slr_util_routed.rpt`. Use the **Used Resources** row |
| WNS | Timing report, "Design Timing Summary", first column |
| Freq | Timing report, clock `clkwiz_aclk_kernel_00_clk_out1`. Freq (MHz) = 1000 / period (ns) |

All reports are in `build_dir.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/reports/link/imp/`.

- **GOPS** = 2 x N³ / time / 10⁹. With N = 512 that is 268,435,456 operations.
- **WNS** must be positive. Negative means the design misses timing.
- The final numbers must come from a real hardware run with N = 512, M = 256. Not from emulation.

### HLS report vs. implementation report

- `_x/reports/pl/hls_reports/pl_csynth.rpt` is an early **estimate** from HLS.
- `_x/logs/pl/pl_vitis_hls.log` tells you if your `pipeline II=1` worked. Look for `All loop constraints were satisfied.` If not, it tells you why.
- The `reports/link/imp/` files are the **final** numbers after place and route. Use these in the report.

---

## Lab 1: Vector add

**Goal:** learn the build and run flow on a simple kernel.

The kernel (`lab1/src/pl.cpp`) adds two arrays of 32,768 ints. The host runs it 10 times.

**What to do:** fill in the two `Fill this part!!` spots in `host.cpp` with a timer. Use the same style as later labs:

```cpp
// before the loop
auto start = std::chrono::high_resolution_clock::now();

// after the loop
auto end = std::chrono::high_resolution_clock::now();
double total = std::chrono::duration<double>(end - start).count();
std::cout << "Average time = " << total / 10 << " s" << std::endl;
```

Add `#include <chrono>` at the top. Then build, run, and check for `TEST PASSED`.

---

## Lab 2: Matrix multiply with HLS pragmas

**Goal:** speed up a tiled 512 x 512 matrix multiply with pragmas.

The matrix is split into 256 x 256 tiles (`M = 256`). For each tile of C, the kernel keeps `C_block[256][256]` on chip, reads one row of B into `Bj`, and does 256 multiply-adds at a time.

| Version | What changes |
|---|---|
| V0 | Baseline. No pragmas |
| V1 | Pipeline the loops and partition the arrays so 256 MACs run in parallel |
| V2 | V1, plus B and C ports widened to 512 bits (16 ints per read/write) |

### V1 fill-ins

```cpp
#pragma HLS array_partition variable=Bj complete   // after int Bj[M];
#pragma HLS pipeline II=1                          // at the top of i_loop
```

Partitioning lets all 256 values of `Bj` be read in the same clock cycle. Pipelining with II=1 starts a new `i` every cycle, and the inner `j` loop unrolls into 256 parallel MACs.

### V2 key idea: packing 16 ints in one 512-bit word

```cpp
typedef ap_int<512> block_t;
const int DTYPE_PER_PORT = 16;   // 512 / 32
```

- To read: take one `block_t` and slice out 16 ints with `B_temp(32*j + 31, 32*j)`.
- To write: put 16 ints into a `block_t` and write it once.
- Word index = scalar index / 16.

### The V2 bug to avoid

The first V2 try built fine but failed with `TEST FAILED! Error count : 262124`. Two mistakes:

1. The B address left out `jb` and used `M` where it should use `N`.
2. `Bj[j]` only filled the first 16 slots instead of all 256.

The fix:

```cpp
B_temp = B[((kb * M + k) * N + jb * M) / DTYPE_PER_PORT + jj];
for (int j = 0; j < DTYPE_PER_PORT; j++) {
    Bj[jj * DTYPE_PER_PORT + j] = B_temp(j*32 + 31, j*32);
}
```

**Tip:** always work out the wide index from the V0 scalar index. Write the V0 index, then divide by 16.

### Lab 2 results

| | V0 | V1 | V2 |
|---|---:|---:|---:|
| Time (s) | 1.47167 | 0.0122996 | 0.0082593 |
| GOPS | 0.18 | 21.82 | 32.50 |

V1 is about 120x faster than V0. V2 is about 1.5x faster than V1. More notes are in the `lab2/` folder.

---

## Lab 3: Transpose and dataflow

**Goal:** make V3 and V4 faster than V2.

Files: `lab3/Lab3/src/pl_v3.cpp`, `pl_v4.cpp`, and `host.cpp`. Only change the kernel files. The host is done for you.

### What is new in the host

The host now gives the kernel **A transposed** (`At[k*SIZE + i]`). This means a column of A is now stored as a row in memory. So the kernel can read 16 A values in one 512-bit read, just like B. All three ports (A, B, C) are now `block_t`, each on its own bundle (`gmem0`, `gmem1`, `gmem2`).

### V3: transposed A and wide ports

Fill in each `// Fill This Part !!!`:

| Spot | What to add |
|---|---|
| After `DTYPE C_block[M][M];` | `#pragma HLS array_partition variable=C_block complete dim=2` |
| Top of `init_i_loop` | `#pragma HLS pipeline II=1` |
| After `DTYPE Bj[M];` | `#pragma HLS array_partition variable=Bj complete` |
| Inside `readB_j_loop` inner loop | `Bj[jj*DTYPE_PER_PORT + j] = B_temp(DTYPE_WIDTH_b*(j+1)-1, DTYPE_WIDTH_b*j);` |
| Top of `ii_loop` | `#pragma HLS pipeline II=1` |
| Inside `in_i_loop` | `A_line[ii*DTYPE_PER_PORT + i] = Ai_block(DTYPE_WIDTH_b*(i+1)-1, DTYPE_WIDTH_b*i);` |
| Top of `i_loop` | `#pragma HLS pipeline II=1` |

Watch the `Bj` and `A_line` indexes. Use `jj*16 + j`, not just `j`. That was the Lab 2 bug.

Why it is faster: in V2, `i_loop` read one A value from DDR every cycle. Now all 256 A values for this `k` are loaded first into `A_line` with 16 wide reads, so the compute loop never waits on memory.

### V4: dataflow with streams

V4 splits the kernel into 5 functions that run at the same time and pass data through FIFOs (`hls::stream`). This overlaps memory reads, compute, and writes.

```text
readA --> changeARate --> \
                           comp --> writeC
readB -------------------> /
```

Everything is written for you except `comp`. It must read the streams in the **exact same order** the other functions write them. Follow V3's loop structure:

```cpp
void comp(hls::stream<DTYPE> &AStream, hls::stream<block_t> &BStream,
          hls::stream<block_t> &CStream, int N) {
    DTYPE C_block[M][M];
    #pragma HLS array_partition variable=C_block complete dim=2

    for (int ib = 0; ib < N/M; ib++) {
        for (int jb = 0; jb < N/M; jb++) {

            // 1. zero C_block
            for (int i = 0; i < M; i++) {
                #pragma HLS pipeline II=1
                for (int j = 0; j < M; j++) {
                    C_block[i][j] = 0;
                }
            }

            for (int kb = 0; kb < N/M; kb++) {
                for (int k = 0; k < M; k++) {

                    // 2. read one row of B (16 wide words) into Bj
                    DTYPE Bj[M];
                    #pragma HLS array_partition variable=Bj complete
                    for (int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {
                        #pragma HLS pipeline II=1
                        block_t B_temp = BStream.read();
                        for (int j = 0; j < DTYPE_PER_PORT; j++) {
                            #pragma HLS unroll
                            Bj[jj*DTYPE_PER_PORT + j] =
                                B_temp(DTYPE_WIDTH_b*(j+1)-1, DTYPE_WIDTH_b*j);
                        }
                    }

                    // 3. one A value per i, 256 MACs per cycle
                    for (int i = 0; i < M; i++) {
                        #pragma HLS pipeline II=1
                        DTYPE a = AStream.read();
                        for (int j = 0; j < M; j++) {
                            #pragma HLS unroll
                            C_block[i][j] += a * Bj[j];
                        }
                    }
                }
            }

            // 4. pack C_block into wide words and send to writeC
            for (int i = 0; i < M; i++) {
                for (int jj = 0; jj < M/DTYPE_PER_PORT; jj++) {
                    #pragma HLS pipeline II=1
                    block_t C_temp;
                    for (int j = 0; j < DTYPE_PER_PORT; j++) {
                        #pragma HLS unroll
                        C_temp(DTYPE_WIDTH_b*(j+1)-1, DTYPE_WIDTH_b*j) =
                            (ap_int<DTYPE_WIDTH_b>) C_block[i][jj*DTYPE_PER_PORT + j];
                    }
                    CStream.write(C_temp);
                }
            }
        }
    }
}
```

Check the counts match:

- `readB` writes M/16 words per `k`. `comp` reads M/16 per `k`.
- `changeARate` writes M values per `k`. `comp` reads M per `k`.
- `writeC` reads M x M/16 words per tile. `comp` writes the same.

If the counts do not match, the kernel will hang (a stream waits forever) or give wrong answers. Test with `hw_emu` first.

**Optional:** if HLS warns about stream depth, add `#pragma HLS stream variable=AStream depth=...` in `pl()`. See the AMD docs linked in the PDF:

- https://docs.amd.com/r/2022.2-English/ug1399-vitis-hls/pragma-HLS-dataflow
- https://docs.amd.com/r/2022.2-English/ug1399-vitis-hls/pragma-HLS-stream

### Lab 3 submission (100 pts)

Zip this layout:

```text
.
├── report.pdf
├── src
│   ├── host.cpp
│   ├── pl_v3.cpp
│   └── pl_v4.cpp
├── v3
│   ├── host.exe
│   └── pl_hw.xclbin
└── v4
    ├── host.exe
    └── pl_hw.xclbin
```

`report.pdf` needs the table above for V3 and V4. V3 and V4 should both be faster than V2 (0.0082593 s, 32.5 GOPS), so compare against it in your write-up.

Two things in the PDF look like typos. Check with the TA:

- It says to name the zip `lab2_${YourBrownID}.zip`. It probably means `lab3_`.
- It says "three versions of HLS kernel" but only gives two (`pl_v3.cpp`, `pl_v4.cpp`).

---

## Common mistakes

1. Running `ssh vm1` while already on the VM or the VCK.
2. Forgetting `source /opt/xilinx/xrt/setup.sh` on the VCK.
3. Building outside tmux, then losing the build when SSH drops.
4. Pressing Ctrl+C because the build went quiet.
5. Overwriting your edited `pl.cpp` with `cp` before saving it.
6. Not saving reports before the next build replaces them.
7. Thinking the design works because it built. You need `TEST PASSED!`.
8. Using `hw_emu` or CPU times in the report. Only real hardware numbers count.
9. Getting wide-port indexes wrong. Start from the scalar index and divide by 16.
10. Changing the starter comments. Keep them and only change what is needed.
