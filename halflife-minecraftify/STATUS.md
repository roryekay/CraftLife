# Half-Life Minecraftify status — October 20, 2026

Fresh Minecraftify Anything integrations are built and installed in isolated
native runtimes and official Minecraft Launcher profiles. HalfCraft is not used.
These are experimental playable integrations, not full campaign certifications.
Read [the play guide](../HALFLIFE_MINECRAFTIFY_HANDOFF.md).

## Cross-game boss teleporter

F8 now contains both catalogs. Selecting a destination in the other game replaces
only the recorded, path-verified isolated native process and reconnects the same
Minecraft guest. Either installed profile can stay open for both games. Inventory
and equipment remain in that guest; placed blocks remain keyed by game and map.
No native campaign save is resumed: destinations load fresh maps.

Keyboard controls while the menu is open: F9 selects HL1, F10 selects HL2,
Page Up/Down changes page, and 1–8 selects the numbered destination. These were
tested through actual native-window inputs in both engines. Use the keyboard if
a short mouse click is missed; HL2's polled pointer/buttons still have that limit.
The Source adapter now explicitly releases both SDL and VGUI cursor capture.

The final October 20 round trip loaded HL2 Antlion Guard (d2_coast_11, encounter 9)
and HL1 Gargantua (c2a5g, encounter 2), receiving both native encounter acknowledgements.
Guest PID 19420 stayed running; equipped elytra, selected sword and 64 offhand
rockets were retained. Gargantua killed the idle test player after arrival;
the final HL1 inventory check also covered ordinary native death/respawn back to
its checkpoint. Boss travel does not grant invulnerability or pause native AI.
Evidence: local/cross-game-travel-acceptance.json. Catalog validation, malformed
destination rejection, recorded-PID checks and rejection of a matching PID whose
executable is unrelated also passed. Both native builds and the shared guest
build passed; CTest passed 3/3 and the template-isolation verifier passed.

## Minecraft movement extension

Production now defaults to real vanilla Minecraft travel. F6 switches between
Minecraft and native movement. The kit migration equips elytra and provides
64 plain offhand flight rockets, preserving already occupied equipment slots.
Attached boost rockets do not enter native weapon impact/explosion handling.

Both native adapters passed the live movement checks using the installed shared guest:

| Native game | Walking / sprint distance | Jump rise | Firework speed before / after |
|---|---|---|---|
| HL1, Surface Tension | 3.86 / 5.02 blocks | 1.22 blocks | 0.34 / 1.68 blocks per tick |
| HL2, Highway 17 | 3.84 / 5.02 blocks | 1.25 blocks | 0.36 / 1.28 blocks per tick |

Real elytra gliding and real rocket consumption moved the native player in both.
Crouching and restoration of native movement ownership also passed. HL1 real
inventory potion transfer/consumption and boss travel were rechecked with
Minecraft movement enabled; the crossbow then damaged Gargantua 800 → 725.
Evidence: local/hl1-minecraft-movement.json, hl2-minecraft-movement.json and
hl1-movement-travel.json. CTest also checks fast sweeps, wall sliding, floor and
ceiling contact, ground stepping and refusal to step while gliding.

The production F6 keyboard toggle passed; native forward input moved the player
in both games after returning to native movement. Seven passing movement/input/
UI regression records are consolidated in local/acceptance-summary.json.

Native hull sweeps own collision; Minecraft owns acceleration, drag, jumping
and elytra/rocket physics. HL1 keeps its native standing/duck hull sizes. Exact
vanilla tight-gap collision, swimming, ladders, vehicles and scripted campaign
movement and complete vanilla fall/wall damage parity are not certified in
Minecraft mode; use the F6 native fallback where needed.

## Observed gameplay passes

| Feature | Evidence in both native games |
|---|---|
| Real Minecraft | Native game displays actual guest hands, blocks, HUD, inventory and avatar |
| Sword | Actual native zombie 1000 → 990; matches crowbar skill damage 10 |
| Power V bow | Real arrows damage native zombies; final HL1 shot 990 → 967.86 and HL2 990 → 953 after native armor |
| Explosive crossbow | Actual loaded crossbow arrows and actual firework ammunition produce native damage/blasts |
| Blocks and pickaxe | Real cobblestone consumed, native cube collider installed, real pickaxe breaks block and removes collider |
| Support physics | Native player stands, jumps and walks off a Minecraft cube |
| Ender pearls | Real pearl consumed and native player moves to safe landing |
| TNT | Real TNT damages native zombie: HL1 1000 → 887.44; HL2 1000 → 831 |
| Potions | Real healing consumption; speed/jump measured through native walking and jump heights; native fire damage blocked; milk restores movement |
| Inventory and respawn | Both authenticated profiles passed real sword transfer, native death/respawn inventory retention, transfer afterward, UI close and reconnect |
| Third person | F5 changes native camera; real Minecraft avatar visually inspected in both; returned to first person |
| Boss travel | Real menu loads fresh native map and reconnects; Gargantua 800 → 700 and Antlion Guard 500 → 350 from real explosive crossbow shots |

Real invisibility potions enabled native target protection in both games and
adapter disconnect cleared it. Real milk consumption also passed in HL2.
Both authenticated profiles exported 119 coherent color/depth/HUD frames in two
seconds. Ownership, invalid packets, held-input timeout/release and reconnect
checks pass. The final guest JAR, GoldSrc DLLs and Source plugin compile; CTest
frame/wire checks, Node bounds/injection checks and template isolation/generator
checks pass. Final installed-profile HL2 combat, native support physics,
third-person avatar and fresh-map boss travel checks also passed. Third-person
checks wait for the rendered camera to follow the mode change; tight native
spaces can restrict camera distance. Native model visibility is networked so
the native player does not appear behind the Minecraft avatar.

## Local evidence

Ignored local/ contains official-hl1-guest-test.json, official-hl2-guest-test.json,
official-hl1-gameplay.json, official-hl2-combat.json, official-hl2-physics.json,
plus hl1-* and hl2-*:
native-block-test, block-gameplay, pearl-gameplay, firework-gameplay, tnt-gameplay,
potions-native, third-person, travel-gameplay and invisibility results.
Tests use real items and observable native state. Bounded native fixtures require
-minecraftify-tests; production shortcuts omit that flag. Latest recorded results
determine acceptance; failed earlier probes are not evidence of a pass.
local/acceptance-summary.json consolidates 23 passing records and SHA256 values
for the built binaries; installed copies were checked byte-for-byte. The final
acceptance worlds were archived, leaving both profiles ready for fresh kits.
The combined official-hl2-gameplay.json retains a checkpoint walk-off fixture
failure after its combat/healing passes. A bridge-side bow probe also failed.
The established Ravenholm area passed the separately scoped final combat and
physics tests. Those passes do not certify every map or fixture position.

## Limits

- Native BSP walls/floors cannot be carved into voxels. Mining applies to placed
  Minecraft blocks; normal native breakable entities retain native damage rules.
- Full cubes have native collision, up to 512 installed at once per scene. Stairs,
  slabs and other complex shapes do not have native voxel collision parity.
- Native terrain sampling is coarse and local. Slopes, moving props and distant
  geometry are not mirrored exactly.
- HL1 uses its own OpenGL compositor/depth path; exhaustive spatial alignment and
  performance acceptance remain. HL2 native depth occlusion is disabled pending
  calibration; guest visuals can show through native surfaces.
- HL2 jump uses gravity adjustment. HL2 resistance and splash/lingering effects
  on native NPCs are not implemented. Existing campaign damage filters are
  preserved, which can constrain fire resistance in special scenes.
- Catalog: 19 HL1 moments/14 boss locations, 15 HL2 moments/37 boss locations.
  Some require normal story/puzzle triggers. Every entry is not certified as an
  independently activated fight. Travel loads a fresh native map.
- Full campaign completion, every projectile edge case, native NPC loot conversion,
  container block-entity data and synchronized campaign saves are not established.
  Placed Minecraft BlockStates persist per native scene; player inventory persists.
- Local UDP is bounded/sequenced, not a general reliable transaction protocol.
  Block creation retries; other missed acknowledgements are logged.

## Installed environment and launch

HL1 Steam build 15961492: x86 official SDK client/server DLLs, OpenGL.
HL2 Steam build 19307283: x86 public Source SDK plugin, D3D9 ReShade renderer.
Minecraft 26.3, Fabric Loader 0.19.5, Fabric API 0.161.0+26.3, Java 25.
SDK pins/licenses/review limits: [VALVE_REUSE_REVIEW.md](VALVE_REUSE_REVIEW.md).

Start an authenticated Minecraftify Launcher profile first, wait for its
world, then double-click the root Play shortcut for the initial game. F8 can
switch games without closing Minecraft or starting another profile. Only one
native integration runs at a time. Shortcuts use the recorded existing Node runtime; no script-policy
change is required. Both normal shortcuts start and attach automatically; their
native-process exit releases the relay ports.
The supplied native/Minecraft runtimes are isolated from Steam originals and
ordinary Minecraft worlds. Existing isolated worlds are preserved on this update.
