## Summary & Scientific Rationale

Provide a concise technical summary of the proposed contribution, rationale, and mathematical/architectural implications.

## Contribution Category

- [ ] Core algorithmic refinement (ternary logic / cellular apoptosis)
- [ ] Hardware verification / embedded target porting
- [ ] Empirical benchmarking suite & reproducibility evidence
- [ ] Formal documentation & API specification

## Architectural Invariants Verification (Strict)

All contributions MUST strictly satisfy these non-negotiable invariants:
- [ ] **Zero Floating-Point Operations:** Primary inference path (`feed(int16_t)`) contains strictly zero FLOPs.
- [ ] **Zero Dynamic Allocation:** `malloc = 0`, no heap, `new`, `std::vector`, or dynamic memory structures.
- [ ] **Deterministic $\mathcal{O}(1)$ Complexity:** Strictly bounded constant-time execution without data-dependent branching or loops.
- [ ] **Flat Memory Footprint:** Static RAM state does not exceed 84 bytes on 8-bit AVR (`ATmega328P`).
- [ ] **Cross-Platform Silicon Compilability:** Compiles with zero warnings on 8-bit AVR (`avr-gcc`), 32-bit ARM Cortex-M, and ESP32.
- [ ] **Single-Header Self-Sufficiency:** Entire core distribution encapsulated strictly in `src/qsetun.h`.

## Empirical Verification Suite

- [ ] `python benchmarks/run_all_benchmarks.py` passes all 4 evaluation blocks.
- [ ] `python benchmarks/regression_equivalence.py` confirms bit-for-bit backward equivalence.
- [ ] Physical hardware validation on silicon: ______________ (specify target board and clock rate).

## Related Issues

Closes #
