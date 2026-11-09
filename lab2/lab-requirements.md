# Lab Requirements

## Goal

Apply HLS pragmas to tiled matrix multiplication using Vitis HLS on the VCK5000.

## Source files

- `src/host.cpp`
- `src/pl_v0.cpp`
- `src/pl_v1.cpp`
- `src/pl_v2.cpp`
- `src/pl.cpp`: the working kernel, which each version is copied into before building

## Versions

| Version | Definition |
|---|---|
| V0 | Baseline, no modifications |
| V1 | HLS pipelining and array partitioning, 256 MACs |
| V2 | Based on V1, with 512-bit B/C ports, packing/unpacking 16 INTs per 512-bit transfer |

The lab generally expects V1 to be faster than V0, and V2 to be faster than V1.

## Rules for measurements

- Emulation may use smaller matrices.
- The final onboard test and the resource/timing reports must use **N=512, M=256**.
- `hw_emu` is faster but does not give accurate execution time or resources.
- Final timing, resource, and execution measurements must come from actual hardware.

## Submission checklist

- [ ] `host.cpp`
- [ ] `pl_v0.cpp`
- [ ] `pl_v1.cpp`
- [ ] `pl_v2.cpp`
- [ ] `host.exe`
- [ ] Hardware xclbin (`pl.xclbin` in the requirements; the build produces `pl_hw.xclbin`)
- [ ] `report.pdf`
- [ ] Resource and timing reports
- [ ] Table with Time, Throughput, resources, frequency, and WNS

Check the PDF for the exact xclbin name it wants, and whether it wants one per version.

## Where things are in the Lab 2 PDF

The PDF is not in this folder. These line numbers are from the handoff.

| Lines | Topic |
|---|---|
| 8–10 | Learn/use `tmux` |
| 13–20 | Vitis HLS pragma lab, V0/V1/V2 |
| 21–22 | Copy each version to `pl.cpp` |
| 27–33 | Save source and binaries |
| 34–40 | Resource/timing report paths |
| 42–50 | hw_emu is faster; hardware required for execution time/resources |
| 52–61 | Emulation can use smaller matrices; final hardware uses N=512, M=256 |
| 80–87 | hw_emu commands |
| 92–99 | hw_emu run |
| 107–110 | V0 |
| 116–122 | V1: 256 MACs, pipeline/array partition |
| 123–126 | V2: B/C port changed to 512-bit |
| 129–160 | Submission/report requirements |
| 168–172 | V2 Q&A |
| 175–186 | 512-bit access example |
| 190–199 | tmux commands |
