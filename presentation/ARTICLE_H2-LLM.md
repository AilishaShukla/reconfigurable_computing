# H2-LLM — Paper Notes

## Citation
| | |
|---|---|
| **Title** | H2-LLM: Hardware-Dataflow Co-Exploration for Heterogeneous Hybrid-Bonding-based Low-Batch LLM Inference |
| **Authors** | Cong Li, Yihan Yin, Xintong Wu, Jingchen Zhu, Zhutianya Gao, Dimin Niu, Qiang Wu, Xin Si, Yuan Xie, Chen Zhang\*, Guangyu Sun\* |
| **Affiliations** | Peking University, Shanghai Jiao Tong University, Alibaba, Houmo AI, Southeast University, HKUST |
| **Venue** | ISCA '25 (52nd International Symposium on Computer Architecture), Tokyo, June 21–25, 2025, 🏆 **Best Paper** |
| **DOI** | https://doi.org/10.1145/3695053.3731008 |
| **PDF** | https://chenzhangsjtu.github.io//files/2025-H2_LLM_ISCA25.pdf (local copy is in this folder) |
| **Code** | https://github.com/leesou/H2-LLM-ISCA-2025 |

## Glossary (Know These Before Presenting)
| Term | Meaning |
|---|---|
| **Low-batch inference** | Serving only 1 to a few tens of requests at once. This is typical for edge devices: personal chatbots, smart-home assistants, reception bots. |
| **Prefill vs. decoding** | Prefill processes the whole prompt in one step, which is **compute-bound**. Decoding produces one token per step, which is **memory-bound** because there is little data reuse. |
| **NMP (Near-Memory Processing)** | Putting compute units (PEs) right next to DRAM banks so they get massive internal bandwidth. |
| **In-die NMP** | NMP where the PEs are built *inside the DRAM die* (Samsung LPDDR5-PIM, SK Hynix AiM). |
| **Hybrid bonding (HB)** | 3D stacking: a DRAM die is bonded face-to-face onto a **separate logic die** with direct copper-to-copper connections (~110,000 connections/mm² at 3 µm pitch). |
| **Centralized processor** | The main accelerator (a GPU or TPU-like chip) that handles compute-heavy work. |
| **MHA / GQA / MQA** | Attention variants. Each step from MHA → GQA → MQA shares more key/value heads, which raises arithmetic intensity. |
| **Operator fission** | Splitting one operator (e.g., one matrix multiply) so that part runs on the centralized processor and part runs on NMP. |
| **DSE** | Design Space Exploration: automatically searching for the best design. |

---

## Slide 1 — Motivation
**Problem:** LLMs are moving to the **edge** (personal assistants, smart homes, private servers). There, they serve **small batches** (1 to ~16 users), and existing hardware handles that poorly.

- **Decoding is memory-bound.** A centralized processor (GPU/TPU) is starved by external memory bandwidth.
- **Existing fix: in-die NMP** (compute inside DRAM chips). It gives 4–16× more bandwidth, but:
  - DRAM process technology is bad at logic: transistors are **3× slower**, logic density is **10× lower**, and there are fewer metal layers.
  - The logic area budget is tiny (~25% of the die), so the compute-to-bandwidth ratio is only **~1–2 FLOP/Byte**.
  - Roofline analysis (Fig. 3) shows in-die NMP **stops helping at batch size ≥ 8**, or with GQA/MQA attention, even though the main processor is *still* memory-bound.
- Moving compute to HBM's logic die (Duplex) uses too much power for edge devices.
- Existing dataflows **hard-code which operators go to NMP** (e.g., "all attention → NMP"). They also **ignore prefill**, which can be 36–90% of latency in long-prompt workloads.

**Why it matters:** edge LLMs need fast, low-power, private inference across *both* prefill and decoding.

**One-liner for the slide:** *"In-die NMP has bandwidth but no compute. Edge LLMs need both."*

---

## Slide 2 — Architecture Overview
**H2-LLM = Hybrid-bonding-based Heterogeneous accelerator for LLMs**

```
            ┌──────────────────────────────┐
            │  Centralized Processor (TPU) │  ← prefill, softmax, norms,
            │  8× 128×128 systolic arrays  │    compute-heavy GEMMs
            └──────────────┬───────────────┘
                           │ LPDDR5 external interface
     ┌──────────┬──────────┼──────────┬──────────┐
  Normal ch.  Normal ch.  HB-NMP ch.  HB-NMP ch.  ...
  (DRAM only)             ┌─────────────────────┐
                          │  DRAM die (banks)   │
                          │═══ Cu-Cu hybrid bond│
                          │  Logic die: 1 PE per│
                          │  bank + HB ctrl +   │
                          │  shared input buffer│
                          └─────────────────────┘
```

- **Two kinds of memory channels:** *normal* channels (plain DRAM) and *HB-NMP* channels, where a DRAM die is stacked on a logic die.
- **Each HB-NMP channel:** **one PE per DRAM bank**. Each PE has FPUs (16 MACs each), a weight buffer, an output buffer, and its own HB controller. One input global buffer is shared by all PEs. There is no NoC, to save area.
- **Two modes per channel:** *normal mode*, where the centralized processor reads DRAM, or *NMP mode*, where all PEs access their banks in parallel. Only one mode is active at a time, which avoids row-buffer conflicts.
- **Execution model (offload-based):** (1) the centralized processor scatters inputs to channels → (2) PEs compute in parallel, output-stationary → (3) the processor gathers and merges results.
- **Simple command set:** Mode Change, Near-Memory Computation, Input Buffer Move, Local Buffer Move. Commands are coarse-grained, "all-bank" commands.
- PEs do **only MACs (matrix multiplies)**. Softmax, normalization, and element-wise ops stay on the centralized processor or are fused into the merge step.

---

## Slide 3 — Key Features (1/2): Hardware Innovation
**1. Hybrid bonding instead of in-die NMP**
- PEs are built on a **real logic process** (40 nm in the study), not a DRAM process, so they are much denser and faster.
- Gives high bandwidth at **lower power than HBM**, which makes it viable for the edge.

**2. Treating the compute-vs-bandwidth trade-off as a first-class design knob**
- More HB I/O pins → more bandwidth, **but** the HB controllers eat logic-die area that could have been compute.
- Controller area per PE: **4.6% / 10.7% / 19.7% / 40.2%** for 128 / 256 / 512 / 1024 pins (6.4 → 51.2 GB/s).
- **Architecture design space:**
  - Number of HB-NMP channels (2 / 4 / 6 / 8)
  - PE compute: FPU count and frequency (0.4–1.0 GHz)
  - HB I/O bandwidth
  - SRAM buffer sizes (input / weight / output)

## Slide 4 — Key Features (2/2): Dataflow + Automatic Search
**3. Data-centric dataflow abstraction.** Map *data to channels* instead of *operators to engines*.
- **Step 1, Memory Access Groups (MAGs):** group independent operators so they can run in parallel. For example, in a parallel transformer, attention and the FFN share an input.
- **Step 2, Operator-channel binding:** assign each operator (or each part of one) to a set of memory channels, normal and/or NMP.
- **Operator fission:** one matrix multiply can be **split** between the centralized processor and NMP PEs, with no weight duplication.
- **Prefill-aware:** previous work (SpecPIM) took channels away from the processor and hurt prefill. H2-LLM keeps the full external bandwidth available.
- Tiling factors for splitting across channels come from a closed-form analytical model. Bad tiling costs **1.5–5.4× more overhead**.

**4. Hardware–dataflow co-exploration (DSE) framework**
- Inputs: the LLM model, the scenario (batch size, prompt and decode lengths), and candidate architectures.
- Uses a **genetic algorithm** (100 rounds × 5k candidates) that jointly searches the hardware design *and* the dataflow, with a capacity checker and a simulator in the loop.
- Open-sourced.

---

## Slide 5 — Experimental Results (1/2): Setup + Headline Numbers
**Setup (simulation, not silicon):**
- **Models:** OPT 6.7B (MHA), LLaMA3 8B (GQA), PaLM 8B (MQA, parallel transformer)
- **Workloads:** HumanEval (code), ShareGPT (chat) are *decoding-heavy*; LongBench, LooGLE are *prefill-heavy*. Context length 2048; batch sizes 1, 4, 16.
- **Hardware:** TPU-like processor (8× 128×128 systolic arrays at 1 GHz, 128 MB SRAM), 8 LPDDR5-6400 channels
- **PE logic:** synthesized in **40 nm** (Chisel). HB area numbers come from the authors' **real tape-out**.
- **Simulator:** extended Ramulator2 plus the Tileflow performance model

**Baselines:**
- **CP:** centralized processor only, with 2× the compute for fairness
- **ID-NMP:** Samsung LPDDR5-PIM-style in-die NMP
- **ID-NMP+:** in-die NMP with the strongest commodity PE (SK Hynix AiM-style, 1 GHz)

**Headline results (vs. ID-NMP+):**
| Metric | Result |
|---|---|
| End-to-end speedup, all cases | **2.72× (abstract) / 2.71× (Sec. 7.2), geomean** |
| Decoding-heavy workloads (HE, SG) | **3.81×** |
| Prefill-heavy workloads (LB, LG) | **1.94×** |
| Mixed-length requests | **3.24×** |
| Decoding energy efficiency | **1.48× vs. ID-NMP, 1.54× vs. ID-NMP+** |
| CP alone | only **27%** of ID-NMP+ performance |

## Slide 6 — Experimental Results (2/2): What the Exploration Reveals
- **The dataflow matters:** on the same hardware, H2-LLM's dataflow beats "all FC → NMP" by **1.37×** and the prior compute-centric search (SpecPIM-style) by **1.11×**. Prefill gets **1.27×** faster.
- **Full hardware DSE** adds a further **1.38× speedup and 1.74× energy efficiency** over the fixed design.
- Overhead from sync and data transfer is only **1.6–15.7%**.

**Architect takeaways from the paper:**
1. **Batch size 1:** bandwidth is king, so more HB pins win.
2. **Batch size 16:** you need more compute, so balance matters. The best overall design is **8 FPUs @ 0.6 GHz + 25.6 GB/s**, not the maximum bandwidth: going from 25.6 to 51.2 GB/s gives *no* gain because controllers eat the compute area.
3. **Buffers:** larger batches → input/output buffer size matters more, and weight buffer size matters less.
4. **Channel count:** 2 → 4 NMP channels helps. 8 does not, because each channel gets "wimpy" and operator fission is lost.
5. **The main processor still matters** for prefill-heavy workloads.

**What it demonstrates:** stacking a logic die on DRAM, *plus* searching hardware and dataflow together, beats compute-in-DRAM for edge LLMs on both speed and energy.

---

## Slide 7 — Your Thoughts: 2 Questions & Answers

**Q1. Why not just maximize hybrid-bonding bandwidth, since decoding is memory-bound?**
*Answer:* That was the most surprising result for me. The HB controllers needed to drive 1024 pins take **~40% of each PE's logic area**, so maximum bandwidth leaves too little compute. Going from 25.6 to 51.2 GB/s gave **no speedup at batch 4**, and it hurt at batch 16. The best design was a *balanced* one. The lesson is that in 3D integration, the interface itself costs area, so bandwidth is not "free." As an architect, I would want a smaller controller design, or controllers shared across banks, so the trade-off curve moves outward.

**Q2. What would I try next?**
*Answer:* Three things:
- **Silicon:** results are simulated, with per-MAC energy taken from synthesis. Only the HB area numbers come from a real chip. A full tape-out would validate the thermal behavior of stacking DRAM on hot logic, which the paper does not address.
- **Lower precision:** the PEs use floating-point MACs. Edge LLMs usually run **INT4/INT8 quantized**, so the same area could hold several times more MACs. That would shift the compute-bandwidth balance point.
- **Runtime adaptivity:** the DSE picks *one* design per scenario offline. Real edge devices see changing batch sizes and prompt lengths, so I would explore switching dataflows at runtime.

*(Backup Q for Q&A: How does this differ from NeuPIMs/AttAcc? Those put compute inside HBM/DRAM and always offload attention. H2-LLM uses a separate logic die and decides per operator, per scenario, what to offload, including splitting a single operator.)*

---

## Presentation Timing (5 min)
| Slide | Time |
|---|---|
| 1 Motivation | 0:45 |
| 2 Architecture | 1:00 |
| 3–4 Key Features | 1:00 |
| 5–6 Results | 1:00 |
| 7 Questions | 0:45 |
| Q&A buffer | 0:30 |
