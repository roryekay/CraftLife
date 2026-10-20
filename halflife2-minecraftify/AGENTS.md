# MinecraftifyAnything repository guidance

Use the `minecraftify-anything` skill for new host-game ports. This is a game-neutral foundation; keep the existing finished game projects outside this repository intact.

- Shared guest/export/transport/render code belongs in `guest/`, `native/` and `shaders/`. Keep executable hashes/offsets, native engine APIs, coordinate transforms, combat rules and installers under `adapters/<game>/` in a generated port.
- Read `docs/PORTING.md` and `docs/PROTOCOL.md`. Use `tools/new-port.py` for fresh ports. Do not start by copying an old game directory or include its worlds/accounts/binaries.
- Search GitHub before substantial new functionality. Prefer appropriate established implementations, inspect relevant code/license/build/dependencies and pin what is adopted. Document lower-star exceptions honestly.
- An overlay is not evidence that native collision or items work. Record actual observable tests and unresolved features in the port's status file/report. The blank adapter stays inactive until implemented.
- Run relevant build/contract tests from README. Keep `local/`, `guest/run/`, caches, game executables, account files, saves and credentials out of Git. Commit only an explicit source/documentation allowlist.
- Keep the Codex and Claude skill bodies identical; `tools/verify-template.py` checks the packaging. No publishing or access changes are implied by a future skill invocation; follow that user's actual request.
