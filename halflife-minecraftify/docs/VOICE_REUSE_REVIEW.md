# Local G-Man voice preview workflow

Status: Rory approved installation on 2026-10-21. Ten approved overrides are
installed in the isolated mods. The HL2 time-freeze line uses the approved
whispered take 2's Steve after the original recorded Time, without a repeated
Time or stutter. It was installed after Rory's explicit preview approval.
The other lines use the approved Steve/no-Gordon/no-Doctor edits.
No hosted voice generation, credits, subscriptions or paid APIs are used.
Model downloads and inference run locally. Game assets and generated voices are
kept under ignored `local/voice-patch/`, outside the source backup.

Reviewed 2026-10-21:

| Component | GitHub stars checked | Pinned source | License / role |
| --- | ---: | --- | --- |
| SWivid/F5-TTS | 15,434 | `2f866649b779881af8bd644e76b9a6d95053da29` | MIT code; speech inpainting |
| gemelo-ai/vocos | 1,164 | `eb39abfc42c1dee4854d9b10d44dd7d4fd3b0e53` | MIT; mel-to-wave decoder |
| ValvePython/vpk | 191 | `3ff641e5fa7cf53a12c886153db0aab7a7872821` | MIT; local archive reads |

F5 supplies the requested short speech edit rather than regenerating whole
performances. Chatterbox (27,135 stars when checked) was also considered; its
whole-utterance voice cloning did not fit preserving the surrounding samples as
directly. ValvePython/vpk is an explicit lower-star exception: Valve's installed
VPK CLI listed the archive but did not extract the requested files. This small,
dependency-free parser was inspected in full before reading the owned archive.
Its external-volume filename method needs an override to retain `english_` for
this installation. Extraction verifies each entry's CRC before saving locally.

Inspection covered F5 inference entry points, speech-edit example, model/DiT,
mel/tokenizer utilities, dependency manifest and license; Vocos setup/dependencies,
model/backbone, head, modules, spectral operations and checkpoint loader; and VPK
parser/setup/license. No concerning behavior found in these reviewed paths.
This is a limited source review, not an audit of every dependency or wheel.
Upstream training/UI scripts are not executed. Local F5 `model/__init__.py`
omits the unused Trainer import, avoiding wandb/training imports. Local Vocos
`__init__.py` omits pretrained imports: the runner instantiates only the reviewed
backbone and ISTFT head, avoiding unused Encodec and network loaders.

F5 weights are CC-BY-NC, separately from its MIT code; keep this personal,
noncommercial experiment local and do not redistribute weights or game audio.
The F5 model is from Hugging Face revision
`84e5a410d9cead4de2f847e7c9369a6440bdfaca`,
`F5TTS_v1_Base/model_1250000.safetensors`, SHA-256
`670900fd14e6c458b95da6e9ed317cdb20dbaf7a1c02ac06a05475a9d32b6a38`.
Vocos is from revision `0feb3fdd929bcd6649e0e7c5a688cf7dd012ef21`,
`pytorch_model.bin`, SHA-256
`97ec976ad1fd67a33ab2682d29c0ac7df85234fae875aefcc5fb215681a91b2a`.
Its 461-byte `config.yaml` has SHA-256
`da9033922f969a47f0c160010226919e59f27761fd5066f3828d46de6650b0fc`.
The runner checks all three hashes before loading, uses safetensors or
`torch.load(weights_only=True)`, and does not import remote model code.

Isolated Python 3.12 environment: torch/torchaudio 2.8.0+cu128, numpy 2.2.6,
scipy 1.15.3, soundfile 0.13.1, librosa 0.11.0, x-transformers 2.11.24,
torchdiffeq 0.2.5, pypinyin 0.55.0, rjieba 0.1.13, safetensors 0.6.2,
einops 0.8.1, pyyaml 6.0.3. No global Python installation was modified.
TorchAudio's included Wav2Vec2 ASR pipeline assists finding names in HL1's
unlabelled 8-bit recordings; local results require checking because unusual
names and the character's delivery cause recognition errors.

`tools/edit-gman-voices.py` reads local edit manifests, inpaints only named time
ranges, fades boundaries and retains original sample count, rate and all decoded
audio outside edits. HL2's compressed MS ADPCM opening files become supported
PCM16 WAVs; original PCM recordings keep their unedited PCM bytes. Existing
facial metadata was retained for previews; the installer replaces removed titles
and old name phonemes with Steve cues before installation. Source timing metadata has duplicate or
misaligned word entries, so name ranges use verified phoneme/ASR context rather
than blindly replacing every metadata match.

Run with the local environment's Python and an explicit `--manifest` under
`local/voice-patch/`. Generated output and `generation-report.json` stay there.
The generation script deliberately contains no installer or paid-service fallback.
`tools/install-gman-voices.py` separately installs the approved local recordings,
checks PCM headers/hashes, updates HL2 facial cues and confines writes to the
isolated mod sound directories. It never writes through the original-game mounts.
The installer defaults to the approved `--time-take whisper2`; `take18` remains
an explicit local rollback option. Mouth cues follow the selected recording.

The existing cross-game F8 catalog now includes six G-Man destinations: HL1's
full ending, job offer, accepted offer and refused offer; HL2's opening and
time-freeze ending. The menu's G-Man shortcut opens the last page. Existing
destination indices remain stable. Direct scenes reuse the installed maps'
own named actors/choreography; no maps or recordings are included in source.
