# NEONspasim

A reusable C++20 engine in development around **space simulation and ship-to-ship combat**, with SPASIM as its first porting target and a thin console driver. An entity means any independently identified object in the world: friendly or hostile ships, bases, torpedoes, and later potentially stations, projectiles, debris, or planets. Players are controllers; teams describe relationships among entities.

## Build on Windows with MSYS2 UCRT64

The project links to Lua 5.4. In an **MSYS2 UCRT64** terminal, install the compiler, CMake, Ninja, and Lua development files:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-lua
```

Open PowerShell at the repository root and configure in a new build directory. A directory configured with NMake cannot be reused with Ninja because CMake generators are fixed at configuration time.

```powershell
cmake -S . -B build-ucrt -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/ninja.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe `
  -DLUA_INCLUDE_DIR=C:/msys64/ucrt64/include `
  -DLUA_LIBRARY=C:/msys64/ucrt64/lib/liblua.dll.a
cmake --build build-ucrt --parallel
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
.\build-ucrt\neonspasim.exe
```

```powershell
python tools/render_debug.py --watch
```

CMake copies the Lua rules beside the executable. `lua54.dll` is found through `PATH`; the PowerShell line above adds the UCRT64 runtime directory for that terminal. If CMake cannot find Lua, verify that `lua.h` and `liblua.dll.a` exist at the paths above, then rerun the configure command.

### VS Code CMake Tools

Open this repository folder, run **CMake: Select a Kit**, choose the **GCC UCRT64** kit, then run **CMake: Delete Cache and Reconfigure**. Workspace settings select Ninja and provide the compiler and Lua paths. Build with **CMake: Build**. To launch from VS Code's integrated PowerShell terminal, add `C:\msys64\ucrt64\bin` to `PATH` using the command above first.

For a clean rebuild, remove only the generated `build-ucrt` folder and repeat the configure and build commands. The older `build` folder can remain configured with its previous generator.

The initial executable creates a four-team SPASIM scenario and advances its fixed-step simulation automatically while accepting console commands. `spasim_core` is a separate CMake library. The source listing, porting decisions and engine priorities are recorded in [PORTING_SPEC.md](PORTING_SPEC.md).

## Current console commands

`status`, `teams`, `planets`, `turn <x> <y> <z>`, `accelerate`, `brake`, `cruise`, `throttle <1-4>`, `goal <planet> [orbit-level]`, `orbit <level>`, `class <small|medium|large>`, `select <entity-id>`, `weapon <1-4>`, `fire [power]`, `torpedo <energy>`, `detonate`, `repair <amount>`, `load <energy|material> <amount> <planet>`, `transfer <energy|material> <amount> <entity-id>`, `ally <team 1-4> <team 1-4>`, `snapshot [off]`, `pause`, `resume`, `speed [multiplier]`, and `quit`.

`turn` takes a **direction vector**, not Euler angles or a world position. For example, `turn 0 1 0` points along +Y, and `turn 0 0 1` points upward along +Z. Scale does not matter; the vector is normalized by the heading conversion. The ship turns toward that direction at its configured turn rate.

`teams` lists entity IDs, teams, positions, and shields for ships and bases. The world starts with one stationary practice ship per team; each player therefore has three hostile default targets. Practice ships have aliases `T1`–`T4` matching their team's number. Attack flow: run `teams`, choose an enemy, use `select 2` (or `select ship-2`, `select ship #2`, or its practice alias such as `select T2`) to aim, then `fire`. Lua calculates the desired aim direction each tick while C++ applies the ship's turn rate. Instant weapons aim directly; finite-speed projectiles can use Lua's intercept calculation. Firing succeeds when the target is in range and within the forward arc. Failed shots do not consume energy.

`goal <planet> [level]` engages navigation and uses a stopping-distance speed profile to brake before the planet's sphere of influence, then starts a Lua-controlled spiral from the ship's actual position toward the requested orbit. It does not reset the ship to level 1 first. `orbit <level>` starts an automatic transfer while already orbiting. The C++ integrator maintains circular motion during transfers and holds ships on their selected orbit radius after arrival. Level 1 is the outer boundary; higher levels are closer to the planet. `class` changes the test ship class. Lua rules in `scripts/orbit_rules.lua` set orbit spacing, influence radius, transfer behavior, acceleration, braking, speed tiers, and ship-class limits. Initial ship-class limits are small=4, medium=2, and large=1; these are prototype tuning values. Edit the Lua file and restart the program to load changed rules. `turn` rotates the ship's nose without changing inertial velocity; `accelerate` thrusts along the nose up to the selected speed tier, `brake` opposes current velocity, and `cruise` coasts. `throttle 1`–`throttle 4` sets a speed cap from 0.5 to 2.5 times the L1 circular speed. With current planet values, the caps are approximately 0.71, 1.41, 2.12, and 3.54 units/second. The default is stage 3. Goal autopilot uses the selected cap and automatically switches between accelerating, coasting, and braking. At the speed cap, it can still use lateral thrust to steer the velocity direction. `status` distinguishes facing from inertial velocity and reports the active orbit, throttle tier, and orbit radius. The debug renderer draws the gravity boundary and orbit levels.

`speed` reports the current playback multiplier; `speed 1000` runs up to 1000 times faster, and `speed 1` restores real-time playback. Valid values range from 0.1 to 10000. The physics timestep remains fixed, so actual fast-forward speed depends on CPU capacity.

Example: `class small`, `goal 2 4` requests planet 2's innermost orbit; from orbit, `orbit 1` returns to the outer boundary. `class large` restricts the ship to level 1, and deeper requests are rejected with an explanatory message. The current orbit controller uses simplified thrust and circularization rules for gameplay; it is not a high-fidelity Hohmann transfer model.

Weapon numbering follows the source roster: 1 Aggstroms, 2 Fouriers, 3 Diffractions, 4 Lasers. All four profiles currently use provisional values because the original lookup data is missing from the text export.

### Scenario configuration

Edit the `scenario` table in `scripts/orbit_rules.lua` to change world timing and seed, player/ship/base/torpedo defaults, team names and home planets, ship classes and spawn/orbit settings, planet coordinates/gravity/orbit ranges/starting values, weapon profiles, practice targets, alliances, and ID starting values. Planet IDs must remain zero-based and contiguous; the Lua loader builds the planet list from this table at startup. Team and weapon IDs remain fixed slots.

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
- `src/spasim/lua_rules.*` and `scripts/orbit_rules.lua`: Lua 5.4 rule loading and orbit/class tuning.
- `src/spasim/simulation_detail.hpp`: internal contracts shared between simulation systems.
- `src/spasim/command.*`: console text parsing into a compact command value.
- `src/spasim/console.*`: text input/output only; independent of simulation rules.

## Core boundary

An alternate front end should include `spasim/simulation.hpp` and `spasim/world.hpp`, construct a `Simulation`, translate user actions to a `WorldCommand`, call `Simulation::execute`, and advance the world by calling `Simulation::step`. The sample console has a fixed-step wall-clock loop; the core itself deliberately has no wall-clock loop, rendering dependency, or input polling.

The current movement, initial resource values and weapon profiles are provisional because legacy source notation and shared initial data are not recoverable from the listing alone. Planet growth/revolt is still inactive. See `PORTING_SPEC.md` for fidelity status.

To build only the reusable core without the sample app, configure with `-DSPASIM_BUILD_CONSOLE=OFF`.
