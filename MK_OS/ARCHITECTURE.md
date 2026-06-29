# MK OS Architecture

Deep technical documentation of the generation chain, engine internals, and data flow.

---

## The Generation Chain

MK OS generates responses through a cascading pipeline of four engines. Each engine
attempts generation; if it cannot produce a confident result, control falls through
to the next engine. This guarantees a response for every input while maximizing
quality.

```
Input
  │
  ▼
┌─────────────────────────────────┐
│      PROMETHEUS ENGINE          │  Tier 1: Fluid Intelligence
│  (droplets, flow, birth)        │
└──────────────┬──────────────────┘
               │ fallback (confidence < threshold)
               ▼
┌─────────────────────────────────┐
│      CRYSTAL NETWORK (CXN)      │  Tier 2: Resonance Matching
│  (crystals, resonance, refine)  │
└──────────────┬──────────────────┘
               │ fallback (no strong resonance)
               ▼
┌─────────────────────────────────┐
│   CONSCIOUSNESS ENGINE (MCE)    │  Tier 3: Thought Assembly
│  (atoms, chains, weave, echo)   │
└──────────────┬──────────────────┘
               │ fallback (assembly incomplete)
               ▼
┌─────────────────────────────────┐
│      TEMPLATE SYSTEM            │  Tier 4: Guaranteed Output
│  (602 conversation templates)   │
└─────────────────────────────────┘
```

---

## Prometheus Engine

**Location:** `ai_core/prometheus/`

The Prometheus Engine models language as a physical fluid system. Words are not
static tokens but dynamic **droplets** with properties:

### Core Concepts

| Concept | Description |
|---------|-------------|
| **Droplet** | A word/phrase with momentum, weight, temperature, and surface tension |
| **Droplet Pool** | Collection of active droplets being processed |
| **Fluid Dynamics** | Rules governing how droplets flow, merge, split, and crystallize |
| **Birth Chamber** | Where raw droplet streams solidify into coherent sentences |
| **Precision Spectrum** | Single dial from "creative/casual" to "precise/technical" |
| **Code Fragments** | 145 pre-compiled code patterns for technical responses |

### How It Works

1. **Tokenize** - Input is broken into droplets with initial properties
2. **Flow** - Droplets interact through fluid dynamics (attraction, repulsion, merging)
3. **Temperature** - Controls creativity: hot = more fluid/creative, cold = more structured
4. **Birth** - The Birth Chamber catches stable droplet configurations and births them as sentences
5. **Precision** - A single spectrum value determines whether output is conversational or technical

### Why Fluid?

Traditional template systems produce rigid output. Neural networks require GPU.
Fluid dynamics gives MK a middle ground: responses emerge organically from
word interactions, producing natural variety without any external model.

---

## Crystal Network (CXN)

**Location:** `ai_core/cxn/`

CXN stores knowledge and response patterns as **crystals** -- structured data
objects with resonance properties.

### Core Concepts

| Concept | Description |
|---------|-------------|
| **Crystal** | A response pattern with trigger words, semantic weight, and output template |
| **Crystal Store** | Database of all crystals indexed by trigger signature |
| **Resonance Engine** | Measures how strongly an input resonates with each crystal |
| **Crystallizer** | Refines raw resonance matches into polished output |

### How It Works

1. **Extract Triggers** - Input is decomposed into semantic trigger words
2. **Resonate** - Each trigger is tested against the crystal store; resonance scores computed
3. **Select** - Highest resonance crystal(s) selected as response candidates
4. **Crystallize** - The Crystallizer combines crystal output with context to produce final text

### Bootstrap Data

CXN is bootstrapped with:
- Casual response patterns from personality modules
- Knowledge facts converted to crystal format
- Hand-crafted crystals for common interaction patterns

---

## Consciousness Engine (MCE)

**Location:** `ai_core/mce/`

MCE models response generation as a **thought process** with five distinct layers,
each progressively refining raw input into articulated output.

### The Five Layers

```
Layer 1: Thought Atoms
    Input is decomposed into atomic meaning units (subject, action, object, context)
        │
        ▼
Layer 2: Chain Reactor
    Each atom triggers associated memories and related concepts
    Chains propagate outward like a nuclear reaction
        │
        ▼
Layer 3: Thought Assembler
    Selects the most relevant chain results
    Builds a response blueprint (structure without words)
        │
        ▼
Layer 4: Word Weaver
    Walks a word-association graph to fill the blueprint with actual language
    Produces multiple candidate phrasings
        │
        ▼
Layer 5: Echo Memory
    Learns the user's speech patterns over time
    Adjusts word choice, sentence length, and formality to match
```

### Design Philosophy

MCE treats conversation as consciousness: thoughts don't appear fully formed.
They start as atoms, react and grow, get assembled into structure, then are
woven into words. This produces responses that feel considered rather than
retrieved.

---

## The Four Dimensions

MK OS intelligence is organized around four orthogonal dimensions:

### 1. Meaning Dimension

**What things are and how they relate.**

- Powered by: Knowledge Graph (9,000+ facts in .mk format) + HRE (Hybrid Reasoning Engine)
- Pattern Graph stores facts as weighted source|relation|target triples
- Deep Reasoner performs multi-hop inference (A relates to B, B relates to C, therefore A may relate to C)
- Compositional Logic combines facts to answer complex queries

### 2. Structure Dimension

**How to organize thoughts into language.**

- Powered by: Prometheus Engine + Crystal Network
- Fluid dynamics handle creative/casual structure
- Crystal resonance handles factual/structured output
- Precision spectrum selects the right balance automatically

### 3. Self Dimension

**Adapting to the user.**

- Powered by: MCE Echo Memory + Personality modules
- Tracks user vocabulary, sentence length preferences, formality level
- Adjusts over time without explicit configuration
- Maintains consistent personality while adapting style

### 4. Evolution Dimension

**Getting smarter over time.**

- Powered by: Self-Improver + Knowledge Grower + Rule Generator
- Nightly: identifies knowledge gaps, researches them, generates new facts
- Learns from corrections ("no, actually X is Y")
- Auto-generates new reasoning rules from observed patterns
- Self-programming: can write new code modules for itself

---

## Data Flow: Complete Request Lifecycle

```
User types: "What causes earthquakes?"
    │
    ▼
┌─ Input Preprocessor ─────────────────────────────────────────┐
│  1. Normalize text (trim, lowercase for matching)            │
│  2. Detect language (English/Arabic/other)                   │
│  3. Expand contractions, fix typos                           │
│  4. Extract intent keywords: [cause, earthquake]             │
└──────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ Smart Router ───────────────────────────────────────────────┐
│  Score query against categories:                             │
│    knowledge: 0.85  (factual question)                       │
│    search:    0.30                                           │
│    math:      0.05                                           │
│    code:      0.02                                           │
│  Route to: Knowledge Graph                                   │
└──────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ Knowledge Graph Query ──────────────────────────────────────┐
│  Search: "earthquake" -> finds facts:                        │
│    earthquake|caused_by|tectonic_plate_movement|0.95         │
│    earthquake|occurs_at|fault_lines|0.88                     │
│    tectonic_plates|float_on|mantle|0.82                      │
│  Deep Reasoner chains: plates -> stress -> release -> quake  │
└──────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ Generation Chain ───────────────────────────────────────────┐
│  Prometheus: temperature=cold (factual), precision=high      │
│    Droplets: [earthquakes, caused, tectonic, plates, move]   │
│    Flow: merge related droplets, birth coherent sentence     │
│    Output: "Earthquakes are caused by the movement of..."    │
│    Confidence: 0.78 (above threshold)                        │
│  Result: Prometheus output accepted                          │
└──────────────────────────────────────────────────────────────┘
    │
    ▼
┌─ Output ─────────────────────────────────────────────────────┐
│  Format response with confidence indicator                    │
│  Display in REPL / send via Telegram / speak via TTS         │
└──────────────────────────────────────────────────────────────┘
```

---

## Knowledge Format (.mk files)

All knowledge is stored in plain text `.mk` files:

```
# Section header (comment)
source|relation|target|weight
```

Example:
```
# Physics
earth|has_property|gravity|0.99
light|travels_at|299792458_m_per_s|0.99
photon|is_a|particle|0.95
photon|is_a|wave|0.95
```

- **source** - The subject entity
- **relation** - How source relates to target
- **target** - The object entity
- **weight** - Confidence/importance (0.0 to 1.0)

Knowledge files are loaded at startup by the Pattern Graph into an in-memory
weighted directed graph for fast traversal.

---

## Module Dependency Graph

```
mk_entry.cpp (includes everything)
    │
    ├── ai_core/input_preprocessor.cpp
    ├── ai_core/smart_router.cpp
    │
    ├── ai_core/prometheus/prometheus_engine.cpp
    │       ├── droplet.cpp
    │       ├── fluid_dynamics.cpp
    │       ├── birth_chamber.cpp
    │       └── code_fragments.cpp
    │
    ├── ai_core/cxn/cxn_engine.cpp
    │       ├── crystal.cpp
    │       ├── crystallizer.cpp
    │       └── resonance.cpp
    │
    ├── ai_core/mce/mce_engine.cpp
    │       ├── thought_atoms.cpp
    │       ├── chain_reactor.cpp
    │       ├── thought_assembler.cpp
    │       ├── word_weaver.cpp
    │       └── echo_memory.cpp
    │
    ├── ai_core/hre/hre_main.cpp
    │       ├── pattern_graph.cpp (+ knowledge_files/*.mk)
    │       ├── deep_reasoner.cpp
    │       ├── reasoning_chains.cpp
    │       ├── composer.cpp
    │       └── ... (20+ sub-modules)
    │
    ├── mk_brain/* (learning, memory, personality, embeddings)
    ├── network/* (HTTP, search, APIs)
    ├── plugins/* (Telegram, plugin API)
    ├── security/* (AES, auth, rate limiting)
    └── tools/* (code runner, file reader)
```

---

## Build System

MK OS uses a **single compilation unit** pattern:

- `mk_entry.cpp` includes all other `.cpp` files via `#include`
- The compiler sees the entire project as one translation unit
- This enables aggressive inlining and whole-program optimization
- Build command: `clang++ -std=c++17 -O2 system/mk_entry.cpp -o mk_os -lcurl -lsqlite3`
- Tests compile separately: `clang++ -std=c++17 tests/test_main.cpp -o mk_os_test`

Advantages:
- Fast incremental builds (single file to compile)
- No linker errors from missing symbols
- Compiler can optimize across all module boundaries
- Simple build system (just one Makefile rule)

---

## Self-Improvement Loop

```
┌─────────────────────────────────────────────┐
│              Every Night at 3 AM            │
├─────────────────────────────────────────────┤
│  1. Analyze today's failed queries          │
│  2. Identify knowledge gaps                 │
│  3. Search internet for missing facts       │
│  4. Generate new .mk knowledge entries      │
│  5. Generate new reasoning rules            │
│  6. Test new rules against known queries    │
│  7. Commit improvements to knowledge base   │
│  8. Log what was learned                    │
└─────────────────────────────────────────────┘
```

The Self-Improver and Knowledge Grower work together to make MK OS
progressively smarter without any user intervention.
