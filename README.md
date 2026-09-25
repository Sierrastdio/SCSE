# NEONspasim

A reusable C++20 engine in development around **space simulation and ship-to-ship combat**, with SPASIM as its first porting target and a thin console driver. An entity means any independently identified object in the world: friendly or hostile ships, bases, torpedoes, and later potentially stations, projectiles, debris, or planets. Players are controllers; teams describe relationships among entities.

## Build

Requires CMake 3.20+ and a C++20 compiler.

```powershell
cmake -S . -B build
cmake --build build
./build/neonspasim.exe
```

The initial executable creates a four-team SPASIM scenario and advances its fixed-step simulation automatically while accepting console commands. `spasim_core` is a separate CMake library. The source listing, porting decisions and engine priorities are recorded in [PORTING_SPEC.md](PORTING_SPEC.md).

## Current console commands

`status`, `teams`, `planets`, `turn <x> <y> <z>`, `accelerate`, `brake`, `cruise`, `goal <planet 1-4>`, `select <entity-id>`, `weapon <1-4>`, `fire [power]`, `torpedo <energy>`, `detonate`, `repair <amount>`, `load <energy|material> <amount> <planet 1-4>`, `transfer <energy|material> <amount> <entity-id>`, `ally <team 1-4> <team 1-4>`, `snapshot [off]`, `pause`, `resume`, and `quit`.

`turn` takes a **direction vector**, not Euler angles or a world position. For example, `turn 0 1 0` points along +Y, and `turn 0 0 1` points upward along +Z. Scale does not matter; the vector is normalized by the heading conversion. The ship turns toward that direction at its configured turn rate.

`teams` lists entity IDs, teams, positions, and shields for ships and bases. The world starts with one stationary practice ship per team; each player therefore has three hostile default targets. Practice ships have aliases `T1`–`T4` matching their team's number. Attack flow: run `teams`, choose an enemy, use `select 2` (or `select ship-2`, `select ship #2`, or its practice alias such as `select T2`) to aim, then `fire`. Aiming turns at the ship's turn rate; firing succeeds when the target is in range and within the forward arc. Failed shots do not consume energy.

`goal <planet 1-4>` steers toward a planet; use `accelerate` to travel. `turn` rotates the ship's nose but does not instantly rotate its inertial velocity; `accelerate` thrusts along the nose, `brake` opposes current velocity, and `cruise` coasts. Ships and torpedoes feel gravity from every planet. Reaching a goal planet's parking-orbit boundary triggers an orbital insertion: the ship is circularized at that radius, its goal clears, and gravity maintains its orbit. The current planetary radii, gravity strengths, and orbit altitudes are tuning values for the prototype, not recovered SPASIM constants. `status` distinguishes facing from inertial velocity and reports the active orbit and engine mode. The debug renderer draws each planet's parking-orbit ring, ship facing direction, and velocity arrow.

Weapon numbering follows the source roster: 1 Aggstroms, 2 Fouriers, 3 Diffractions, 4 Lasers. All four profiles currently use provisional values because the original lookup data is missing from the text export.

## Debug renderer

The Python viewer uses only the standard library (Tkinter); it needs no game engine or pip packages. In one game session, enter `snapshot` to enable updates to `debug/world.json`. In a second terminal run:

```powershell
python tools/render_debug.py --watch
```

The viewer draws 3D axes, planets, ships, bases, torpedoes, team colors, facing/velocity arrows, predicted straight-line travel and recent ship trails. In isometric view, drag with the **right mouse button** to orbit freely; hold **Shift** while right-dragging to pan the camera across the view. Keys `2`–`4` switch to XY, XZ and YZ orthographic views, and `1` returns to orbit view. Use the mouse wheel or `+`/`-` to zoom. Enter `snapshot off` to stop writing live state.

## Project layout

- `src/spasim/model.hpp`: simulation entities and value types.
- `src/spasim/world.*`: world ownership, identifiers, and scenario initialization.
- `src/spasim/simulation.cpp`: fixed-step orchestration and typed command dispatch.
- `src/spasim/simulation_physics.cpp`: inertial movement, gravity, surface contact, and orbit insertion.
- `src/spasim/simulation_navigation.cpp`: heading, acceleration, goal, and target selection commands.
- `src/spasim/simulation_weapons.cpp`: phasors, torpedoes, impacts, and damage.
- `src/spasim/simulation_logistics.cpp`: repair, transfers, planetary loading, and alliances.
- `src/spasim/simulation_snapshot.cpp`: debug JSON serialization.
- `src/spasim/simulation_detail.hpp`: internal contracts shared between simulation systems.
- `src/spasim/command.*`: console text parsing into a compact command value.
- `src/spasim/console.*`: text input/output only; independent of simulation rules.

## Core boundary

An alternate front end should include `spasim/simulation.hpp` and `spasim/world.hpp`, construct a `Simulation`, translate user actions to a `WorldCommand`, call `Simulation::execute`, and advance the world by calling `Simulation::step`. The sample console has a fixed-step wall-clock loop; the core itself deliberately has no wall-clock loop, rendering dependency, or input polling.

The current movement, initial resource values and weapon profiles are provisional because legacy source notation and shared initial data are not recoverable from the listing alone. Planet growth/revolt is still inactive. See `PORTING_SPEC.md` for fidelity status.

To build only the reusable core without the sample app, configure with `-DSPASIM_BUILD_CONSOLE=OFF`.

```powershell
g++ -std=c++20 -Wall -Wextra -Wpedantic -pthread -Isrc `
  src/main.cpp src/spasim/command.cpp src/spasim/console.cpp `
  src/spasim/simulation.cpp src/spasim/simulation_logistics.cpp `
  src/spasim/simulation_navigation.cpp src/spasim/simulation_physics.cpp `
  src/spasim/simulation_snapshot.cpp src/spasim/simulation_utilities.cpp `
  src/spasim/simulation_weapons.cpp src/spasim/world.cpp `
  -o out/neonspasim.exe

```

```powershell
python tools/render_debug.py --watch
```
