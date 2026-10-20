# Reuse and extraction review

Reviewed2026-10-17. GitHub searches for Minecraft passthrough templates and cross-game ReShade bridges found the established [rehan-remade/universal-modder](https://github.com/rehan-remade/universal-modder) foundation (**8,237stars**, API-checked). Its Minecraft example is tied to a particular host; no suitable ready-made neutral template was identified. We reuse the MIT exporter approach and targeted known-working render hooks, then provide the missing neutral boundary, minimal guest, mock/probe and generator. No lower-star whole-game port is adopted into this repository.

Upstream pin: **6a8865ae171b406f50892c7f46fc5f45655269eb**. Reviewed example `mc/build.gradle`, `HostLink.java`, `FrameExporter.java`, native compositor transfer/registration paths, install script and root MIT license. The host install script was read, not executed or adopted. The local exporter includes previously developed completed-readback, coherent-pose and monotonic-publication fixes; package names/mapping were neutralized. The original Rehan/universal-modder MIT notice is retained in LICENSE and licenses/. The new minimal host link replaces legacy host commands/relays; no arbitrary remote console or inherited native game hooks remain.

Shared components:

| Component | Pin/license | Scope |
| --- | --- | --- |
|ReShade|6.8.0, commit18deaa52de0c425a78b329e9cb3c497281cd00ec; BSD-3-Clause;5,601stars checked|Eight vendored API headers match official tag byte-for-byte; hashes in third_party/reshade-sha256.json. Native registration, upload, binding and release paths inspected. No ReShade runtime binary distributed.|
|Java-WebSocket|1.6.0; MIT;10,801stars checked|Pinned Fabric dependency and included JAR dependency. Local binding, connection lifecycle/message routing reviewed. No claim of complete dependency audit.|
|Fabric|Loader0.19.5/API0.161.0+26.3; API Apache-2.0|Pinned build dependencies; Fabric/official Minecraft endpoints. No custom launcher or account workflow.|
|Gradle|9.7.1; Apache-2.0|Wrapper distribution SHA256 pinned in gradle-wrapper.properties; scripts/JAR retained from reviewed tooling.|
|Minecraft|Java26.3, Java25 required|User-owned runtime/development dependency. No game binaries/assets redistributed.|

The separate AZP3001/mc-passthrough repository was considered (0stars, no repository license reported by API) and not adopted. No engine SDK, MinHook, ARGENT, Sentinel, prior boss selectors, native offsets or controller-injection code is present in this neutral starter.

This is a targeted review of the paths used and the extraction boundary, not a complete security audit of Minecraft, Java, Fabric, Gradle, ReShade or upstream code. Builds and contract/live tests are recorded separately in docs/TESTING.md. Source verification does not prove an independently downloaded binary's provenance.

The repository and development release omit accounts, credentials, purchased game files, developer worlds, local logs and caches. GitHub publishing uses an explicit source allowlist. Local test artifacts stay under ignored folders.
