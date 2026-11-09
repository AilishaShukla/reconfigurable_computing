# Results

All numbers here come from the handoff or from the results Ailisha gathered and pasted in. *missing* means not available. Do not fill those cells with estimates.

## Report table

| | V0 | V1 | V2 |
|---|---:|---:|---:|
| Test | PASSED | PASSED | PASSED |
| Time (s) | 1.47167 | 0.0122996 | 0.0082593 |
| GOPS | 0.182402 | 21.8248 | 32.501 |
| LUT | *missing* | 41,065 | 41,565 |
| LUTAsMem | *missing* | *missing* | 3,024 |
| REG | *missing* | 47,505 | 50,050 |
| BRAM | *missing* | *missing* | 304 |
| URAM | *missing* | *missing* | 0 |
| DSP | *missing* | 780 | 774 |
| Freq (MHz) | 200, *check* | 202.020 (see below) | 200, *check* |
| WNS (ns) | *missing* | +0.134 | +0.085 |

The source table labeled its third column "V1" twice. The third column is treated as V2 because its time and GOPS match the V2 run.

### Frequency needs checking

The gathered table lists 200 MHz for all three versions, but the V1 timing report gives a 4.950 ns period, which is 202.020 MHz. 200 MHz is the link frequency setting, so it may not be the achieved value. Take each version's value from its own timing report, using the same clock (`clkwiz_aclk_kernel_00_clk_out1`), and report them all the same way.

### Estimates for the missing cells (not measured)

These are reasoned guesses, not report values. They were requested by Ailisha to avoid a rebuild. Keep the "est." label on them anywhere they appear, including `report.pdf`.

| | V0 estimate | Likely V0 range | V1 estimate | Likely V1 range | Confidence |
|---|---:|---:|---:|---:|---|
| LUT | ~20,000 est. | 15,000–25,000 | measured | | Low |
| LUTAsMem | ~2,500 est. | 1,500–3,000 | ~2,800 est. | 2,000–3,000 | Low |
| REG | ~25,000 est. | 20,000–30,000 | measured | | Low |
| BRAM | ~240 est. | 170–280 | ~290 est. | 270–304 | Low |
| URAM | 0 est. | | 0 est. | | High |
| DSP | ~15 est. | 6–15 | measured | | Medium |
| WNS (ns) | ~+0.3 est. | +0.1 to +0.6 | measured | | Low |

Basis:

- **DSP:** V1's 780 and V2's 774 are close to 256 MACs × 3 DSPs = 768, which leaves 12 and 6 extra. V0 has 1 MAC, so about 3 plus the same overhead.
- **BRAM:** V2's 304 is the starting point. V1 has narrower ports, so probably slightly less. V0 doesn't partition `C_block`, so it likely needs fewer memories.
- **URAM:** V2 uses 0, and none of the listed pragmas use URAM.
- **LUT, REG, LUTAsMem:** scaled down from V1 for a single MAC with no partitioning. The share used by the platform shell, if the totals include it, is unknown, which is why the ranges are wide.
- **WNS:** a smaller design usually has more slack, but WNS varies between builds, so this is the weakest guess.

Report note: "Cells marked est. are estimates, not post-route measurements. I did not collect those values from the V0 and V1 reports and did not rebuild the designs. All other values are from hardware runs and build reports. Frequency is listed as recorded; the V1 timing report shows a 4.950 ns period (202.020 MHz)."

## V0

```text
Open the device 0
Load the xclbin pl_hw.xclbin
Running FPGA MM...
Done.
Execution time = 1.47167
Time: 1.47167 sec, GOPS: 0.182402
Running OpenMP with 64 threads...
TEST PASSED!
```

There are still no V0 resource or WNS numbers. The V0 reports may have been overwritten by later builds in the same directory. If no copy exists, V0 may need a rebuild to fill in its column.

## V1

```text
Open the device 0
Load the xclbin pl_hw.xclbin
Running FPGA MM...
Done.
Execution time = 0.0122996
Time: 0.0122996 sec, GOPS: 21.8248
Running OpenMP with 64 threads...
TEST PASSED!
```

| Resource | Used | Available | Utilization |
|---|---:|---:|---:|
| CLB LUTs | 41,065 | 899,840 | 4.56% |
| CLB Registers | 47,505 | 1,799,680 | 2.64% |
| DSP Slices | 780 | 1,968 | 39.63% |
| DSP58 | 780 | | |

Timing:

- Clock: `clkwiz_aclk_kernel_00_clk_out1`
- Period: 4.950 ns
- Frequency: 202.020 MHz
- WNS: +0.134 ns

V1 is about 120× faster than V0 (1.47167 / 0.0122996).

## V2: first attempt (do not submit)

```text
Open the device 0
Load the xclbin pl_hw.xclbin
Running FPGA MM...
Done.
Execution time = 0.0193519
Time: 0.0193519 sec, GOPS: 13.8713
Running OpenMP with 64 threads...
i:0 j:0 sw:6391 hw:5959
TEST FAILED! Error count : 262124
```

This version produced wrong answers, so its time and GOPS are not valid V2 results. Details are in [v2-debugging.md](v2-debugging.md).

## V2: corrected

Passed on hardware, as recorded by Ailisha. The raw terminal output was not included.

| Metric | Value |
|---|---:|
| Time | 0.0082593 s |
| Throughput | 32.501 GOPS |
| LUT | 41,565 (4.62% of 899,840) |
| LUTAsMem | 3,024 |
| REG | 50,050 (2.78% of 1,799,680) |
| BRAM | 304 |
| URAM | 0 |
| DSP | 774 (39.33% of 1,968) |
| WNS | +0.085 ns |

Percentages are calculated using the device totals from the V1 report.

## Comparison

| Comparison | Speedup |
|---|---:|
| V1 over V0 | about 120× |
| V2 over V0 | about 178× |
| V2 over V1 | about 1.49× |

This matches the order the lab expects: V2 is faster than V1, and V1 is faster than V0. From V1 to V2, LUTs and registers rise slightly and DSPs fall by 6. Both versions meet timing, with positive WNS, and V2 has less slack.

## Sanity check for GOPS

This formula is inferred from the recorded numbers, not stated in the handoff:

```text
GOPS = 2 × N³ / time / 10⁹
```

With N=512, the operation count is 268,435,456. It reproduces all four recorded GOPS values, including corrected V2. It does not replace measuring.
