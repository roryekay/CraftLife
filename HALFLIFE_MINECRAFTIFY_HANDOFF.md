# Half-Life Minecraftify builds

These are fresh Minecraftify Anything integrations using real Minecraft alongside
the owned Steam games. HalfCraft is not used. Both native runtimes and Minecraft
profiles are installed locally; they remain experimental integrations.

## Play

1. Open Minecraft Launcher, Java Edition. Select **Minecraftify Half-Life
   (development)** or **Minecraftify Half-Life 2 (development)** and press Play.
   Wait for the isolated Minecraft world to finish loading.
2. Double-click the matching **Play Half-Life 1.cmd** or **Play Half-Life 2.cmd**
   in this folder. Keep Minecraft running behind the native game.
3. Play in the Half-Life window. Press **F8**, choose **Half-Life 1** or
   **Half-Life 2**, then select a boss or key moment. Selecting a destination in
   the other game closes the current native game and starts the selected one.
   Keep the same Minecraft session open: inventory, elytra and equipment stay
   with you. You do not need to start the other Minecraft profile.

The teleporter also has keyboard controls: **F9** selects Half-Life 1, **F10**
selects Half-Life 2, **Page Up / Page Down** changes pages, and **1–8** selects
the numbered destination shown on that page. Use these if a short mouse click
does not register. These keys apply while the F8 menu is open.

The first connection supplies a finite Minecraft kit. The bow has Power V;
crossbow arrows explode. For actual firework ammunition, move fireworks from
inventory to the offhand with F, then load and shoot the crossbow normally.
Drink potions by holding right-click until consumption finishes. Milk clears
effects. TNT needs flint and steel. Mining applies to placed Minecraft blocks.

Minecraft movement is now the default. The kit equips real elytra and puts 64
plain flight rockets in your offhand (occupied equipment slots are preserved).
Jump, then press Space again while airborne to open the elytra. Look where you
want to fly; select the sword or an empty main hand and right-click to boost.
Use the explosive fireworks in inventory when you want crossbow ammunition.
Press F6 to switch to native movement for vehicles, ladders, swimming or scripted
sections; press it again to return to Minecraft movement.

| Control | Action |
|---|---|
| WASD / mouse / Space / Shift | Minecraft movement, look, jump, crouch |
| Ctrl while moving forward | Minecraft sprint |
| Space again in the air | Open equipped elytra |
| Left / right mouse | Minecraft attack or mining / use or place |
| 1–9 / wheel | Minecraft hotbar |
| I | Real Minecraft inventory; click to transfer items |
| F / Q | Swap offhand / drop item |
| E / L | Native interaction / flashlight |
| F5 | Third person / first person |
| F6 | Minecraft movement / native movement fallback |
| F8 | Choose either game, then a boss or key moment; Previous/Next changes page |

Travel loads a fresh native map. Some encounters still need their ordinary
story or puzzle triggers. Cross-game travel takes a few seconds to start and
reconnect; it does not resume native campaign saves. Placed blocks are stored per native scene; inventory
is retained on native death. Original Minecraft worlds and Steam game files
are not modified by these isolated profiles/runtimes.
Boss destinations can put you into immediate combat; the menu does not pause
native enemies. Use the native respawn action after death, then reopen F8.

## Verification and limits

See [the current test report](halflife-minecraftify/STATUS.md) and
[reuse review](halflife-minecraftify/VALVE_REUSE_REVIEW.md) for evidence and pins.
These builds support the installed HL1 build 15961492 and HL2 build 19307283.

Native walls and floors cannot be carved into voxels. Placed full cubes have
native collision; stairs, slabs, containers and moving native geometry do not
have complete parity. HL2 native depth occlusion still needs calibration, so
Minecraft visuals can show through native surfaces. The supplied drink potions
have native effects; splash/lingering effects on native NPCs and HL2 resistance
are not implemented. Full campaign completion and every boss location have not
been certified.
Walking, sprinting, crouching, jumping, gliding and firework boosts were tested
in both native games. Swimming, ladders, vehicles and campaign movement scripts
are not certified in Minecraft mode; use F6 there. HL1 retains its native hull
sizes, so tight gaps differ from vanilla Minecraft.

For rebuilding, use the primary project's `tools/prepare-valve-runtime.ps1`,
`tools/install-minecraft-profiles.py` and `tools/start-native.mjs`. The installer
requires the reviewed Fabric 0.19.5 / Minecraft 26.3 launcher version. Close the
launcher before reinstalling profiles. The native start scripts use the local
recorded Node runtime and stop their relay when the native process exits.
