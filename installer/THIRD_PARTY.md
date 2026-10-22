# Runtime and asset notices

This private, noncommercial test package preserves existing per-component licenses in `licenses/`.

- CPython 3.12: Python Software Foundation license; standard library and runtime from the existing local installation. https://www.python.org/
- Node.js 22.23.3: MIT and bundled third-party notices; runtime license included. https://nodejs.org/
- Fabric API 0.161.0+26.3: Apache-2.0; Fabric Loader 0.19.5 metadata points the official Launcher to Fabric's Maven endpoints. https://github.com/FabricMC/fabric
- ReShade 6.8.0: BSD-3-Clause; inherited pinned runtime/headers and notices retained. https://github.com/crosire/reshade
- Valve GoldSrc/Source SDK adapter builds: Valve's SDK terms apply; notices included. Requires owned supported Steam games. No original Valve game executable, BSP, character model or full audio recording is bundled.
- `installer/assets.py`: LGPL-2.1-or-later, with MS ADPCM decoder derived from libsndfile 1.2.2 (Copyright 1999-2017 Erik de Castro Lopo). Source is included and may be modified/replaced under that license. https://github.com/libsndfile/libsndfile/tree/1.2.2
- Approved Steve edits were produced locally with the previously reviewed F5/Vocos workflow. No TTS service, checkpoint or paid API is used by setup. Keep this voice patch package private and noncommercial.

The official Minecraft Launcher manages game ownership, sign-in and all Minecraft assets. CraftLife does not implement offline account substitution or copy authentication data. Public version JSON metadata is included so the Launcher can resolve its own normal downloads. See the source repository for build sources and existing component notices.
