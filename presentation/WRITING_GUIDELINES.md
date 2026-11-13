# Writing Guidelines for the H2-LLM Slides

These rules apply to every slide, speaker note, and the Q&A write-up.

## Hard rules
1. **No em dashes. Ever.** Not on slides, not in notes, not in titles. Use a period, a comma, a colon, or "and" instead. Watch for en dashes used as em dashes too. A plain hyphen is fine inside words (low-batch, near-memory) and for number ranges (1-16).
2. **Results must be on the slides.** Real numbers from the paper (speedup, energy, overhead) go on the results slides, not just in the notes.
3. **Cite where it came from.** When possible, every claim or number gets a small source tag that points to the paper, like `Sec. 3.2`, `Fig. 3`, `Table 2`. Put it in small text at the bottom of the slide or right next to the number.

## Language
- Write at a high school reading level. Short sentences. Common words.
- Keep it technical but not too technical. Use the real terms (DRAM, bandwidth, prefill, decoding) but explain each one the first time in a few plain words.
- Say it once. No repeating the same point in different words.
- Slides hold short phrases, not paragraphs. Aim for 6 words or fewer per bullet and no more than 4 bullets per box.
- Use active voice: "The PE reads the bank," not "The bank is read by the PE."
- Use "I" and "my" on the Your Thoughts slide. It is my opinion, so it should sound like mine.

## Make it sound human, not AI
Avoid these words and patterns. They are tells of AI-written text:
- Words: delve, leverage, harness, seamless, robust, cutting-edge, game-changer, revolutionize, unlock, landscape, realm, tapestry, pivotal, crucial, paramount, holistic, synergy, novel (unless quoting the paper), "it's worth noting", "in today's world".
- "Not just X, but Y" and "It's not X, it's Y" sentence shapes.
- Lists of exactly three adjectives ("fast, efficient, and scalable") used for rhythm.
- Rhetorical questions as slide titles ("Why does this matter?").
- Every bullet starting with a bolded label and a colon.
- Emojis, sparkle icons, and arrow chains like A → B → C → D on every slide.
- Ending slides with a summary line that just restates the slide.
- Perfectly parallel bullets everywhere. Some variation in length is normal.

Do these instead:
- Titles say the point: "In-die NMP runs out of compute at batch 8", not "Motivation".
- Use a concrete number or example instead of an adjective. "3.81x faster" beats "much faster".
- It is fine to say what surprised me, or what I am unsure about.
- Mention limits honestly (for example, results come from simulation, not a real chip).

## Design
- Stick to the template's fonts (BBH Bartle for titles, Space Mono for body) and colors (#0e0e0e, #dedede, #bfc9ca, accent #9e0e0e).
- Every slide gets something visual: a diagram, chart, table, or big number. No text-only slides.
- Charts should be real, editable PowerPoint charts, not screenshots, when possible.
- Paper figures can be used if they are labeled with their figure number.
- Remove all leftover template text and Slidesgo instruction slides. Keep the Slidesgo credit on the Thank You slide if the free license requires it.

## Timing
- 5 minutes total including Q&A. About 45-60 seconds per slide.
- 6-7 slides max: Motivation, Architecture, Key Features (1-2), Results (1-2), Your Thoughts.
- Speaker notes hold the extra detail. The slide holds only what the audience needs to see.

## Before calling it done
- [ ] Search every slide and note for "—" and "–". Zero results.
- [ ] Every number on a slide has a source tag.
- [ ] No leftover template text (lorem, "Modern Business Proposal", "yourwebsite.com", etc).
- [ ] Read each slide out loud. If it sounds like a press release, rewrite it.
- [ ] Timed run under 5 minutes.
