# Tutorial recording outline

Goal: show how a reusable Minecraft guest connects to one real game's adapter. Avoid promising that one button can mod every engine.

1. Explain the two running games and shared responsibilities: camera/input/events over local WebSocket; colour/depth/HUD through shared memory.
2. Clone MinecraftifyAnything and invoke the included Codex or Claude Code skill with the chosen game and desired behavior. Generate a fresh port; show its initial unimplemented capability file.
3. Build and start the isolated Minecraft sandbox. Run `mock-host.mjs` and `frame_probe`; record live capture IDs before adding a game.
4. Identify the game's modding route, supported version and units/axes. Review one suitable upstream adapter or API and record its license/pin. Explain why copying old offsets cannot replace this step.
5. Implement a camera-only host adapter, load the neutral renderer, and align one block. Show both the successful overlay and a wall/angle test.
6. Add input forwarding and menu/disconnect handling. Then native block collision. Show standing, jumping, release and mining with observable results.
7. Add one real combat action and one real heal/potion action. Show native enemy/player health before and after; explain the acknowledgment/ownership boundary.
8. Test a fresh install, document remaining limits, and package redistributable source/binaries without game files or accounts.

Assets to capture later: architecture sketch, the clean repository tree, mock-host capture output, coordinate comparison, collision demonstration, native damage/heal evidence, and clean main-PC install. No tutorial video or universal working game port is claimed by this initial template release.
