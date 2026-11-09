# Next Steps

All three versions have passed on hardware. What remains is protecting the V2 work, filling the missing numbers, and writing the report.

Give these one at a time and wait for the output after each. Run them on the VM, in `~/lab2_src`.

## 1. Save the corrected V2 source

See how `src/pl.cpp` differs from `src/pl_v2.cpp`:

```bash
diff src/pl.cpp src/pl_v2.cpp
```

- No output means they match, so skip the next command.
- If the differences are only the corrected V2 code, save it as the submission copy:

```bash
cp src/pl.cpp src/pl_v2.cpp
```

This must happen before any rebuild, because rebuilding V0 or V1 overwrites `src/pl.cpp`.

## 2. Save the V2 binaries and reports

Make a folder for them:

```bash
mkdir -p saved/v2
```

Copy the binaries and the two reports into it. Do this only if V2 was the most recent build:

```bash
cp host.exe pl_hw.xclbin build_dir.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/reports/link/imp/impl_1_full_util_routed.rpt build_dir.hw.xilinx_vck5000_gen4x8_qdma_2_202220_1/reports/link/imp/impl_1_xilinx_vck5000_gen4x8_qdma_2_202220_1_bb_locked_timing_summary_routed.rpt saved/v2/
```

## 3. Check V2's frequency

Open the saved timing report and find the period for `clkwiz_aclk_kernel_00_clk_out1`:

```bash
nano saved/v2/impl_1_xilinx_vck5000_gen4x8_qdma_2_202220_1_bb_locked_timing_summary_routed.rpt
```

Frequency in MHz is 1000 divided by the period in ns. Use the report's value in the table.

## 4. Find the V0 and V1 reports

Look for any saved copies, starting with what is in the lab folder:

```bash
ls -la ~/lab2_src
```

- **If they exist:** fill in V0's LUT, REG, DSP, frequency, and WNS, plus V1's LUTAsMem, BRAM, and URAM.
- **If V0's are gone:** rebuild V0 in tmux to get them, after finishing steps 1 and 2. Save its reports the same way, in `saved/v0`.
- **If only V1's extra fields are gone:** check whether the PDF's table requires them before spending a rebuild on them.

If a rebuilt version is also going to be submitted, rerun it on the VCK and confirm `TEST PASSED!`. If its time differs from the recorded one, use one consistent run and don't mix numbers across builds without saying so.

## 5. Finish the submission

1. Confirm the xclbin the PDF wants: name, and whether it wants one per version.
2. Write `report.pdf` with the completed table from [results.md](results.md). Label the third column V2, since the gathered table has it as V1.
3. Go through the checklist in [lab-requirements.md](lab-requirements.md).

If any V0 numbers can't be recovered, leave them blank and say why in the report rather than estimating them.
