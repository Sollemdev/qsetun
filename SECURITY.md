# Security Policy

## Supported Versions

| Version | Supported |
|:--|:--|
| 2.0.x | ✅ Active |
| 1.0.x | ❌ End of life |

## Reporting a Vulnerability

Q-SETUN is a signal processing library for embedded systems. If you discover a vulnerability (e.g., integer overflow leading to undefined behavior, buffer overrun in the ring buffer), please:

1. **Do not** open a public issue.
2. Email the maintainers directly at the address listed in `library.properties`.
3. Include: affected version, reproduction steps, and potential impact.

We will respond within 7 days and issue a patch release if confirmed.

## Scope

- Integer overflow / underflow in Q8 arithmetic
- Buffer overrun in `_trit_ring[32]`
- State corruption via malformed `calibrate()` callback
- Undefined behavior on edge-case inputs (e.g., `INT16_MIN`)
