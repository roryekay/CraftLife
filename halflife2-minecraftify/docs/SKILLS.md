# Codex and Claude Code

The repository contains the same `minecraftify-anything` skill in each tool's project-local discovery folder:

| Tool | Location | Explicit invocation |
| --- | --- | --- |
|Codex|`.agents/skills/minecraftify-anything/SKILL.md`|`$minecraftify-anything`|
|Claude Code|`.claude/skills/minecraftify-anything/SKILL.md`|`/minecraftify-anything`|

Open the cloned template or a generated port in the coding tool. Start a new session if it was already open before the skill files were added. No API key, MCP server or global configuration change is required by these instruction-only skills. They use the coding tool's existing shell/network/UI capabilities and permissions; they do not provide missing native game access.

Example prompt:

```text
Use the minecraftify-anything skill to put Minecraft into <my game>.
Create an isolated port from this template. Start with camera and rendering,
then add block collision, combat and healing. Record what actually passes.
```

Project-local skills travel with generated ports. If you copy a skill into a global directory, keep a local clone of this repository too: its scripts, source and protocol documents are required, and the skill instructs the agent to locate that clone. Global installation is optional and is not performed by this template.

The canonical skill body is `.agents/skills/minecraftify-anything/SKILL.md`; keep the Claude copy byte-identical. Validate both using Codex's skill-creator `quick_validate.py` when available, plus `python tools/verify-template.py`. This checks structure/distribution, not the behavior of an entire future game port.

Locations and syntax checked against [OpenAI's skill documentation](https://developers.openai.com/codex/skills) and [Claude Code's skill documentation](https://code.claude.com/docs/en/skills) on2026-10-17. A future tool version may change discovery details.
