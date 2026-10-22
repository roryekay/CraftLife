# Windows installer validation — 2026-10-21

October 22 installer update: `0.3.0-test.2` supports automatic HL1-only,
HL2-only and both-game installation. Fixture checks cover each combination,
missing-game launch refusal, and adding the second game while preserving
progress. The guest compiles with unavailable travel tabs and keyboard group
switching disabled using the installed profile's game list. No new live
campaign or single-game menu visual test is claimed for this packaging update.

Passed using the packaged Python and Node runtimes on this Windows computer:

- SHA-256 package manifest verification; binaries match the currently installed working CraftLife build.
- Fresh installation with spaces in its path and owned Steam copies of both games.
- Existing unrelated Launcher profiles and top-level settings preserved; exact settings backup saved locally.
- Isolated Fabric profiles for both games; official public Minecraft version metadata included.
- All ten locally reconstructed voice overrides match the approved installed WAV hashes byte-for-byte.
- Campaign and shortcut ending maps match the working prepared BSP hashes byte-for-byte; original Steam assets remain unchanged.
- Repeat installation preserves the player's isolated world, options, ReShade settings and unrelated profile.
- Unsupported executable and unrelated nonempty target folder refused before installation writes.
- Modified installed guest JAR refused before native launch.
- Half-Life 1 launched from the newly installed runtime, attached to the existing authenticated Minecraft session and visibly rendered the real inventory/held items.
- Switched the owned test session to Half-Life 2 through the production process-identity checks. The new runtime compiled the shader and attached successfully.
- Half-Life 2 ending ran from the newly installed runtime; original G-Man was visible on Minecraft grass, with no broad dark halo while walking beside the purple portal. Native logs confirmed portal crossing and both authored phases.
- Live handoff observer passed: selected item/offhand counts preserved; final saved Overworld had normal physics and Minecraft camera ownership. Native session quit after the acknowledged handoff.

The current gameplay build also previously passed the four native contract tests and wire validation. Full campaigns were not replayed for this packaging change. A completely separate Windows machine and a newly authenticated Minecraft account were not tested. The live installer smoke test reused the existing authenticated Minecraft process; the new official Launcher profile definitions and their on-disk mods were verified without altering the user's current Launcher settings.

Known existing limits: experimental item/geometry parity, native movement fallback for vehicles/ladders/scripts, incomplete depth calibration, and keyboard travel being more reliable than short mouse clicks. Minecraft weather may remain visible during native cinematic scenes. No campaign-completion guarantee is made.

Build from source: run `installer/build-package.py` with the prepared local native/guest outputs and CPython 3.12. It uses the existing pinned Node executable (override `CRAFTLIFE_NODE`) and Python runtime/standard library, then fetches only official public Minecraft metadata. `CRAFTLIFE_PACKAGE_DIR` chooses a fresh output directory. Never use a folder containing saves or account data as a build input. `installer/test-installer.py` exercises the extracted package against isolated fixtures and owned Steam installations.
