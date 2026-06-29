# Security

Encryption, authentication, and input protection layer.

## Key Components

- **main_security.cpp** - AES-256-CBC encryption, auth token management, rate limiting, and input sanitization
- **crypto.cpp** - Cryptographic primitives and key management
- **encrypted_shards.cpp** - Data-at-rest encryption for sensitive files (memory, schedules, tokens)

## Features

- AES-256-CBC symmetric encryption (pure C++, no external libs)
- Token-based authentication with generation and expiration
- Sliding-window rate limiter (per-user, per-IP)
- Input sanitization: SQL injection, path traversal, XSS, and command injection blocking
