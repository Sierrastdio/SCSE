# SPASIM (version 3.6) C++ porting specification

## Goal and fidelity policy

The first project goal is to port the SPASIM program represented by `xspasim.txt` into C++, preserving its rules and interaction where the printed listing is sufficient to establish them. This is a simulator port, not a StarCraft adaptation. The old terminal interface, global common-memory offsets, and NOS lesson runtime are implementation details to replace; their visible behavior and simulation semantics should be retained when recoverable.

Use three labels while implementing:

- **Known**: directly stated in comments, prompts, data declarations, or unambiguous control/data flow.
- **Reconstructed**: inferred from executable expressions whose old-language operators are legible.
- **Unknown**: depends on missing lessons/runtime or glyphs/formulas damaged by print conversion. Keep these behind named policy functions/configuration and do not silently present guesses as source facts.

## Engine direction

The reusable engine concept is **space simulation and ship-to-ship combat**. SPASIM remains the first ruleset and porting target; engine abstractions should be useful for other games without carrying terminal UI or SPASIM-specific command syntax into the core.

An **entity** is any independently identified thing that exists in the simulated world and may have position, state, ownership or interactions. This includes friendly and hostile ships, bases, torpedoes, and potentially stations, debris, planets, sensors and projectiles. “Entity” describes identity/lifetime, not a requirement that every kind shares identical fields or behavior. Teams/factions describe relationships; they are not entity kinds. A player is a controller/session and may own or command several entities.

### Engine work to prioritize

1. **Unified entity identity and lifecycle**: stable IDs, explicit spawn/despawn, ownership/team, entity categories/components, and no ID aliasing between planets and entities.
2. **Simulation contract**: fixed-step `step(dt)` with deterministic system order; commands are validated and queued/applied at tick boundaries; render/input never mutate simulation state directly.
3. **Space movement**: explicit unit conventions, 3D transforms, velocity/acceleration/turn rate, target-relative navigation, collision radii, and configurable movement profiles. Keep exact SPASIM formulas isolated until decoded.
4. **Combat model**: weapon profiles, acquisition/aiming, projectile/beam travel, range and line-of-fire, hit resolution, shield/hull layers, damage events and destruction. Separate weapon data from resolution algorithms.
5. **Faction/target rules**: allied/enemy relations, friendly fire policy, target filters and ownership changes.
6. **Events and observability**: emit structured events (fired, hit, shield depleted, destroyed, resource transferred) so console, future UI, replay and debugging observe the same core.
7. **Scenario/configuration and persistence**: data-driven entities/weapons/planets, seeded RNG, versioned save state and scenario setup.
8. **Performance only where measured**: current SPASIM scale is at most dozens of active slots, so fixed arrays are simple and fast. Design for interchangeable storage if worlds grow; add spatial partitioning only when profiling shows full scans are costly.

Avoid speculative “universal ECS” machinery at this stage. A small typed model and focused systems are clearer; generalize after a second concrete game/scenario proves which abstractions repeat.

## Program identity and dependencies

- The lesson header identifies the application as SPASIM, version 3.6, by Jim Bowery; description: three dimensional simulation of space.
- `pray` is the player session loop. It calls `move`, optional `setup`, `screen`, `teams`, `planets`, `matt`, then optional console rendering, before processing input.
- `common spasim,duh,1932,checkpt` and `comload nc1,1,1500` indicate shared persistent state and a common-memory layout. In C++, replace this with named typed state and serialization rather than reproducing raw offsets.
- Required external facilities/files include NOS 2.8.7, lesson runtime, `spasimnote`, `spahelp`, `dogfight`, character set `starry`, station/session services and shared common memory. Their implementations are not included in the attachment. The printed listing has a `doggy` jumpout to `dogfight` but no dogfight implementation.
- Startup banner calls this SPASIM version 3.6 and says it is being overhauled. Do not confuse the 2022 print timestamp with program authorship/date.

## World and state model

### Capacities and slots

The declarations use 24 player slots, 4 teams with 6 slots per team, 24 ships, 24 bases, 24 torpedoes, and 4 planets. A per-player object block has three contiguous slots: ship at `sno`, base at `sno+24`, torpedo at `sno+48`. `teams` labels object kinds `0=ship`, `24=base`, `48=torp`; planets are handled separately. C++ should use typed vectors/arrays and stable object identifiers instead of the encoded arithmetic, while preserving capacities unless further evidence changes them.

### Entity fields observed in declarations

- Shared object positions: `xoc/yoc/zoc`; heading trig values `ctht/stht/crot/srot`.
- Ship/base resources and status: `ae` (energy), `mat` (raw material), `dam` (repair/damage state), `skil` (skill), `fld` (field/shield strength), `exist` flags.
- Player/session: team (`type`), handle/name, station, help status, current selected object (`isave`), target/goal, view coordinates, orientation, speed/warp, acceleration, range/magnification, phasor power (`ppow`) and diameter (`pd`).
- Planet state: coordinates `pox/poy/poz`, living-standard `psol`, energy `pae`, raw material `pmat`, population `ppop`, skill `pskil`, revolt timer.
- Relationships and warnings: `ally(team, planet/team-index)`, `warn`, `wb` (base threat warning).
- The index expressions for legacy strings use mixed `nc`/`vc` memory. These are storage encodings, not domain boundaries.

## Session and command flow

1. Startup clears/initializes common state and displays active-user/help counts.
2. Sign-on selects team 1–4, enforces six-player team capacity, requests a pseudonym (max 20 characters), and initializes the player's ship/base/torpedo state. New player coordinates are randomized around the team planet. Reserved names are excluded (`initv` lines 178–211).
3. Player enters recurring `pray` loop. It processes movement and passive world updates, then waits up to 10 units for commands. Timeout/NEXT returns to the simulation loop.
4. Commands are character/menu driven; map the menu to named C++ commands. The 0–35 command table at source lines 268–300 contains turn/view/navigation/system actions and commands for transport, goal, kill, load, repair, field setting, team messages, shooting, phasor, cursor, jumping, range and acceleration modes.
5. Help can leave to `spahelp`; dogfight can leave to missing `dogfight`; disconnect/wrapup recycles player slots and returns carried resources to the planet (`wrapup` lines 1430–1439).

## Simulation systems

### Update ordering

Preserve the visible per-loop order initially: `move` → optional view `setup` → `screen` → `teams` (object projection, visibility and hit checks) → `planets` (planet economics) → `matt` (base resource transfer) → optional `console`. Split this into deterministic C++ systems, but be aware that same-tick ordering affects results.

### Movement and view

- Steering angles approach requested angles by at most `wr` each movement update; headings are wrapped into 0–360 degrees (`move` 421–432).
- Heading trigonometry is recalculated from yaw/pitch-like angles; warp and acceleration affect speed/range and energy cost (443–464).
- Ships/base/torps are updated while active. Ship movement includes a random damage/obstruction check. When energy is depleted, movement destination handling differs (`move` 445–470).
- The view is a 3D perspective projection with camera/view position, normalized view direction, and screen scale (`setup1`, `create` 613–645). `cursor` maps a selected screen point to a world-space direction/heading (761–799).
- Exact legacy operator meanings for `\`, `&`, `/`, overstrike characters and some expression precedence must be recovered from the symbol legend and original runtime semantics before claiming bit/formula fidelity.

### Teams, visibility, collision, and weapons

- `teams` iterates the other teams and each ship/base/torp kind, projects potential targets, performs a view/range check, skips allied phasor hits, and applies target radii/diameter checks (`teams` 510–550).
- Friendly/allied relationship affects warnings and phasor hit eligibility. The target surface check depends on `pd` and `basz`; do not replace with a simple range-only hit test.
- Four named weapons are listed: Aggstroms, Fouriers, Diffractions, Lasers. Source selects a weapon's geometry through lookup tables `ts/ks/rs/as` and base variants `tb/kb/rb/ab`. The lookup payloads are in common memory and absent from the printed listing; precise weapon curves/projectile shapes are therefore unknown.
- `shoot` spends up to `ppow` energy and turns phasor mode on (`shoot` 1546–1552). Actual hit resolution appears in `teams`: incoming power depends on `ppow`, distance and `pd`; shield `fld` is reduced first, and damage `dam` rises when shield energy is insufficient. There is random variation in damage (`teams` 533–547).
- Torpedoes are separate moving entities, can be controlled/jumped to, and can be detonated. Detonation applies distance-decaying field damage around the blast and destroys the projectile (`deton` 1486–1515). Blast/beam rendering is in `explode`/`flashdes`.
- Destruction marks object existence/energy and names the attacker/victim (`destem` 1519–1545); destroyed objects may transfer remaining resources to the planet during cleanup (`wrapup` 1430–1439).
- The `dogfight` exercise is external. It must not be inferred from the in-space phasor implementation.

### Planets and logistics

- Four planets have population, energy reserves, raw material, standard of living, skill and revolt state.
- `planets` updates resource/population/skill/living-standard values based on elapsed time and can trigger revolt/population losses (1270–1292). The visible expressions use exponential-like terms; port formulas only after operator notation is decoded.
- `matt` transfers raw materials from the base to the nearby planet using distance, available energy and skill (`matt` 1294–1299).
- `load` checks proximity and capacity, transfers energy or raw material, and constrains energy transport by ship/base type (`load` 1172–1248).
- `trans` moves energy/material between player objects, with distance-dependent energy cost and resource checks (`trans` 1098–1135).
- `rep` repairs ship/base damage using energy and skill with random rate (`rep` 841–871); `setf` redistributes energy into field strength and adjusts repair state (`setf` 957–966).

## Port architecture proposal

Keep a headless simulation core independent of UI, then add a thin CLI first. Suggested modules:

- `types.hpp`: `Vec3`, `TeamId`, `EntityId`, units and enums.
- `world.hpp/.cpp`: `World`, `Player`, `Ship`, `Base`, `Torpedo`, `Planet`, alliances, deterministic RNG state.
- `commands.hpp/.cpp`: parsed commands and validation/dispatch.
- `systems/movement.cpp`, `combat.cpp`, `economy.cpp`, `logistics.cpp`, `cleanup.cpp`.
- `render/console.cpp`: initial textual view/status; later 3D rendering can replace this without changing rules.
- `save.cpp`: typed persistence and migration/versioning.
- `main.cpp`: initializes scenario and drives fixed simulation steps/input.

Represent each legacy command as a function taking `(World&, PlayerId, CommandArgs)` and returning a result/event. Keep random draws in an injected seeded RNG so scenarios are reproducible. Use a fixed simulation timestep; translate source loop's timed input separately from simulation time.

### Current repository implementation

The structured core currently exists in `src/spasim/`:

- `model.hpp`: `World`, `Team`, `Player`, `Ship`, `Base`, `Torpedo`, `Planet`, IDs, vectors, resources, combat and movement state.
- `world.cpp`: four-team/four-planet initial world, 24 fixed player slots, deterministic random generator, team join/leave and entity lookup.
- `simulation.cpp`: fixed-step dispatch, basic heading/acceleration movement and torpedo integration, plus typed `WorldCommand` application.
- `command.cpp`: console vocabulary/parser, translating strings into compact values before the UI calls the core.
- `console.cpp`: separate terminal sign-on and status/team/planet presentation.
- `main.cpp` and `CMakeLists.txt`: executable entry point and C++20 build.

This is a structured core, not yet a faithful rules port. Placeholder starting resources, simple movement, and inactive economy/hit resolution are explicitly provisional. Replace each only after reconciling source operators and common-memory initial values. Goals are typed as entity or planet targets. The core intentionally has no real-time main loop yet; the application/engine loop should be added only after its simulation and input facilities are complete.

The present implementation stores ships, bases and torpedoes in separate fixed-capacity arrays with shared `EntityId`s. This is a transitional typed design; a true unified entity registry/component layout is an engine improvement to schedule after source-rule recovery. Do not treat an integer `Target::id` as interchangeable across domains without checking `Target::kind`.

The sample console now runs a fixed-step real-time loop while a dedicated input reader feeds a synchronized command queue. Simulation state is only mutated by the simulation thread. `pause`, `resume` and `tick` are available. The core still exposes explicit `step()` and has no wall-clock dependency.

The console `turn x y z` input is interpreted as a desired forward direction vector, not yaw/pitch angles and not a world-space destination. The vector is converted to heading/pitch and the ship slews toward it at its configured turn rate. Status prints the current normalized facing vector and the internal yaw/pitch angles so a slow turn can be distinguished from a failed command.

For debug rendering, `Simulation::writeSnapshot` exports entity/planet state as JSON without adding a graphics dependency to the core. Console command `snapshot` enables 10 Hz writes to `debug/world.json`; `snapshot off` disables them. `tools/render_debug.py` is a standard-library Tkinter renderer with freely orbitable isometric and orthographic plane views, entity markers, ship heading and torpedo velocity vectors, predicted straight paths and ship trails. Right mouse drag controls orbit azimuth/elevation; Shift+right-drag pans the camera across the screen plane.

Current console-playable operations include team sign-on, status/team/planet listings, heading/acceleration, planet goals, target selection, four selectable weapon profiles, forward-arc phasor firing, torpedo launch/impact/detonation, base-range repair, planet loading, nearby entity resource transfer, and alliance toggles. These provide a coherent playable shell, but do not yet reproduce the whole 36-key menu or NOS station/network session behavior.

### Playable implementation versus source fidelity

- **Implemented as source-derived structure**: 4 teams, 6 player slots per team, ship/base/torpedo ownership slots, 4 planets, team names, object/resource concepts, commands present in the printed listing, fixed-step world ownership and a deterministic RNG seed.
- **Implemented provisionally for playability**: scenario planet coordinates/resources, spawn offset, movement acceleration, hull/shield capacities, weapon energy/damage/range values, forward aiming threshold, torpedo velocity/lifetime/splash, repair distance/cost, transfer range, and console real-time period. The listing omits required common-memory constants and/or its number semantics for many of these.
- **Not yet ported**: source-exact NOS operator/runtime semantics, source-exact 3D view projection and wireframe models, passive planet economy and revolt formula, weapon shape/penetration lookup tables, all menu command variants (messages, magnification/cursor/absolute view/backout/help integration), station-signaling concurrency, save/restore, external `spahelp`, `dogfight` and `spasimnote` lessons.

Consequently this is now a runnable prototype of the game loop and main combat/logistics path, not a complete faithful translation. Completing faithful SPASIM requires the missing common-memory initialization and associated lessons or an authoritative readable source listing.

## Build phases and acceptance gates

1. **Notation and source map**: decode operator legend; annotate all runnable units; inventory missing common-memory tables and external lessons. No guessed combat constants.
2. **Headless data/loop port**: typed state, sign-on/team capacities, initialization, fixed-step update, save/load. Validate slot mapping against source comments.
3. **Navigation/view**: steering, warp, acceleration, goal, 3D coordinates and projection.
4. **Combat**: phasor configuration/fire, target checks, shields/damage/destruction, torpedo movement/detonation. Keep uncertain weapon tables configurable and visibly marked.
5. **Economy/logistics**: planet updates, transfer/load, repair and field setting.
6. **Terminal parity**: team/planet/status panels, messages, command mapping; external help/dogfight documented as ports/stubs until source is available.

Do not claim a faithful port until every in-scope unit has an implementation or explicit excluded-dependency disposition and source formulas have been checked against the original notation.

## Uncertainties requiring resolution

- What the legacy operators `\`, `&`, `/`, `.` and superscript/subscript overstrikes mean in each context. The symbol legend says `&` superscript, `\` font/times depending on notation, `.` understrike, etc.; plain text extraction is lossy.
- Values/meaning of shared-memory weapon geometry and damage tables (`ts/ks/rs/as`, `tb/kb/rb/ab`, `pl`, `tp`), initialization state and random seed behavior.
- Whether the included complete lesson archive sections after the listing contain usable raw initialized arrays; printed output includes common dumps but not clearly decodable values.
- NOS clock/time units, concurrency and station signaling semantics, and persistence/error recovery.
- Missing `spahelp`, `dogfight`, `spasimnote`, `starry` charset assets/runtime.
- Some variables have overloaded names and encoded slot offsets. Use source line references in implementation comments until each has a domain-level interpretation.

## Primary source map

Line references refer to numbered source statements printed in the attachment:

| Area | Source lines |
|---|---:|
| State declarations and memory mappings | 11–123 |
| Initialization/sign-on/player spawn | 124–232 |
| Main loop and key map | 234–317 |
| Team roster/status listing | 318–390 |
| Planet initialization and passive updates | 391–413, 1270–1292 |
| Movement, warp, steering, camera | 415–503 |
| Target scan/hit resolution/render selection | 510–580 |
| Projection/drawing | 582–713 |
| Repair, field, navigation and utility commands | 761–999 |
| Shutdown/backout/coordinate controls | 1001–1096 |
| Transport, goal, loading | 1098–1248 |
| Range and planet economy | 1250–1299 |
| Console/status, wrapup and destruction | 1301–1464 |
| Torpedo control/detonation and phasor | 1467–1574 |
| Effects and session cleanup | 1576–1666 |
