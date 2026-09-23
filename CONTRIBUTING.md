# Guidelines for Scientific Contributions & Empirical Peer Review

Contributions to Q-SETUN are welcome from embedded systems engineers, microarchitecture researchers, and neuromorphic computing practitioners.
To preserve the mathematical and architectural integrity of the core, all contributions undergo rigorous verification against verified silicon invariants.

---

## Academic & Repository Resources

- **Primary Repository:** [github.com/Sollemdev/qsetun](https://github.com/Sollemdev/qsetun)
- **Permanent Research Passport (Zenodo DOI):** [10.5281/zenodo.22813548](https://doi.org/10.5281/zenodo.22813548)
- **License:** GNU General Public License v3.0 (GPL-3.0)
- **Issue Tracking & RFCs:** [GitHub Issues](https://github.com/Sollemdev/qsetun/issues)

---

## How to Contribute

### Reporting Bugs

Open an issue with:
1. Board/platform (e.g. ESP32-S3, Arduino Uno, STM32F411)
2. Arduino IDE / PlatformIO version
3. Minimal sketch that reproduces the problem
4. Expected vs. actual behavior

### Suggesting Features

Open an issue with the `enhancement` label. Describe the use case and sensor type.

### Pull Requests

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-improvement`
3. Make changes — **keep the core invariants** (see below)
4. Run the benchmark suite: `python benchmarks/run_all_benchmarks.py`
5. Open a PR against `main`

---

## Core Invariants (Do Not Break)

These architectural guarantees must hold for **every** commit:

| Invariant | Requirement |
|:--|:--|
| **Zero-FLOP inference** | `feed(int16_t)` must contain zero floating-point operations |
| **Zero heap** | No `malloc`, `new`, `std::vector`, or any dynamic allocation |
| **O(1) execution** | No loops, recursion, or data-dependent branching in `feed()` |
| **Single header** | The entire library is `src/qsetun.h` — no `.cpp` files |
| **84-byte footprint** | Static state must not exceed 84 bytes |
| **Cross-platform** | Must compile on 8-bit AVR (ATmega328P), 32-bit ARM, and ESP32 |

### Verification

Before submitting, verify with:

```bash
# Run the full benchmark + correctness suite
python benchmarks/run_all_benchmarks.py

# Check AVR binary size (requires avr-gcc)
avr-gcc -Os -mmcu=atmega328p -o qsetun_test.elf your_test.cpp
avr-size -C --mcu=atmega328p qsetun_test.elf
```

---

## Code Style

- C++11 standard (no C++14/17 features for AVR compatibility)
- 4-space indentation
- Prefix private members with `_` (e.g. `_baseline_ema`)
- Document public methods with Javadoc-style `/** */` comments
- Keep `qsetun.h` self-contained — no external dependencies beyond `<stdint.h>` and `<stdbool.h>`

---

## Adding Examples

Place new examples in `examples/NN_Example_Name/NN_Example_Name.ino` following the existing pattern. Include a comment header explaining hardware requirements and what the example demonstrates.

---

## License

By contributing, you agree that your contributions will be licensed under the [GPL-3.0 License](LICENSE).
