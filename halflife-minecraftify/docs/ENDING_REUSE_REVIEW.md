# HL2 ending and saved Overworld

Current validation: guest/native builds and four C++ contract tests passed.
Live ending tests reached both authored phases, retained selected item/offhand
counts, and saved a healthy player at (64.5, 68, 0.5) with ordinary physics and
no host camera ownership. Saved-world reopen passed repeatedly. Visual checks
confirmed the original G-Man on Minecraft grass beside the active purple
portal; the native crossing event hides him, then the portal closes before
normal Minecraft control resumes. Inventory and both third-person views were
visually verified after handoff. Automated short jump-key taps were inconclusive.
Normal Survival damage was verified after the sandbox protection was disabled
for free Minecraft play: a local one-point damage command reduced health to 19.
A dark compositing fringe around the original model remains a visual limitation.
The full campaign map carries the same authored triggers; a complete HL2
campaign playthrough was not repeated. The approved whispered voice is installed.

Checked 2026-10-21. GitHub searches covered Fabric NPCs, dimension travel and
cinematic handoffs. No qualifying project supplied a Source actor composited
into Minecraft 26.3 followed by a saved single-player handoff.

FabricMC/fabric had 3,198 stars when checked, Apache-2.0. This port retains its
existing pinned Fabric API 0.161.0+26.3 and Fabric Loader 0.19.5. The installed
lifecycle/player events and Minecraft's public level generation, teleport,
respawn and save APIs supply the reusable components. No new dependency or
downloaded installer is needed. Relevant installed API signatures and the
vanilla 26.3 normal-world registry definitions were inspected locally.

CitizensDev/Citizens2 (727 stars, OSL-3.0) targets Bukkit rather than this Fabric
client. KingContaria/WorldPreview (1 star, MIT) targets older Minecraft and does
not perform this handoff. Neither is adopted. No smaller NPC library is used.
This is a scoped compatibility review, not an audit of every dependency.

Source's original owned G-Man model, animation, dialogue and door choreography
remain in Source. The isolated campaign map and ending shortcut add an
info_target marker driven by scene events. Phase 1 prepares a generated forest
clearing and translates the guest origin; phase 2 follows the actual door
closing. Both native sky passes are disabled for the composited stage, leaving
its original actor in front of Minecraft. The Source door-open event activates
the matching Minecraft portal, which closes on handoff. Shared frame flag 8 requests native foreground
occlusion, with a black-background fallback for unavailable D3D9 depth.

The guest saves the player/inventory in minecraftify:overworld, releases host
camera/input ownership, restores normal physics and sets a persistent respawn.
Only then does the current native session receive its bounded FinishEnding
message. It focuses the existing guest process and quits itself. Neither an
arbitrary process-kill interface nor a console-command bridge is exposed.

`tests/ending-handoff.mjs` observes the real production handoff; `--resume`
checks an actual saved-world reopen. Local receipts and all generated maps,
recordings, model weights and saves remain under ignored local/.
