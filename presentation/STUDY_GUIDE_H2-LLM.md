# H2-LLM Study Guide: Paper, Slides, Script, and Q&A Prep

Paper: *H2-LLM: Hardware-Dataflow Co-Exploration for Heterogeneous Hybrid-Bonding-based Low-Batch LLM Inference*. Li et al., ISCA 2025, Best Paper. Say the name as "H-squared LLM" or "H-two LLM."

How to use this file:
1. Read Part 1 once to get the whole story.
2. Learn the Part 2 glossary until you can explain each term in one sentence.
3. Practice the Part 4 script out loud with a timer. Target: about 4:05.
4. Read the Part 5 Q&A bank the night before.

---

## Part 1. The whole paper in 60 seconds

LLMs are moving onto edge devices like home assistants and private servers. These serve only 1 to about 16 users at once. At that small batch size, writing each new token (decoding) is limited by memory bandwidth, not by math.

The current fix is **in-die NMP**: put tiny compute units inside the DRAM chip. That raises bandwidth 4 to 16x. But DRAM is a poor process for logic, so those units are weak, at only 1 to 2 FLOPs per byte. Past batch size 8 they stop helping.

**H2-LLM's hardware idea:** use **hybrid bonding** to stack the DRAM die on a separate **logic die** built in a real logic process (40 nm in the study). Each DRAM bank gets its own processing engine (PE) on that logic die. The catch is that every bonding pin needs a controller, and the controllers take area away from compute. So bandwidth and compute trade against each other.

**H2-LLM's software idea:** a **data-centric dataflow**. Instead of hard-coding "attention goes to memory," it decides which memory channels hold which weights. It can split one matrix multiply between the main chip and the PEs. A **genetic algorithm** then searches hardware settings and dataflow together.

**Result (simulated):** 2.72x faster (geomean) and 1.48x better energy efficiency than the best in-DRAM designs. The biggest win is 3.81x on decode-heavy chat and code. The most interesting finding is that **more bandwidth is not always better**. A balanced design (25.6 GB/s per PE) beats the max-bandwidth one (51.2 GB/s).

---

## Part 2. Glossary (know each in one sentence)

| Term | Plain meaning |
|---|---|
| **Edge / low-batch inference** | Running an LLM on a local device that serves 1 to a few tens of requests at a time. |
| **Prefill** | Reading the whole prompt in one big step. Lots of tokens at once, so it is **compute-bound**. Runs on the main processor. |
| **Decoding** | Writing the answer one token at a time. Each step reads all the weights but does little math, so it is **memory-bound**. |
| **Arithmetic intensity** | FLOPs per byte read from memory. Low means memory-bound, high means compute-bound. |
| **Roofline** | A plot of performance vs. arithmetic intensity. A chip is limited by bandwidth on the left and by peak compute on the right. Fig. 3 in the paper. |
| **NMP (near-memory processing)** | Putting compute next to DRAM banks so it can use the much higher bandwidth inside the memory. |
| **In-die NMP** | NMP built inside the DRAM die itself. Examples: Samsung HBM-PIM and LPDDR5-PIM, SK Hynix AiM. |
| **Hybrid bonding (HB)** | 3D stacking where two dies are joined face to face with direct copper-to-copper links. About 110,000 links per mm² at 3 µm pitch. |
| **Logic die** | A separate chip made in a normal logic process, stacked under the DRAM die. This is where H2-LLM's PEs live. |
| **HB controller** | The logic that drives the bonding pins. More pins means more bandwidth, but also more controller area. |
| **Centralized processor (CP)** | The main accelerator (GPU, TPU, or similar). In the paper it is TPU-like: 8 systolic arrays of 128x128 at 1 GHz, with 128 MB SRAM. |
| **PE / FPU / MAC** | PE = processing engine, one per DRAM bank. FPU = floating-point unit, with 16 MACs each. MAC = one multiply-accumulate. |
| **FC / GEMM** | Fully connected layer, a big matrix multiply. GEMM is the general name for matrix multiply. |
| **MHA / GQA / MQA** | Attention types. MHA: every query head has its own K/V. GQA: groups of query heads share K/V. MQA: all share one K/V. Sharing raises arithmetic intensity, which hurts weak in-die NMP. |
| **Parallel transformer** | A layer where attention and the FFN read the same input and can run at the same time. PaLM uses it. |
| **MAG (Memory Access Group)** | A set of operators in a layer that can run at the same time. Step 1 of the dataflow. |
| **Operator-channel binding** | Assigning each operator's weights to specific memory channels (normal, NMP, or both). |
| **Operator fission** | Splitting one operator so part runs on the CP and part on the NMP PEs, without copying weights. |
| **DSE (design space exploration)** | Automatic search for the best design. Here it uses a genetic algorithm. |
| **Geomean** | Geometric mean. The standard way to average speedups. It is why the paper says "2.72x geomean." |

---

## Part 3. Deep dive by section (with the "why")

### 3.1 Motivation (paper Sec. 1 to 3, your Slide 2)

**Why is decoding memory-bound?** A quick derivation of my own (not in the paper, but consistent with it): in FP16 each weight is 2 bytes. With batch size B, each weight is used for B multiply-adds, which is 2B FLOPs. So an FC layer's intensity is about **B FLOPs per byte**. At batch 1 that is 1 FLOP/byte, which is tiny.

**Why does in-die NMP stop at batch 8?** Another quick derivation of my own that matches Fig. 3:
- 8 LPDDR5-PIM channels give about 0.8 TFLOPS of NMP compute (102.4 GFLOPS each).
- The main chip reads through the external interface at about 102.4 GB/s total (12.8 GB/s per channel x 8). In decoding it gets about 102.4 GB/s x B FLOPs.
- These meet at **B = 8**. Past that, in-die NMP's weak compute is the limit, while the main chip is still memory-bound. Nobody wins.
- Attention works the same way with KV heads. With 32 query heads, at **4 or fewer KV heads** the intensity is 8 or more, and in-die NMP stops helping.

**Why is DRAM bad at logic?** (Sec. 3.2) Transistors are 3x slower and logic density is 10x lower than CMOS at the same node. There are fewer metal layers. And only about 25% of the die can be logic (SK Hynix AiM's number).

**Why not HBM?** Duplex put PEs on HBM's base logic die. It works, but HBM uses too much power for an edge device. HB has lower parasitic capacitance, so it uses less power per bit. In the paper's energy model: **HB I/O 0.88 pJ/bit vs. LPDDR5 interface 7.0 pJ/bit**, about 8x cheaper.

**Dataflow problems in older work** (Sec. 3.3, Table 3):
| Design | Mapping | Fission |
|---|---|---|
| GPU + HBM-PIM (Samsung) | Fixed: FC only | No |
| GPU + GDDR6-AiM (SK Hynix) | Fixed: single-batch FC + attention | No |
| NeuPIMs | Fixed: attention only | No |
| AttAcc | Fixed: attention + fixed FFN split | Fixed |
| SpecPIM | Compute-centric search, prefill-unaware | No |
| **H2-LLM** | **Data-centric search, prefill-aware** | **Flexible** |

SpecPIM's problem: it picks the engine first (CP or NMP), then gives that operator only that engine's channels. That cuts the CP's external bandwidth, which can turn compute-bound prefill into memory-bound prefill. Edge processors have compute-to-bandwidth ratios of 500 to 1000, so losing bandwidth hurts.

### 3.2 Architecture (Sec. 4, Fig. 5 and 6, your Slide 3)

- **System:** CP + memory system with 8 channels, each with 16 DRAM banks of 256 MB, so 32 GB total. LPDDR5-6400 external interface.
- **Two channel types:** normal (plain DRAM) and HB-NMP (DRAM die on a logic die).
- **Two modes per HB-NMP channel:** normal mode (the CP reads it) or NMP mode (all PEs read their banks at once). Only one at a time, to avoid row-buffer interference.
- **Per PE:** FPUs (16 MACs each), a weight/KV buffer, an output buffer, and its own HB controller. **One input global buffer is shared** by all PEs in a channel, so the input is not copied to every bank.
- **No network-on-chip.** The execution flow never needs PEs to talk to each other, so the area goes to compute.
- **PEs only do MACs.** Softmax and norms need all outputs first (low parallelism), and element-wise ops fuse into the CP's merge step.
- **Execution flow (offload model):** (1) the CP scatters inputs, (2) PEs compute in parallel, (3) the CP gathers and merges. Inside a channel it is **output-stationary**: load an input tile into the global buffer, stream weight tiles, accumulate, then write the output back to the bank.
- **Splitting across channels:** weights are never split along the input dim, to avoid duplication. The tiling factors for the other two dims come from a closed-form formula (Eq. 1). Bad tiling costs **1.5 to 5.4x more overhead** (Fig. 17).
- **4 commands:** Mode Change (also acts as a memory barrier), Near-Memory Computation, Input Global Buffer Move, Local Buffer Move. All are coarse-grained "all-bank" commands, so one command drives every PE in the channel.

### 3.3 Key feature 1: the compute vs. bandwidth trade-off (Sec. 3.2, 4.4, 7.1, your Slide 4)

| HB pins per bank | Bandwidth per PE | Controller share of PE area |
|---|---|---|
| 128 | 6.4 GB/s | 4.6% |
| 256 | 12.8 GB/s | 10.7% |
| 512 | 25.6 GB/s | 19.7% |
| 1024 | 51.2 GB/s | 40.2% |

Each pin runs at 0.4 Gbps. The HB area numbers come from the authors' **real tape-out** (ref [55]). The FPUs are synthesized in 40 nm with Chisel.

**Table 6, max FPUs per PE** at 0.4 / 0.6 / 0.8 / 1.0 GHz:
- 6.4 GB/s: 8, 8, 8, 8
- 12.8 GB/s: 8, 8, 8, 4
- 25.6 GB/s: 8, 8, 4, 4
- 51.2 GB/s: 8, 4, 4, 2

More bandwidth leaves room for fewer or slower FPUs.

**Design space (Table 5):**
- NMP channels: {2, 4, 6, 8}
- FPUs per PE: {1, 2, 4, 8}
- Frequency: {0.4, 0.6, 0.8, 1.0} GHz
- HB bandwidth: {6.4, 12.8, 25.6, 51.2} GB/s
- Input global buffer: 4 to 128 KB
- Weight buffer: 4 to 128 KB
- Output buffer: 0.25 to 8 KB

**The chosen design** (Sec. 7.4 default): 4 NMP channels, 8 FPUs at 0.6 GHz, 25.6 GB/s, 32/32/4 KB buffers.
- My arithmetic: 8 x 16 MACs x 2 FLOPs x 0.6 GHz = **153.6 GFLOPS per PE**. Against 25.6 GB/s that is **6 FLOPs per byte**, versus 1 to 2 for in-die NMP.

### 3.4 Key feature 2: data-centric dataflow + DSE (Sec. 5 and 6, your Slide 5)

1. **MAG partition.** Split the layer's operator graph into ordered groups whose operators don't depend on each other. Operators that share an input can be combined to try different fission options.
2. **Coarse-grain binding (GCMap).** Inside a MAG, each independent sub-graph (an MPG, "memory partition group") gets its own subset of channels. All normal channels count as one "virtual channel," since the CP runs everything on them. Rules: no channel shared between groups, and every channel used.
3. **Fine-grain binding (OCMap).** Each operator inside a group gets specific channels.
4. **Execution mapping.** Prefill always runs on the CP. In decoding, an operator on only normal channels runs on the CP. One on only NMP channels runs on the PEs. One on both uses **fission**: a GEMM splits along the output dimension, and attention sends different heads' GEMMs to each side.
5. **Sync points.** The CP's controller syncs between MAGs, between tiers, and between the two halves of a split operator. It can do this because CP and PE timings are deterministic.
6. **DSE.** A genetic algorithm: random initial population, then a capacity checker throws out designs whose weights don't fit, then a simulator scores latency and energy, then the top-K are kept and evolved with re-sample, mutate, and crossover operators. The paper's settings: **100 rounds, 5,000 designs per round, top 50 kept**. For the full hardware search the population is 50k.
7. Open-sourced (Apache-2.0): github.com/leesou/H2-LLM-ISCA-2025. The artifact takes about 9 hours to reproduce.

### 3.5 Evaluation setup (Sec. 7.1)

- **Models (all FP16):** OPT 6.7B (MHA, 32/32 heads), LLaMA3 8B (GQA, 32 Q / 8 KV), PaLM 8B (MQA, 16 Q / 1 KV, parallel transformer).
- **Workloads (Table 1), average prompt / decode lengths:**
  - HumanEval (code): 157 / 67, decode-heavy
  - ShareGPT (chat): 783 / 209, decode-heavy
  - LongBench (context): 1886 / 97, prefill-heavy
  - LooGLE (QA): 1971 / 17, prefill-heavy
- Context length 2048. Batch sizes 1, 4, 16.
- **Baselines:**
  - **CP:** main chip only, with **2x compute** to be fair, since NMP adds compute.
  - **ID-NMP:** Samsung LPDDR5-PIM style, 1 FPU at 200 MHz per PE, 6.4 GB/s per PE.
  - **ID-NMP+:** SK Hynix AiM-style PE at 1 GHz, the strongest commodity in-die PE.
  - In ID-NMP and ID-NMP+, **all 8 channels are NMP**. They also get the new data-centric dataflow, so the comparison isolates the hardware.
- **Simulator:** Ramulator2, extended for the NMP PEs, plus the Tileflow performance model for CP operators.
- **Energy model:**
  - CP MAC: 0.682 pJ (10 nm)
  - HB-NMP MAC: 0.974 to 1.365 pJ, depending on clock
  - In-die MAC: 1.172 / 1.849 pJ
  - SRAM: 0.027 pJ/bit
  - LPDDR5 interface: 7.0 pJ/bit
  - HB I/O: 0.88 pJ/bit

### 3.6 Results (Sec. 7.2 and 7.3, your Slide 6)

All results are normalized to ID-NMP+.

| Result | Number | Source |
|---|---|---|
| CP alone | 27% of ID-NMP+ | Sec. 7.2 |
| ID-NMP vs. CP | 3.03x faster at batch 1 and 4, but only 71% of CP at batch 16 | Sec. 7.2 |
| ID-NMP+ vs. the better of CP and ID-NMP | only 1.76x | Sec. 7.2 |
| **H2-LLM, decode-heavy (HE, SG)** | **3.81x** | Sec. 7.2, Fig. 10 |
| **H2-LLM, prefill-heavy (LB, LG)** | **1.94x** | Sec. 7.2, Fig. 10 |
| **H2-LLM, all cases** | **2.71x** (Sec. 7.2), stated as 2.72x in the abstract | |
| **Mixed request lengths** | CP 0.24x, ID-NMP 0.49x, **H2-LLM 3.24x** | Fig. 11 |
| **Decoding energy efficiency** | **1.48x vs. ID-NMP, 1.54x vs. ID-NMP+**, thanks to SRAM buffer reuse | Sec. 7.2 |

**Dataflow comparison** (same H2-LLM hardware, Fig. 12 and 13):
- Attn-NMP gets only 14% of FC-NMP's speed, because it can't use the PEs for low-batch FC layers.
- CC-NMP (SpecPIM-style) is 1.24x over FC-NMP.
- **H2-LLM is 1.37x over FC-NMP and 1.11x over CC-NMP.**
- Against ID-NMP+: FC-NMP 1.98x, CC-NMP 2.45x, H2-LLM 2.71x.
- **Useful split:** the new hardware with a simple dataflow already gives about 2x. The smarter dataflow takes it to 2.7x.
- Prefill share of total time after decoding is accelerated: **12 to 26%** on decode-heavy work and **36 to 90%** on prefill-heavy work. H2-LLM's prefill is **1.27x** faster than CC-NMP's (Fig. 14).

**Hardware search** (Fig. 15): the full search adds **1.38x speed and 1.74x energy efficiency** over the fixed design.

**Overhead** (Fig. 16): sync and data transfer take **1.6 to 15.7%** of the time, mostly data movement.

### 3.7 The six takeaways (Sec. 7.4, your Slide 7)

1. As batch size grows, give the NMP more compute.
2. Balance compute and bandwidth. **8 FPUs at 0.6 GHz with 25.6 GB/s ranked best on average.** From 25.6 to 51.2 GB/s there was no gain at batch 4, and at batch 16 it got worse. At 6.4 GB/s even max compute doesn't help.
3. Bigger buffers help. Input buffer gives 1.05 to 1.18x, output buffer 1.09 to 2.26x, weight buffer up to 1.15x. 32/32/4 KB is the smallest size that saturates the gain.
4. As batch size grows, input and output buffers matter more and the weight buffer matters less, because weights get more reuse.
5. More NMP channels means more weights can live in NMP memory, which helps from 2 to 4 channels.
6. But too many makes each channel "wimpy." At 8 channels (all NMP) you also lose fission, since there are no normal channels left. 8 gives no gain and hurts at batch 16.

Plus Fig. 21: a stronger CP helps prefill-heavy work. You still need a capable main processor.

---

## Part 4. Slide-by-slide script

Target: about 4:05 total, at about 150 words per minute. That leaves about 55 seconds for questions inside the 5 minutes. Bold words are the ones to stress. The "if you have time" lines are extras. Skip them unless you are running fast.

### Slide 1: Title (about 0:15)
> Hi, I'm Ailisha. This is H2-LLM, which won **Best Paper** at ISCA 2025. The idea is to stack a logic chip under the DRAM, so small LLM workloads on edge devices get both bandwidth **and** compute. It ends up **2.72 times faster** than the best in-DRAM design.

### Slide 2: DRAM logic runs out of math (about 0:40)
> Edge LLMs, like home assistants, serve only a **few users** at once. When the model writes each new token, which is called decoding, it reads every weight but does very little math. So **memory** is the bottleneck. Today's fix puts small compute units **inside** the DRAM chip. That gives 4 to 16 times more bandwidth, but DRAM is bad at logic. Transistors are 3 times slower, density is 10 times lower, and you only get 1 to 2 FLOPs per byte. By **batch size 8**, it stops helping. Older designs also ignore prefill, which is reading the prompt. That can be up to **90%** of the time on long prompts.

*If asked why batch 8:* the main chip's throughput grows with batch size, and at batch 8 it catches up to the in-DRAM units' tiny compute.

### Slide 3: Logic die bonded under DRAM (about 0:35)
> Here's the system, from Figure 5. There's a TPU-like main chip, plus memory with two kinds of channels: normal DRAM, and HB-NMP channels, where a DRAM die sits **on top of a logic die**. On the logic die, each DRAM bank gets its **own PE**. A layer runs in three steps. The main chip **scatters** the inputs. Every PE does multiply-adds on its own bank, all in **parallel**. Then the main chip **gathers** and merges the results, and handles softmax and norms.

*If you have time:* "A channel is in normal mode or NMP mode, never both, so they don't fight over DRAM rows."

### Slide 4: Bandwidth costs area (about 0:30)
> Key idea one is **hybrid bonding**. Copper links join the DRAM to the logic die, about 110,000 per square millimeter, and the PEs get a real **40 nm** logic process. The catch is in this chart. Every pin needs controller logic. At 51.2 gigabytes per second, the controllers take **40%** of each PE. That's area lost from math. So H2-LLM tunes bandwidth, compute, and buffers **together** instead of just maxing out bandwidth.

### Slide 5: Map data, then search (about 0:30)
> Key idea two is the **dataflow**. Older designs decide by operator type, like "all attention goes to memory." H2-LLM decides by **data**. It groups operators that can run together, binds each one's weights to channels, and can **split one matrix multiply** between the main chip and the PEs. Then a **genetic algorithm** searches hardware and dataflow together. The main chip also keeps its full bandwidth, so prefill doesn't slow down.

### Slide 6: Result, 2.7x over in-DRAM NMP (about 0:30)
> These results are **simulated**, on three models of 7 to 8 billion parameters, compared to the strongest in-die design. With mixed requests, H2-LLM is **3.24 times** faster. The main chip alone, even with double the compute, gets 0.24. Chat and code, which are decode-heavy, see **3.81x**. Long documents still get **1.94x**. Over all tests it averages **2.71x**, with **1.48x** better decoding energy than in-die NMP.

### Slide 7: Balance beats max bandwidth (about 0:30)
> What I liked most is what the search found. At batch 4, doubling bandwidth from 25.6 to 51.2 did **nothing**, because the controllers ate the compute. The best all-around design is balanced: **8 FPUs at 0.6 gigahertz** with 25.6 gigabytes per second. The dataflow alone gives **1.37x** on the same hardware, and searching the hardware adds **1.38x** more. Sync and data movement cost only 1.6 to 15.7%.

### Slide 8: My two questions (about 0:40)
> My first question: why not just max out bonding bandwidth? I **expected** that to win. It didn't, because the link itself costs area, and past 25.6 there was no gain. I'd try **smaller or shared controllers**.
>
> My second: would this hold up on a **real chip**? It's all simulated. Only the bonding area comes from silicon, and the paper doesn't cover **heat** from stacking DRAM on logic. I'd also try **INT8 or INT4** PEs. The paper uses FP16, and smaller math units fit more compute in the same area.

### Slide 9: Thank you (about 0:05)
> Thanks. The paper and code links are on the slide. Happy to take questions.

**Timing tips**
- If you are running long, cut the last sentence of Slide 2 (prefill) and the energy sentence on Slide 6. Both numbers are on the slides anyway.
- Don't read the source tags out loud. They are for the reader.
- Practice the numbers until they're automatic: **2.72, 3.81, 1.94, 3.24, 1.48, 40%, 25.6, 1.37, 1.38**.

---

## Part 5. Q&A prep

### Likely "gotcha" questions about your slides

**Q: Slide 1 says 2.72x, but Slide 6 says 2.71x. Which is it?**
Both come from the paper. The abstract and conclusion say 2.72x, and the results section (Sec. 7.2) says 2.71x. It's a rounding difference in the geomean. The vs.-ID-NMP+ figure in the results text is 2.71x.

**Q: Is 1.48x energy versus ID-NMP or ID-NMP+?**
It's 1.48x versus ID-NMP and 1.54x versus ID-NMP+. ID-NMP is slightly more efficient than ID-NMP+ because it runs at 200 MHz with lower MAC energy. It's decoding energy efficiency, and it comes from on-chip SRAM buffer reuse.

**Q: You say 4 NMP channels beat 8. Is that always true?**
Not always. Going from 2 to 4 helps. Past 4 there is almost no gain, and at batch 16 it gets worse (Fig. 20). The fair phrasing is "4 is about as good or better than 8."

**Q: Why did the CP baseline get double compute?**
Fairness. H2-LLM adds compute in the PEs, so the CP-only baseline gets extra compute to match. It still loses, at 27% of ID-NMP+, because it is stuck behind the external memory bandwidth.

**Q: Do the baselines use the paper's new dataflow too?**
Yes. ID-NMP and ID-NMP+ also use the data-centric dataflow. That means the 2.7x comes from the hardware difference, not from giving the baselines a worse dataflow.

### Concept questions

**Q: Why is decoding memory-bound but prefill compute-bound?**
In decoding, each weight is read once and used only for the few requests in the batch, which is about B FLOPs per byte in FP16. Prefill processes hundreds or thousands of prompt tokens at once, so each weight gets reused thousands of times.

**Q: What exactly is hybrid bonding, and how is it different from HBM?**
HB bonds two dies face to face with direct copper-to-copper connections at a very fine pitch (about 3 µm, so about 110,000 per mm²). HBM stacks dies with through-silicon vias and sits next to the processor on an interposer (2.5D). HB gives high bandwidth with lower parasitic capacitance, so it uses less power. The paper models HB I/O at 0.88 pJ/bit vs. 7.0 pJ/bit for LPDDR5.

**Q: Why doesn't in-die NMP just add more PEs?**
DRAM makers cap logic at about 25% of the die to protect memory density. DRAM logic is also 3x slower and 10x less dense, with fewer metal layers.

**Q: Why do the PEs only do multiply-adds?**
Softmax and normalization need all the outputs gathered first, so they have little parallelism and low bandwidth needs. Element-wise ops fuse into the CP's merge step. Keeping PEs to MACs saves area for more compute.

**Q: Why no network-on-chip between PEs?**
The execution flow never needs PEs to talk to each other. Each PE works on its own output tile, and the CP does the merging. Skipping the NoC frees area for compute.

**Q: Why can't the CP and PEs use the same channel at the same time?**
They would interfere in the DRAM row buffers. A Mode Change command switches the channel and acts as a memory barrier.

**Q: What does "data-centric" mean versus "compute-centric"?**
Compute-centric (SpecPIM) picks the engine first (CP or NMP), and then the operator only gets that engine's channels. Data-centric picks which channels hold the operator's weights first. An operator on both kinds of channels gets split, and the CP keeps access to all normal channels.

**Q: What is operator fission and why not duplicate weights instead?**
Fission splits one GEMM along its output dimension, so part runs on the CP and part on the PEs, with no copies. Duplicating weights, like cloud prefill/decode disaggregation does, doubles the memory footprint. An edge device can't afford that.

**Q: Why a genetic algorithm?**
The joint space of hardware choices and dataflow choices (group partitions, channel maps, split ratios) is huge and discrete. A GA handles that well and can score each candidate with the simulator. The settings are 100 rounds x 5,000 candidates, keeping the top 50.

**Q: How do you avoid designs that don't fit in memory?**
A capacity checker throws out any candidate whose weights overflow a channel before it gets simulated.

**Q: Why does the number of NMP channels matter?**
PEs can only compute on weights stored in their own channel. Each channel is 4 GB (16 banks x 256 MB), and an 8B FP16 model is about 16 GB. With only 2 NMP channels, most weights would have to live in normal channels. That is my reading of Takeaway 5.

### Results and methodology questions

**Q: Is this real silicon?**
No. It's simulated with Ramulator2 plus Tileflow. The HB controller area numbers come from the authors' earlier real tape-out, and the FPUs were synthesized in 40 nm.

**Q: Why use 40 nm for the PEs when the CP's energy uses 10 nm?**
An older node is cheaper and has better yield, which suits edge parts. The paper says less advanced logic technology meets edge cost and yield needs. It also shows HB-NMP still wins even with an older node.

**Q: Where does the speedup come from: hardware or dataflow?**
Both. On H2-LLM hardware, a simple "all FC to NMP" dataflow already gets about 1.98x over ID-NMP+. The full data-centric dataflow takes it to 2.71x (Fig. 13). Searching the hardware per scenario adds another 1.38x on top of the fixed design (Fig. 15).

**Q: Why is the gain smaller on prefill-heavy workloads?**
Prefill runs on the CP in every design, so NMP can't speed it up much. H2-LLM still helps prefill by 1.27x versus CC-NMP, because it doesn't take bandwidth away from the CP. Prefill can be 36 to 90% of the time on long prompts.

**Q: What overheads does this add?**
Sync and data transfer take 1.6 to 15.7% of the time, mostly from moving data before and after. Good tiling matters, since bad tiling costs 1.5 to 5.4x more overhead.

**Q: Which models and why those three?**
OPT 6.7B (MHA), LLaMA3 8B (GQA), and PaLM 8B (MQA plus parallel transformer). Together they cover the main attention types, which changes the arithmetic intensity, and they are a realistic size for edge devices.

### Critique and "what would you do" questions

**Q: Biggest limitation?**
No silicon, so there's no data on heat from stacking DRAM on logic. DRAM retention gets worse when it's hot, which could force more refresh. Also, the operator templates are written by hand, and the paper calls automating them future work.

**Q: Why FP16 and not INT8 or INT4?**
The paper only tests FP16. Quantized edge models would halve or quarter the bytes per weight. That raises effective bandwidth, and smaller integer MACs fit more compute per mm². Both would move the best balance point. I'd want to see the search re-run with that.

**Q: The design is fixed at design time. What if the workload changes?**
The search can weight several workloads together, and the mixed-request test still gets 3.24x. But the hardware is chosen once. The dataflow could, in principle, change at runtime, and that's what I'd look at next.

**Q: How is this different from NeuPIMs or AttAcc?**
Those target large-batch cloud serving with HBM-based PIM and always send attention to memory. H2-LLM targets small-batch edge, uses a separate logic die, and decides per operator and per scenario. It can even split one operator.

**Q: How is it different from Duplex?**
Duplex also puts PEs on a logic die, but it's HBM's base die, and HBM uses too much power for the edge. H2-LLM uses hybrid bonding on LPDDR-class memory.

**Q: If you were the architect, what's one change you'd make?**
Reduce controller area, for example by sharing one controller across several banks or using a simpler PHY. The 40% controller cost at 1024 pins is what limits the design, so shrinking it moves the whole trade-off curve.

### If you blank on a question
Say: "Good question. The paper doesn't test that directly, but here's how I'd think about it..." Then connect it to the **compute vs. bandwidth balance**. Most hardware questions about this paper come back to that trade-off.

---

## Part 6. Number cheat sheet

| Number | Meaning |
|---|---|
| 2.72x / 2.71x | Overall geomean speedup vs. ID-NMP+ (abstract / Sec. 7.2) |
| 3.81x | Decode-heavy (HumanEval, ShareGPT) |
| 1.94x | Prefill-heavy (LongBench, LooGLE) |
| 3.24x | Mixed request lengths |
| 1.48x / 1.54x | Decoding energy efficiency vs. ID-NMP / ID-NMP+ |
| 27% | CP alone vs. ID-NMP+ |
| 1.37x / 1.11x | Dataflow vs. FC-NMP / CC-NMP on the same hardware |
| 1.27x | Prefill speedup vs. CC-NMP |
| 1.38x / 1.74x | Extra speed / energy from the full hardware search |
| 1.6 to 15.7% | Sync and data transfer overhead |
| 1.5 to 5.4x | Overhead from bad tiling vs. optimal |
| 4.6 / 10.7 / 19.7 / 40.2% | Controller share at 6.4 / 12.8 / 25.6 / 51.2 GB/s |
| 8 FPUs @ 0.6 GHz, 25.6 GB/s | Best balanced design |
| 3x / 10x / 25% | DRAM logic: slower, less dense, area cap |
| 1 to 2 FLOP/byte | In-die NMP compute-to-bandwidth ratio |
| Batch 8 | Where in-die NMP stops helping |
| 110,000/mm², 3 µm | Hybrid bonding density and pitch |
| 0.88 vs. 7.0 pJ/bit | HB I/O vs. LPDDR5 interface energy |
| 36 to 90% | Prefill share of time on prefill-heavy work |
| 100 x 5,000, top 50 | GA rounds x population, survivors |

---

## Part 7. Small fixes to consider on the slides

- **Slide 7, "4 beats 8":** the paper says 8 gives little or no gain and hurts at batch 16. Something like "More NMP channels is not always better: 8 is no faster than 4 and is slower at batch 16 (Fig. 20)" is more exact.
- **Slide 1 (2.72x) vs. Slide 6 (2.71x):** both are correct, but someone may ask. Keep the answer from Part 5 ready, or use one number on both slides.
- **INSTRUCTIONS / WRITING_GUIDELINES fonts:** these still list BBH Bartle and Space Mono. The deck now uses Inter, with Space Mono only on the source tags.
