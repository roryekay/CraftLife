# CraftLife private Windows test build

1. Download the Windows ZIP from this private repository's Releases page and **Extract All**.
2. Close Minecraft Launcher and Half-Life. Double-click **Install.cmd**.
3. Open the official Minecraft Launcher. Select **CraftLife - Half-Life** or **CraftLife - Half-Life 2**, then Play. Let its world load.
4. Double-click **Play Half-Life 1.cmd** or **Play Half-Life 2.cmd**. Steam must be running.

Requires Minecraft Java Edition, Windows 10/11 x64, and **at least one** owned supported Steam game: Half-Life 1, Half-Life 2, or both. Setup detects installed Steam games and skips the missing one. Both games enable cross-game travel; missing-game tabs are disabled. Run Install again after adding the second game to enable it without losing your CraftLife worlds. The official Launcher handles your sign-in and any Minecraft/Java/Fabric downloads; the bridge runtimes and mods are bundled. No development tools, administrator access, paid APIs or subscription service are needed. Installation defaults to `Games\CraftLife` in your user folder. For an unrecognized Steam library, pass `--steam-dir "D:\SteamLibrary"`, or `--halflife` / `--halflife2` with the installed game folder. Use English Steam voice files for the approved Steve lines.

**Play G-Man Ending.cmd** starts the tested ending shortcut. Original G-Man walks through the Minecraft portal and disappears; after the portal closes you continue in a real saved Overworld. The halo fix and approved whispered “Time, Steve?” are included.

**F8:** combined boss/key-moment travel. **F9/F10:** choose game. **Page Up/Down:** pages. **1–8:** destination. **I:** inventory in the native game. **T:** Minecraft chat, including `/gamemode creative` and `/gamemode survival`. **F5:** third person. **F6:** switch Minecraft/native movement for vehicles, ladders and scripted sections. After the ending, ordinary standalone Minecraft controls apply, including **E** for inventory.

Cross-game travel keeps the same Minecraft session/inventory and loads a fresh native map. It does not resume a native campaign save. The player package does not include your developer worlds or saved progress. Full campaign completion and complete item parity remain unverified; this is the current experimental build.

Run Install again to repair/update. It preserves existing isolated worlds and user options, backs up launcher settings locally, and leaves original Steam files untouched. It creates local runtime copies and asset junctions, then prepares the ending map and ten approved voice overrides from owned files. No full purchased map, model, recording, game executable, account database, save or model weight is distributed. Installer voices are patch recipes requiring the exact owned originals.

If Steam updates to an unsupported executable/map, setup/launch refuses that version. Do not bypass the check. For a custom folder, run `Install.cmd --install-dir "D:\Games\CraftLife"`; your Play buttons remember it. `Verify.cmd` checks the extracted package's SHA-256 manifest. Checksums detect corruption; the package is not code-signed.

To remove it, close both games and Minecraft, remove only the two CraftLife profiles in the official Launcher, then remove the isolated CraftLife install folder when you no longer need its worlds. Asset junctions point to Steam originals; never follow them when manually removing files. No driver, service, security-setting change or automatic startup entry is installed.
