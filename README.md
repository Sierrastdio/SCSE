# SCSE

**SCSE** (Space Combat Simulation Engine) is an experimental C++20 project exploring space combat and simulation. It is an early prototype inspired by ideas from [SPASIM](https://github.com/jabowery/spasim), with a small console program for trying out the current simulation. The code and gameplay rules are still evolving.

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
.\build-ucrt\scse.exe
```

```powershell
python tools/render_debug.py --watch
```

CMake copies the Lua rules beside the executable. `lua54.dll` is found through `PATH`; the PowerShell line above adds the UCRT64 runtime directory for that terminal. If CMake cannot find Lua, verify that `lua.h` and `liblua.dll.a` exist at the paths above, then rerun the configure command.

### VS Code CMake Tools

Open this repository folder, run **CMake: Select a Kit**, choose the **GCC UCRT64** kit, then run **CMake: Delete Cache and Reconfigure**. Workspace settings select Ninja and provide the compiler and Lua paths. Build with **CMake: Build**. To launch from VS Code's integrated PowerShell terminal, add `C:\msys64\ucrt64\bin` to `PATH` using the command above first.

For a clean rebuild, remove only the generated `build-ucrt` folder and repeat the configure and build commands. The older `build` folder can remain configured with its previous generator.

The prototype console program loads a Lua scenario and advances the current fixed-step simulation while accepting commands. Team count, player capacity per team, planets, and weapon profiles come from the scenario. `scse_core` is the CMake library containing the current simulation code. The source listing, porting decisions and development notes are recorded in [PORTING_SPEC.md](PORTING_SPEC.md).

## Current console commands

`status`, `teams`, `planets`, `turn <x> <y> <z>`, `accelerate`, `brake`, `cruise`, `throttle <1-4>`, `goal <planet> [orbit-level]`, `orbit <level>`, `class <small|medium|large>`, `select <entity-id>`, `weapon <profile>`, `fire [power]`, `torpedo <energy>`, `detonate`, `repair <amount>`, `load <energy|material> <amount> <planet>`, `transfer <energy|material> <amount> <entity-id>`, `ally <team> <team>`, `snapshot [off]`, `pause`, `resume`, `speed [multiplier]`, and `quit`. Team and weapon ranges are shown by `help` from the loaded scenario.

`turn` takes a **direction vector**, not Euler angles or a world position. For example, `turn 0 1 0` points along +Y, and `turn 0 0 1` points upward along +Z. Scale does not matter; the vector is normalized by the heading conversion. The ship turns toward that direction at its configured turn rate.

`teams` lists entity IDs, teams, positions, and shields for ships and bases. The starter world has one stationary practice ship per team; practice ships can be changed in the Lua scenario. Practice ships have aliases `T<number>` matching their team's number. Attack flow: run `teams`, choose an enemy, use `select 2` (or `select ship-2`, `select ship #2`, or a practice alias such as `select T2`) to aim, then `fire`. Lua calculates the desired aim direction each tick while C++ applies the ship's turn rate. Instant weapons aim directly; finite-speed projectiles can use Lua's intercept calculation. Firing succeeds when the target is in range and within the forward arc. Failed shots do not consume energy.

`goal <planet> [level]` engages navigation and uses a stopping-distance speed profile to brake before the planet's sphere of influence, then starts a Lua-controlled spiral from the ship's actual position toward the requested orbit. It does not reset the ship to level 1 first. `orbit <level>` starts an automatic transfer while already orbiting. The C++ integrator maintains circular motion during transfers and holds ships on their selected orbit radius after arrival. Level 1 is the outer boundary; higher levels are closer to the planet. `class` changes the test ship class. Lua rules in `scripts/game_rules.lua` set orbit spacing, influence radius, transfer behavior, acceleration, braking, speed tiers, and ship-class limits. Initial ship-class limits are small=4, medium=2, and large=1; these are prototype tuning values. Edit the Lua file and restart the program to load changed rules. `turn` rotates the ship's nose without changing inertial velocity; `accelerate` thrusts along the nose up to the selected speed tier, `brake` opposes current velocity, and `cruise` coasts. `throttle 1`–`throttle 4` sets a speed cap from 0.5 to 2.5 times the L1 circular speed. With current planet values, the caps are approximately 0.71, 1.41, 2.12, and 3.54 units/second. The default is stage 3. Goal autopilot uses the selected cap and automatically switches between accelerating, coasting, and braking. At the speed cap, it can still use lateral thrust to steer the velocity direction. `status` distinguishes facing from inertial velocity and reports the active orbit, throttle tier, and orbit radius. The debug renderer draws the gravity boundary and orbit levels.

`speed` reports the current playback multiplier; `speed 1000` runs up to 1000 times faster, and `speed 1` restores real-time playback. Valid values range from 0.1 to 10000. The physics timestep remains fixed, so actual fast-forward speed depends on CPU capacity.

Example: `class small`, `goal 2 4` requests planet 2's innermost orbit; from orbit, `orbit 1` returns to the outer boundary. `class large` restricts the ship to level 1, and deeper requests are rejected with an explanatory message. The current orbit controller uses simplified thrust and circularization rules for gameplay; it is not a high-fidelity Hohmann transfer model.

Weapon numbering follows the source roster: 1 Aggstroms, 2 Fouriers, 3 Diffractions, 4 Lasers. All four profiles currently use provisional values because the original lookup data is missing from the text export.

### Scenario configuration

Edit the `scenario` table in `scripts/game_rules.lua` to change world timing and seed, players per team, player/ship/base/torpedo defaults, team names and home planets, ship classes and spawn/orbit settings, planet coordinates/gravity/orbit ranges/starting values, weapon profiles, practice targets, alliances, and ID starting values. Team, planet, and weapon lists are built from Lua at startup; their IDs must remain zero-based and contiguous.

## Debug renderer

The Python viewer uses Tkinter and needs no pip packages. On Arch Linux, install Python and Tkinter's optional Tk dependency with `sudo pacman -S python tk` ([Arch Python package](https://archlinux.org/packages/core/x86_64/python/) lists `tk` as optional for Tkinter). Run it from a graphical terminal. In one game session, enter `snapshot`; in a second terminal, from the repository directory, run:

```sh
python tools/render_debug.py --watch
```

The game writes `debug/world.json` relative to its working directory. If the viewer is run without a path, it checks the repository's `debug/` and CMake build directories and follows the newest snapshot. Pass a path explicitly when needed, for example `python tools/render_debug.py build/debug/world.json --watch`.

The viewer draws 3D axes, planets, ships, bases, torpedoes, team colors, facing/velocity arrows, predicted straight-line travel and recent ship trails. In isometric view, drag with the **right mouse button** to orbit freely; hold **Shift** while right-dragging to pan the camera across the view. Keys `2`–`4` switch to XY, XZ and YZ orthographic views, and `1` returns to orbit view. Use the mouse wheel or `+`/`-` to zoom. Enter `snapshot off` to stop writing live state.

## Project layout

- `src/engine/`: current world state, typed actions, movement, navigation, combat, economy, and simulation loop.
- `src/console/`: terminal front end, text-command parser, and JSON debug snapshot writer.
- `src/lua_rules/`: `read_values.cpp` converts and validates Lua values, `scenario.cpp` loads scenario data, and `runtime.cpp` owns Lua state and gameplay policies.
- `scripts/game_rules.lua`: editable world scenario and gameplay rules.

## Core boundary

The current engine code can be used by another front end through `engine/engine.hpp` and `engine/world.hpp`: construct a `Simulation`, issue `WorldCommand` actions through `Simulation::execute`, and advance it with `Simulation::step`. The terminal application, text parser, and debug snapshot format live in `console/`. These interfaces may change as the project develops.

The current movement, initial resource values and weapon profiles are provisional because legacy source notation and shared initial data are not recoverable from the listing alone. Planet growth/revolt is still inactive. See `PORTING_SPEC.md` for fidelity status.

To build only the simulation library without the sample app, configure with `-DSCSE_BUILD_CONSOLE=OFF`.
