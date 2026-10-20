# Protocol v1

Transport: text JSON over WebSocket on `127.0.0.1:25610`. Override guest port with `-Dminecraftify.port=...`. Messages are at most16KiB. There is one active host owner and any number of read-only observers. The connection is local IPC, not an authenticated remote service; do not expose it to a network. No remote console, arbitrary game command, mission selector or legacy director relay exists.

On connect the guest sends `{"t":"hello","v":1,"mapping":"Local\\MinecraftifyAnythingFrame","pid":1234}`. An adapter sends `{"t":"attach","v":1}` and waits for `{"t":"attached","ok":true}` before sending camera/input. A second active adapter receives `ok:false`; observers need no attach. The owner is released on connection close. Unknown owner messages return `unsupported`; malformed messages return `invalid_message`. Parse/validate received JSON in adapters; event fields are data.

## Host to guest

```json
{"t":"cam","f":1,"p":[0,65.62,0],"pl":[0,64,0],"r":[0,0,0],"fov":70,"fp":true,"ctl":true}
```

`f`: monotonically increasing frame number per connection; echoed in image metadata. `p`: camera XYZ, `pl`: feet XYZ; all in Minecraft coordinates. `r`: yaw,pitch,roll in degrees; vertical `fov`10..170. Numbers must be finite. Defaults: first person and control enabled. The camera lease lasts500ms after receipt. A camera drives the guest player's feet without simulating a second host movement controller. Use the isolated Creative void world; host health/death rules are deliberately absent.

```json
{"t":"key","k":"use","down":true}
{"t":"key","k":"use","down":false}
{"t":"slot","n":2}
{"t":"scroll","d":1}
```

Keys: attack/use/pick/inventory/drop/swap/escape. `slot` is zero-based0..8. Scroll uses the sign only. Item keys use held state plus a press edge. Release between distinct clicks. With `ctl:false`, new presses and selection changes are ignored; releases remain allowed. Disconnect/expired control releases attack/use/pick/drop/swap/inventory and clears queued clicks. Native movement and UI cursor injection are adapter work.

## Guest to observers/host

- `{"t":"guest_state","slot":2,"item":"minecraft:crossbow","using":false,"blocking":false,"screen":false}` at5Hz while the guest world is loaded. It also reports `active`, `control`, `attack` and `use` so timeout/release state is observable.
- `{"t":"action","k":"attack","down":true}` when an accepted forwarded action is applied. This is input observation, not a confirmed hit or completed item use.
- `{"t":"block_changed","pos":[0,64,2],"block":"minecraft:stone","air":false}` for successful integrated-server block changes while attached. This is an observation stream, not a collision command or reconnect snapshot. Variant block properties and complex voxel shapes need an extension.

There are no host damage, heal, projectile, shield, totem or potion-effect contracts yet. Design these explicitly in a future port and keep game-specific native effects outside the shared guest.

## Shared-memory frames

Windows x64, little endian, mapping `Local\MinecraftifyAnythingFrame`. Guest override: `-Dminecraftify.mapping=...`; change the native reader's constructor argument to match. Three slots; max3840×2160, per-slot stride99,532,800bytes, header4096bytes. The pagefile mapping reserves about285MiB. `native/include/fusion/frame_protocol.hpp` and `FrameExporter.java` define the same checked layout.

| Header offset | Type | Meaning |
| --- | --- | --- |
|0/4/8/12|uint32×4|magic0x5450434d, version1, header4096, slots3|
|16|uint64|slot stride|
|24/28|uint32×2|max width/height|
|32|uint64|publication counter|
|40/44|int32/uint32|latest slot(-1 means none), producer PID|

Slot descriptors at256+128×slot:

| Offset within descriptor | Type | Meaning |
| --- | --- | --- |
|0|uint64|sequence: odd while writing, even when complete|
|8/16|uint64×2|guest capture ID / host camera frame ID|
|24/28|uint32×2|width/height|
|32/36/40|float32×3|near/far/vertical FOV|
|44|uint32|flags:1=zero-to-one depth,2=bottom-up rows,4=reversed Z|
|48/56/64|float64×3|captured camera XYZ|
|72/76/80|float32×3|captured yaw/pitch/roll|
|84|int32|first person|
|88/96|int64×2|capture/publication monotonic nanoseconds|
|104/112/120|float64×3|captured guest player feet (reserved for future reprojection)|

Pixels at4096+stride×slot: world premultipliedRGBA8, world float32depth, then hands/HUD premultipliedRGBA8. Each layer occupies width×height×4bytes with no padding. Copy descriptor and all layers, fence, and recheck the same even sequence before accepting. Never upload from shared memory that the producer may overwrite. Failed reads preserve the last complete frame. Renderer contexts track producer lifetime and reopen mappings after guest restarts. Images expire after500ms without a new accepted capture.
