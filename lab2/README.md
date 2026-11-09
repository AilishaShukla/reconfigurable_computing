# ENGN 2911X Lab 2: Project Docs

Vitis HLS / VCK5000 lab applying HLS pragmas to tiled matrix multiplication (N=512, M=256).

These files split up `../brown_lab2_agent_handoff.md`, which is unchanged and remains the original record. They also include the results Ailisha gathered afterward.

## Status

| Version | Functional test | Time/GOPS | Resources and timing |
|---|---|---|---|
| V0 | PASSED | Recorded | **LUT, REG, DSP, and WNS missing** |
| V1 | PASSED | Recorded | Main numbers recorded; LUTAsMem, BRAM, URAM missing |
| V2, first attempt | **FAILED**, not usable | Do not report | Not applicable |
| V2, corrected | PASSED | Recorded | Recorded |

## Files

| File | Covers |
|---|---|
| [next-steps.md](next-steps.md) | **Start here.** What is left, one command at a time |
| [results.md](results.md) | Measured numbers and the report table |
| [working-rules.md](working-rules.md) | How to work with Ailisha, integrity rules, past mistakes |
| [lab-requirements.md](lab-requirements.md) | Lab goal, V0/V1/V2 definitions, submission list |
| [server-setup.md](server-setup.md) | VPN, SSH hops, XRT, environment, tmux |
| [build-and-run.md](build-and-run.md) | Hardware build, running on the VCK5000, reports, hw_emu |
| [v2-debugging.md](v2-debugging.md) | V2 architecture, addressing, the failure and the fix |

## Open gaps

1. **V0 resources and WNS are missing**, along with V1's LUTAsMem, BRAM, and URAM. See [results.md](results.md).
2. **The frequency values conflict.** V1 is 202.020 MHz in its timing report but 200 MHz in the gathered table. See [results.md](results.md).
3. **The corrected V2 code may exist only in `src/pl.cpp`.** The submission needs it in `src/pl_v2.cpp`. See [next-steps.md](next-steps.md).
4. **The Lab 2 PDF is not in this folder.** The handoff refers to it as `turn0file0`, from an earlier conversation. The section summaries in [lab-requirements.md](lab-requirements.md) come from the handoff, not from the PDF itself.
