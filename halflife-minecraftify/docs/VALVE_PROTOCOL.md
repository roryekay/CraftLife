# Valve adapter extensions

The inherited v1 camera/frame contract remains in `PROTOCOL.md`. Valve-specific
relay and native adapters stay outside the shared frame core.

The owned single-player host uses bounded 128-byte little-endian packets on
loopback only. `adapters/common/wire.hpp` and `relay.mjs` define the matching
layout: magic/version/kind/sequence/scene, 20 float32 and 8 int32 values. Wrong
size/version, invalid numeric fields and wrong relay source endpoints are
refused. Native work runs in game callbacks.

GoldSrc server/client ports: 25611/25613, relay 25621. Source server port 25612,
relay 25622. Both attach to the guest WebSocket at 127.0.0.1:25610. Only one
active guest owner is allowed. A 500 ms lease controls native input; sequences
reset on renewed ownership after expiration. A heartbeat precedes operations
so the first operation after attach is not lost. Windows UDP reset notices are
disabled on this socket because an absent relay is a normal startup state.
No OS security settings are changed.

Optional camera `eye:[x,y,z]` separates interaction/projectile origin from the
rendered camera in third person. Missing `eye` defaults to `p`, preserving v1
mock/test clients. Player eye queries and launches use it while connected.
Optional `gd` and `sn` carry native ground contact and crouching. Coordinates
are Minecraft X = -native Y / 32, Y = native Z / 32 + 64,
Z = -native X / 32. Yaw is 180 - native yaw.

Additional owner messages: `inventory`, normalized `cursor`, `travel_menu`,
`host_health`, `native_impact`, `terrain`, `scene`. Cursor bits are left/right;
conversion to Minecraft 26.3 SDL button IDs 1/3 occurs at the guest boundary.
Mouse coordinates also update hover/tooltips. Terrain is invisible native
occupancy, separate from placed blocks.

Guest observations: real `projectile` segments, `explosion`, completed
`vital_delta`, active `effects`, catalog-index `travel_request`. Sword input
observations include the actual selected item. Native firework impacts invoke
the real guest burst, which produces the native explosion request. Native
projectile impact IDs are deduplicated.

Native health is authoritative. Completed Minecraft healing/poison effects feed
deltas to it; native health reports reset the guest baseline. Native-owned fall,
pearl, firework and explosion damage is suppressed on the guest player to avoid
double application. Enemy rules and skill balance remain in native adapters.
Health deltas use Minecraft health units; the relay scales by native maximum
health / 20. Positive host health after death invokes the normal server respawn
handler before setting health, preserving the player connection and inventory.
The isolated Valve worlds enable keepInventory; adapter kit tags survive respawn.

Travel indices resolve only against the generated catalog; no arbitrary map or
console-command text is accepted. Scene changes clear transient projectiles/TNT,
clear sampled terrain, restore the map's saved Minecraft blocks, then replay
full-cube colliders as they enter range. Blocks persist with the full Minecraft
BlockState codec; complex native collision shapes are not implemented. Normal
shutdown removes temporary scene/terrain blocks from the void world while
retaining scene storage.
The relay waits for a fresh native scene before selecting an encounter. Safe
boss viewpoints prefer line of sight, but ordinary campaign puzzle triggers
still apply. Normal startup watches the native process and releases its relay
ports when that process exits.

Native fixture operations 20–28 require `-minecraftify-tests`
(`start-native.ps1 -Diagnostics`). They create/remove/read one native zombie,
set bounded test view/health/position, create test cubes/rays and hold fixed
forward/back/jump input for at most 500 ms. There is no remote console. Normal
relay startup does not expose its internal JavaScript diagnostic API. Gameplay
tests still drive real Minecraft items and consumption.

UDP has no general reliable-delivery protocol. Missing acknowledgements are
logged; collider creation retries, other events do not form a transaction log.
See `STATUS.md` before treating any path as verified.

## Optional Minecraft movement authority

Heartbeat integer 1 requests Minecraft movement. Production defaults to this
mode; F6 toggles the native fallback. Diagnostic relay callers default to native
movement unless explicitly passed `movement:true`. Input bits 22–26 are W/S/A/D
and jump; bits 8/9 also drive sneak/sprint; bit 21 is the F6 edge.

Camera `gm` enables vanilla guest travel; `mb` reports collision clipping using
Minecraft X/Y/Z bits 1/2/4. Guest `movement` reports actual per-tick displacement,
velocity and glide/crouch flags. Native kind 30 supplies that desired displacement;
kind 31 returns accepted feet, ground contact and clipped axes. The native game
thread performs bounded hull sweeps, wall sliding and a 0.6-block ground step.
Displacements exceeding 16 blocks per packet are refused. Native locomotion is
suspended while owned, then restored on fallback, death or lease expiry.
Relative displacements prevent a delayed input packet from undoing a native
teleport, scripted move or boss jump.

Vanilla Minecraft owns acceleration, drag, jump and elytra/rocket flight. Native
hulls own geometry collision, rather than the coarse terrain copy. Source uses
Minecraft-sized sweeps; GoldSrc retains its engine's standing/duck hull shapes.
Flight rockets attached to a player are excluded from weapon impact/blast events.
The Valve kit migration equips real elytra and supplies plain offhand boost
rockets while preserving occupied equipment slots.
