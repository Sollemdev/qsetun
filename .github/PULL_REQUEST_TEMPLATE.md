## Description

Brief description of the changes.

## Type of Change

- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Refactor (no functional change)

## Core Invariants Checklist

- [ ] `feed(int16_t)` contains **zero** floating-point operations
- [ ] No `malloc`, `new`, `std::vector`, or dynamic allocation added
- [ ] Execution remains **O(1)** — no data-dependent loops in `feed()`
- [ ] Static state does not exceed **84 bytes**
- [ ] Compiles on **8-bit AVR** (ATmega328P) with `avr-gcc`
- [ ] Library remains a **single header** (`src/qsetun.h`)

## Testing

- [ ] `python benchmarks/run_all_benchmarks.py` passes
- [ ] Tested on hardware: ______________ (specify board)
- [ ] New example added (if applicable)

## Related Issues

Closes #
