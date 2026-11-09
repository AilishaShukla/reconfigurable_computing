# Local Kernel Check

Test a kernel on the Mac before uploading it to the server.

```bash
cd ~/Desktop/2911X/local_test
./check.sh ../lab3/Lab3/src/pl_v3.cpp
./check.sh ../lab3/Lab3/src/pl_v4.cpp
```

Add a size at the end to run smaller, for example `./check.sh <file> 256`. It must be a multiple of 256.

## What it checks

1. **Compile:** finds C++ errors like a missing `;`.
2. **Right answer:** runs the kernel on the CPU and compares it to a plain matrix multiply, using the same random data as `host.cpp`.
3. **Bit slices:** stops if `x(hi, lo)` goes outside the 512 bits.
4. **Streams (V4):** stops if a stream is read when empty (this would hang on the FPGA) or has data left at the end.

## What it does NOT check

- HLS pragmas. Typos in `#pragma HLS` are ignored here.
- Timing, resources, or whether `II=1` worked.
- The CPU time is not an FPGA number. Never put it in the report.

For those, run only the HLS step on the VM first (a few minutes, not 30+):

```bash
make hls
```

Then check `_x.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/pl.log` for errors. If it is clean, run `make all`.

## Files

- `include/ap_int.h`, `include/hls_stream.h`: small stand-ins for the Xilinx headers. Not the real ones.
- `tb_mm.cpp`: the test bench (a copy of `host.cpp` without the FPGA parts).
- `check.sh`: runs everything.
