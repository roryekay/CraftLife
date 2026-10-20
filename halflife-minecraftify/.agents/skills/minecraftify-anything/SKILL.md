---
name: minecraftify-anything
description: Build or extend a real Minecraft passthrough integration for an owned host game using the MinecraftifyAnything template, keeping shared rendering and guest code separate from game adapters. Use for Minecraft-in-another-game ports, not ordinary Minecraft mods or visual mockups.
---

Use this repository as the reusable foundation instead of copying a finished game's mod and stripping it down again. The deliverable is a tested integration with the requested host, not an overlay-only substitute for requested gameplay.

Locate the template or generated port root: it contains `guest/`, `native/`, `adapters/` and `docs/PORTING.md`. If the skill was installed globally, locate the user's clone first; do not silently download an unrelated fork. Read [PORTING.md](../../../docs/PORTING.md) for implementation boundaries and [PROTOCOL.md](../../../docs/PROTOCOL.md) before changing either side of the link. Resolve those documents from the located repository root if the skill has been copied elsewhere.

For a new game, use `python tools/new-port.py <game-slug> <empty-destination>` from the template. Reuse an existing suitable generated port when continuing work. Keep the user's other working game mods intact. Record the engine, exact supported build, graphics API, mod loader, coordinate conversion and requested capabilities in `adapters/<game>/port.json`.

Search GitHub for reusable implementations before substantial new code. Prefer a suitable established project around500+stars, inspect the relevant source/build/dependencies/license/release provenance, and pin the reviewed version. Document why any smaller project is a necessary exception. Never replace the user's requested behavior with a merely similar demo.

Build and test the shared guest/renderer first using the README commands and mock host. Then implement the host adapter in `adapters/<game>/`: camera/state and coordinate conversion, input/focus, collision, combat, health and item effects as requested. Choose the language/loader appropriate to the target. `adapters/stub/adapter.hpp` is an inactive interface example, not a working host hook.

Keep game hashes, offsets, native signatures, thread-sensitive calls, gameplay balance, bosses, engine dependencies and installation logic outside the shared core. Add shared event producers only for semantics that are genuinely independent of a game. Review and validate message changes on both sides. Convert units once at the adapter boundary; preserve image/pose ownership through the shared-memory sequence protocol.

An overlay proves only rendering. Verify each requested native interaction through observable state: block placement/standing/jump/mining, real enemy damage, health synchronization/healing, actual item consumption/effects/expiry, projectile impacts and scene/reconnect behavior as applicable. Queue native operations on the required game thread. Mark incomplete capabilities explicitly and report unverified behavior without claiming success.

Use isolated Minecraft worlds/profiles and supported single-player modding paths. Do not bypass ownership/authentication, anti-cheat or OS security. Do not copy purchased game assets, accounts, saves or credentials into source/packages. Build support must fail closed for unknown game versions when using exact-build hooks.

Finish with appropriate builds/tests, a clean-install path, and a concise tested/remaining report. Use [TUTORIAL.md](../../../docs/TUTORIAL.md) only when preparing a tutorial. Upload/publish/change repository access only when the user has authorized it; this skill itself grants no external-action authorization.
