# Fresh Half-Life ports — reuse review

Reviewed October 18, 2026. The requested strategy is the user's maintained
Minecraftify Anything real-Minecraft guest/camera/shared-memory/compositor
foundation, copied with its generator at commit
`e64409fc2d38adca64fe85370f24478c93e3aad1`. HalfCraft was explicitly excluded:
none of its implementation, binaries, worlds or profiles were adopted or modified.

GitHub searches for Half-Life/Minecraft passthrough and cross-game bridges did
not identify a complete qualifying implementation near 500 stars. Search did
find HalfCraft (2 stars), which the user excluded. No lower-star third-party
implementation was adopted. Custom code here supplies missing adapters.

| Reused project | GitHub stars checked 2026-10-18 | Pin | License / scope |
|---|---:|---|---|
| [ValveSoftware/halflife](https://github.com/ValveSoftware/halflife) | 4,387 | `b1b5cf5892918535619b2937bb927e46cb097ba1` | Valve Half-Life 1 SDK license, free noncommercial mods; fresh client/server DLLs |
| [ValveSoftware/source-sdk-2013](https://github.com/ValveSoftware/source-sdk-2013/tree/singleplayer) | 10,067 | `77567eb4bce8d7b6b7f73e929fcde3a331c0216e` | Valve Source SDK license; singleplayer plugin/tools interfaces and damage layout |

Inspected relevant SDK client/server frame, view, movement, collision, crowbar,
damage, entity creation, player info, server tools, server-plugin callbacks,
keyvalue/filter/speed-modifier paths; project source lists, linked SDK libraries,
build scripts and license/notices. The build preparation reads source lists
instead of executing upstream post-build copying scripts. The optional GoldSrc
ICMP/DNS benchmark is replaced with a disabled benchmark implementation.
SDK-supplied VGUI/import libraries are used; they were not independently rebuilt
or exhaustively audited. No concerning unrelated behavior found in the reviewed
paths. This is not a complete audit of the SDKs or dependency supply chain.

The template's Fabric/Minecraft, Gradle, Java-WebSocket and ReShade API dependency
pins and notices are retained; see the inherited `UPSTREAM_REVIEW.md` for their
prior review. The guest and new adapters are built locally. Game assets remain
in owned Steam installations and are referenced by isolated runtime junctions;
owned runtime executables/DLLs copied into ignored `local/` are never packaged.

For HL2's D3D9 runtime, the official
[ReShade 6.8.0 add-on installer](https://reshade.me/downloads/ReShade_Setup_6.8.0_Addon.exe)
was downloaded and its embedded DLL archive extracted without running its
installer. Installer SHA256:
`afe4c8f13048306307983b8b3d41d5bf00a86820440b0e57dea10950e1176445`.
Windows reported its self-signed `CN=ReShade` signer as an untrusted-root chain,
with a DigiCert timestamp. No security settings were changed. Official origin
and a recorded hash do not establish an independently verified binary/source
match. HL1 now uses its SDK client draw callbacks, not that runtime.

No accounts, launcher authentication databases, purchased assets or saves belong
in source/distributable packages. Nothing was uploaded, published or pushed.

## Minecraft movement extension (October 19, 2026)

GitHub searches covered Half-Life Minecraft movement bridges and vanilla elytra
integration. [Minecraft-X-HalfLife](https://github.com/SawyerTheNerd/Minecraft-X-HalfLife)
describes another strategy; its API metadata reported 0 stars and no license.
It was not adopted, consistent with the user's instruction to keep these ports
on Minecraftify Anything. [Do a Barrel Roll](https://github.com/isXander/do-a-barrel-roll)
changes flight controls/physics rather than supplying this native bridge; the
queried API metadata reported 0 stars and GPL-3.0. No code or binaries were taken
from either candidate. Metadata/README review is not a source security audit.
No suitable qualifying drop-in bridge was found. This extension uses the
already-installed vanilla Minecraft travel, sprint, jump, elytra and firework
logic, with custom bounded native movement/collision glue against the existing
pinned Valve SDKs. No new runtime dependency was added.

## Cross-game travel extension (October 19, 2026)

GitHub searches for MinecraftifyAnything/Half-Life cross-game teleport launchers
found no suitable qualifying drop-in implementation. HalfCraft remains excluded
as requested; Minecraft-X-HalfLife uses a different bridge and was not adopted.
General Minecraft launchers do not provide the required Valve runtime handoff.
This extension reuses our reviewed Node launcher, relay, static moment catalogs
and pinned SDK adapters. No external code, binary or new dependency was adopted.
Replacement checks both the recorded native PID and its exact isolated executable
path before stopping it; destinations are catalog data, never commands.

October 20 follow-up: cursor release uses the existing host SDL2 runtime's
relative-mouse functions and the pinned Source SDK's VGUI_Surface030 interface.
No replacement SDL binary, event library, window-message hook or new dependency
is installed. The final teleporter also supplies native keyboard selection and
paging, verified in both engines; short polled HL2 mouse clicks remain a limitation.


## Movement, camera and chat repair (October 20, 2026)

GitHub searches for Source/GoldSrc moving-platform movement and asynchronous
reprojection found no qualifying drop-in bridge for these OpenGL/D3D9 runtimes.
Valve's official Source SDK reported 10,073 stars and HL1 SDK 4,390 on this check;
the existing pinned SDK revisions remain unchanged. Public collision/pusher,
entity handle, transform and plugin interfaces supply the native building blocks.
`xPoiler/Reshade-Asynchronous-Reprojection` reported 34 stars, GPL-3.0 and a
DX11/DX12/Vulkan target; it was not adopted. The vrtoolkit API metadata request
failed and no review/adoption claim is made for it. No external code or new
runtime dependency was added. Camera math, interpolation and narrow integration
glue are custom. Chat subclasses only the owned game's window procedure while
loaded; it forwards input only during focused Minecraft chat and restores the
procedure on unload. It installs no global hook or persistent input component.
These are scoped source/API reviews, not an audit of the entire SDK or game.
