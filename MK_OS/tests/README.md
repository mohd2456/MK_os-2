# Tests

Automated test suite for MK OS. Uses a custom lightweight test framework.

## Running Tests

```bash
make test
```

## Structure

- **test_main.cpp** - All test functions and the test runner entry point
- Uses `TEST_ASSERT(condition)` macros and `RUN_TEST(func)` pattern
- Tests include modules directly via `#include` (single compilation unit)

## Coverage

Tests cover: knowledge graph queries, reasoning inference, router accuracy,
deep reasoner, composer output, vector search, embeddings, math solver,
correction engine, security modules, and more.
