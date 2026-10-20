# Minecraftify Anything — fresh Half-Life ports

This generated project now contains the shared guest/relay and the fresh GoldSrc
adapter; the Source plugin is in sibling `halflife2-minecraftify`. The old
HalfCraft download is untouched and not used. Read [STATUS.md](STATUS.md) for
observed gameplay checks, limitations and isolated setup. The root
`HALFLIFE_MINECRAFTIFY_HANDOFF.md` explains the installed profiles and Play shortcuts.
Read [VALVE_REUSE_REVIEW.md](VALVE_REUSE_REVIEW.md) for pins/licenses/review limits.

These are development integrations, not completed campaign-tested releases.

## Inherited foundation documentation

A clean starting point for putting **real Minecraft into another game**. Run Minecraft alongside the host game, synchronize its camera, and composite Minecraft's blocks, hands and UI into the host image.

This is a developer template. It does not automatically support every game. Each game still needs its own camera, input and gameplay adapter. Existing working game projects are separate and unchanged.

| Shared foundation | Add per game |
| --- | --- |
| Fabric Minecraft guest, transparent world/HUD capture | Engine/mod-loader integration and supported build detection |
| Local WebSocket camera/input/event protocol | Coordinate conversion, native camera and menu state |
| Coherent shared-memory image transfer | Input collection, native weapon conflicts and menu focus |
| Neutral ReShade renderer and shader | Native collision, damage, healing, projectiles and teleporting |
| Mock host, frame probe and port generator | Measured depth calibration and performance tuning |

No existing game's offsets, boss menus, controller injection, starter kit, damage multipliers, vehicle systems or native engine dependencies are included.

## Start a new game

Clone this repository, open it in Codex or Claude Code, and use the included skill:

```text
Codex: $minecraftify-anything Put Minecraft into <game>. Start a new isolated port.
Claude Code: /minecraftify-anything Put Minecraft into <game>. Start a new isolated port.
```

The skills live in `.agents/skills/minecraftify-anything/` and `.claude/skills/minecraftify-anything/`. They share the same maintained workflow. See [skill setup](docs/SKILLS.md).

Create an isolated source checkout without copying an older game's mod:

```powershell
python tools/new-port.py my-game ../my-game-craft
```

The generator copies source and pinned build tooling, creates `adapters/my-game/port.json`, and refuses to overwrite a nonempty destination. All capabilities start **unimplemented**. Implement the game's bridge under its adapter folder, using [PORTING.md](docs/PORTING.md) and [PROTOCOL.md](docs/PROTOCOL.md).

A prebuilt [Windows developer kit](https://github.com/roryekay/MinecraftifyAnything/releases/tag/v0.1.0) includes this source, both skills, and the compiled guest/add-on/probe under `bin/`. It is a development starter, not a universal game installer.

## Build and test the blank canvas

Windows x64, Java 25, Python 3, Node 22+, Visual Studio 2022 C++ workload/Windows SDK, CMake 3.20+. Own Minecraft Java Edition. No host game or administrator access is required for the core tests.

```powershell
guest/gradlew.bat -p guest build
cmake -S . -B local/build -A x64
cmake --build local/build --config Release
ctest --test-dir local/build -C Release --output-on-failure
python tools/verify-template.py
python tests/synthetic_frames.py # Run with the guest stopped; it owns the test mapping.
```

Outputs: `guest/build/libs/minecraftify-guest-0.1.0.jar`, `local/build/Release/MinecraftifyRenderer.addon64`, and `frame_probe.exe`. Build downloads are Minecraft/Fabric/toolchain dependencies; no game files are distributed here.

For a game-free development smoke test:

```powershell
guest/gradlew.bat -p guest runClient
# After the isolated Creative void world appears, in another terminal:
node tools/mock-host.mjs 30
# While the mock host is attached, in another terminal:
local/build/Release/frame_probe.exe 10
```

Fabric's development client uses `guest/run/`, excluded from Git. It is not a launcher/account replacement. The mock host supplies a fixed camera and prints guest events; it does not pretend to implement a game's combat or physics. See [the testing record](docs/TESTING.md).

The renderer requires a compatible ReShade add-on runtime in a future host. Install/load ReShade using that game's reviewed modding route; this template does not install it globally or modify a game folder. Put `Minecraftify.fx` and the supplied `ReShade.fxh` in its shader search directory and enable the **Minecraftify** technique. Depth occlusion starts disabled until the adapter's depth convention and units are verified. The renderer uses conservative synchronous uploads, so performance must be measured in each port.

## Tutorial and provenance

[TUTORIAL.md](docs/TUTORIAL.md) gives the tutorial sequence and recording checkpoints. [UPSTREAM_REVIEW.md](UPSTREAM_REVIEW.md) records the reused MIT passthrough foundation, pinned dependencies and review limits. Existing game-specific implementations are references only; their native code is not bundled into this starter.

The default local connection is `127.0.0.1:25610`, mapping `Local\MinecraftifyAnythingFrame`. Only one active adapter owns the guest at a time. Disconnects/timeouts release remote item inputs; stale exported images stop being composited. Run separate ports in separate sessions, or change both the guest mapping and the native reader name together.
