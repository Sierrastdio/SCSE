# NEONspasim

A structured C++ port of SPASIM, starting with a headless simulation core and a small terminal interface.

## Build

Requires CMake 3.20+ and a C++20 compiler.

```powershell
cmake -S . -B build
cmake --build build
./build/neonspasim.exe
```

The initial executable creates a four-team SPASIM scenario and accepts a small set of inspection and navigation commands. The source listing and porting decisions are recorded in [PORTING_SPEC.md](PORTING_SPEC.md).

## Project layout

- `src/spasim/model.hpp`: simulation entities and value types.
- `src/spasim/world.*`: world ownership, identifiers, and scenario initialization.
- `src/spasim/simulation.*`: fixed-step simulation and command effects.
- `src/spasim/command.*`: terminal command parsing and validation.
- `src/spasim/console.*`: text input/output only; independent of simulation rules.
