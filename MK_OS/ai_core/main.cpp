#ifndef MK_MAIN_CPP
#define MK_MAIN_CPP

#include <iostream>
#include <string>
#include <vector>

// Core AI modules
#include "memory.cpp"
#include "reasoning.cpp"
#include "language.cpp"
#include "execution.cpp"
#include "feedback.cpp"
#include "coding.cpp"
#include "writing.cpp"
#include "nexus.cpp"      // Nexus optimization subsystem included

// Dataset modules
#include "../datasets/loader.cpp"
#include "../datasets/parser.cpp"
#include "../datasets/db.cpp"
#include "../datasets/compressor.cpp"

// Module-only file: aggregates AI core includes for standalone compilation if needed.
// No standalone entry point - mk_entry.cpp is the unified boot sequence.

#endif // MK_MAIN_CPP