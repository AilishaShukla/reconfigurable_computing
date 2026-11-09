# Server Setup

## Connection path

```text
Mac  --(Brown VPN)-->  arc-server  -->  vm1 (build here)  -->  vck5000 (run here)
```

| Machine | Used for | How to get there | Prompt |
|---|---|---|---|
| Mac | Starting point | | |
| VM | Building | `ssh vm1` from the Mac | `ailisha_shukla@localhost` |
| VCK5000 | Running hardware | `ssh vck5000` from the VM | `ailisha_shukla@arclab0` |

Check the prompt before running anything. Do not run `ssh vm1` from inside the VM.

## Mac to VM

Connect the Brown VPN first, using the F5 BIG-IP Edge Client.

Mac SSH config:

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

## VCK5000

Always run this on the VCK before running a hardware binary:

```bash
source /opt/xilinx/xrt/setup.sh
```

Without it you can get:

```text
libxrt_coreutil.so.2: cannot open shared object file
```

## Lab environment (VM)

```bash
cd ~/lab2_src
source env.sh
```

Expected variables:

```text
XILINX_XRT : /opt/xilinx/xrt
PATH : /opt/xilinx/xrt/bin:...
LD_LIBRARY_PATH : /opt/xilinx/xrt/lib:/opt/xilinx/xrt/lib
PYTHONPATH : /opt/xilinx/xrt/python:/opt/xilinx/xrt/python
```

| Setting | Value |
|---|---|
| `v++` | approximately `/tools/Xilinx/Vitis/2022.2/bin/v++` |
| Platform | `xilinx_vck5000_gen4x8_qdma_2_202220_1` |
| Kernel | `pl` |
| HLS clock | 300 MHz |
| Link frequency | 200 MHz |

## tmux

Builds take a long time and SSH/VSCode can disconnect, so run builds inside tmux.

```bash
tmux ls                            # list sessions
tmux new -s lab2_v2_fix            # create a session
tmux attach -t lab2_v2_fix         # reconnect to it
tmux detach                        # leave it running
tmux kill-session -t lab2_v2_fix   # end it
```

The current session is meant to be `lab2_v2_fix`. Earlier ones were `lab2`, `lab2_v1`, and `lab2_v2`.

Do not kill a session while a build you still need is running in it.
