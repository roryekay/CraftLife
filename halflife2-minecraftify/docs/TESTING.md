# Initial template validation

Checked2026-10-17 on Windows x64 with Minecraft26.3, Java25, Fabric Loader0.19.5, Fabric API0.161.0+26.3, MSVC2022 and the pinned Gradle toolchain.

| Check | Result |
| --- | --- |
|Minecraft guest compilation/JAR build|PASS|
|Native renderer add-on and probe compilation|PASS|
|Frame descriptor/header bounds, incomplete writes, invalid floats/flags|PASS via CTest|
|Actual cross-process native frame reader|PASS:79 coherent synthetic frames; full three-layer layout|
|Fresh isolated Fabric development client|PASS:52mods load; new Creative void sandbox opens|
|Sandbox safety before host attaches|PASS after correcting idle gravity; stays parked instead of falling into the void|
|Real guest → native shared-memory capture|PASS:118 completed1920×1080colour/depth/HUD captures in2seconds|
|WebSocket v1 attach/single-owner behavior|PASS:second host refused while first owns input/camera|
|Malformed pose and legacy command handling|PASS:invalid pose refused; arbitrary command type unsupported|
|Actual hotbar/held attack input|PASS:guest selected requested slot and reported actual Minecraft attack key state|
|Camera lease expiry/input release|PASS:held action released and new use action ignored after expiry|
|Disconnect/reconnect|PASS:second connection acquired ownership and resumed live state without restarting Minecraft|
|Fresh-port generator|PASS:source/tooling/skills copied; caches/builds/worlds excluded; occupied destination refused|
|Game-specific extraction boundary|PASS:runtime source scan excludes old host identifiers/hooks|
|Codex/Claude skill validation|PASS:both skill-creator format checks; byte-identical bodies and source-copy checks|

Run `node tests/live_guest.mjs` with the isolated developer world open and no mock host connected to repeat live transport/input/probe checks. The test changes the sandbox's selected slot and holds/releases a short attack, but does not spawn enemies, alter a host game or write user accounts. `tests/synthetic_frames.py` uses the default mapping name; run it with the guest and host stopped, before a live-capture test.

## Limits of this acceptance

The new neutral ReShade add-on compiles, but has not yet been loaded into an actual new host game's graphics runtime. The small shader is derived from the working prior compositor, with neutral naming; shader compilation, depth alignment, colour-space behavior, device reset and performance remain per-port acceptance work. Do not present this as a verified playable port into an arbitrary game. Synchronous uploads prioritize coherent baseline behavior over performance.

This template intentionally has no native collision, damage, health/healing, projectile, potion-effect, shield/totem, mission or boss implementation. Generic action and block-change observations are extension seams, not completed native effects. The C++ BlankAdapter stays inactive. Full inventory cursor/text forwarding, block replay after reconnect, camera reprojection and complex host depth conventions also require implementation where needed.

Skills were format-validated and their referenced tools/docs were exercised. They were not independently evaluated through complete new-game ports in Codex and Claude Code. Only this Windows development machine was used; another PC/GPU/platform is unverified. No existing game project was migrated or replaced.
