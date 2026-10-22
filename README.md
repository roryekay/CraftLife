# CraftLife

Experimental Minecraftify Anything integrations for **Half-Life 1 and Half-Life 2**.
Real Minecraft runs alongside the owned native games, supplying its inventory,
items, blocks and movement. HalfCraft is not used.

## Play on the prepared computer

**[Download the private Windows installer ZIP from Releases](https://github.com/roryekay/CraftLife/releases).**
Extract All, run **Install.cmd**, start a **CraftLife** profile in the official
Minecraft Launcher, then run **Play Half-Life 1.cmd** or **Play Half-Life 2.cmd**.
Minecraft Java and an owned supported Steam copy of **Half-Life 1, Half-Life 2,
or both** are required. Setup skips missing games; cross-game travel is enabled
when both are installed. Run Install again after adding the second game.
The current package includes the approved Steve voice patches, original G-Man's
Minecraft portal ending, and the dark-outline fix. Setup prepares owned assets
locally; no developer saves or accounts are included. No development tools or
paid services are needed. See the [simple install guide](installer/INSTALL.md).

Start either Minecraftify Half-Life profile in the official Minecraft Launcher,
wait for its isolated world, then use **Play Half-Life 1.cmd** or
**Play Half-Life 2.cmd** from the original prepared workspace.

**F8** opens the combined boss/key-moment teleporter. While it is open:

- **F9 / F10** chooses Half-Life 1 / Half-Life 2.
- **Page Up / Page Down** browses pages.
- **1–8** selects the numbered destination.

The **G-Man voice scenes** button jumps to the dedicated entries on the last page.
The local install uses the approved Steve voices; recordings and the owned
ending-map variant remain local. Their preparation and installation source is
backed up here.

Cross-game travel keeps the same Minecraft session and inventory open. It loads
a fresh native map rather than resuming a native campaign save. Bosses remain
dangerous and the menu does not pause native AI. Short HL2 mouse clicks can miss;
the keyboard route is the tested option.

Minecraft movement is enabled by default, including elytra and firework boosts.
**F6** switches to native movement for ladders, swimming, vehicles and scripts.
Use **I** for inventory and **F5** for third person.

See the [play guide](HALFLIFE_MINECRAFTIFY_HANDOFF.md) and
[tested capabilities and remaining limits](halflife-minecraftify/STATUS.md).
Full campaign completion is not certified.

## Source layout

- `halflife-minecraftify/`: shared Fabric guest, relay, launch/install tools,
  GoldSrc client/server adapter, renderer, tests and documentation.
- `halflife2-minecraftify/adapters/halflife2/`: Source plugin and collider sources.
  This adapter uses the shared guest and common protocol from its sibling.
- `SOURCE_MANIFEST.json`: SHA-256 hashes of the exported source snapshot.

This repository contains source; **Releases** provides the ready-to-install
Windows player package. Developer build scripts still expect prepared local
SDK/toolchain directories and owned Steam/Minecraft installations.
Build caches, SDK checkouts, downloaded runtimes and game assets are deliberately
excluded. Dependency versions and review limits are recorded in the
[Valve reuse review](halflife-minecraftify/VALVE_REUSE_REVIEW.md) and
[foundation review](halflife-minecraftify/UPSTREAM_REVIEW.md).

No accounts, authentication files, credentials, saves, logs, purchased game
assets or native game executables are included. The only JARs tracked are the
Gradle wrapper bootstrap files needed to build the source.

## Licenses

Preserve the existing notices and per-component terms in both projects'
`LICENSE` and `licenses/` directories. The inherited foundation's MIT notice
does not replace Valve SDK or other third-party license terms.

## October 20 repair

Inventory remains **I**. **T** opens Minecraft chat for `/gamemode creative` and
`/gamemode survival`. This update adds native-frame movement smoothing,
moving-floor handling, late camera reprojection and explosive-impact particles
and sound. See [validation and remaining work](halflife-minecraftify/STATUS.md).
This remains an experimental integration; native campaign rules still apply.
