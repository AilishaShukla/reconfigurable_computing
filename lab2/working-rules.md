# Working Rules

## Instructions and communication

- Ailisha Shukla is a Brown ECE master's student.
- Give simple, step-by-step terminal instructions.
- Usually give **one command at a time**, then wait for the output.
- Make commands copy/pasteable, with a brief explanation of what each one does.
- Avoid unnecessary `grep`, which can be slow. Prefer `nano`, `cat`, `ls`, or targeted commands.
- Long FPGA builds and server failures cause anxiety. Base reassurance on actual logs, not guesses.

## Source code changes

- Preserve **all existing comments exactly**.
- Change only the code that needs changing.

## Result integrity

- Do not fabricate measured FPGA results or falsely claim `TEST PASSED!`.
- Simulated or estimated values are acceptable only if the assignment allows them and they are clearly labeled.
- An unsuccessful hardware run can be documented honestly.
- If V2 stays broken, debug it, use `hw_emu` for functional diagnosis, and get real hardware measurements if possible.
- Never present local CPU timing as FPGA timing.

## Past mistakes to avoid

1. Do not run `ssh vm1` from inside the VM or the VCK.
2. Do not forget `source /opt/xilinx/xrt/setup.sh` on the VCK before `./host.exe`.
3. Do not interrupt long builds because the output has gone quiet.
4. Do not use the failed V2 execution result in the final report.
5. Do not change comments in `pl.cpp` unless absolutely necessary.
6. Do not blindly modify V2 if the corrected build fails. Inspect the exact failure output first.
7. Do not assume V2 is correct because the xclbin built. `TEST PASSED!` is required.
