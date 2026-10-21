"""Local-only name inpainting. Requires the separately reviewed, pinned F5/Vocos setup.

Inputs/models and generated game audio stay under local/voice-patch, never in Git.
No hosted inference, API keys, uploads, billing or paid services are used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys


def chunks(data):
    if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise ValueError('Expected a RIFF WAVE file')
    pos = 12
    while pos + 8 <= len(data):
        tag, size = struct.unpack_from('<4sI', data, pos)
        if pos + 8 + size > len(data):
            raise ValueError('Truncated WAV chunk')
        yield tag, data[pos+8:pos+8+size]
        pos += 8 + size + size % 2


def replace_pcm(original, samples, intervals, sr):
    """Preserve PCM bytes outside the named ranges and retain other RIFF chunks."""
    import numpy as np
    items = list(chunks(original))
    fmt = next(value for tag, value in items if tag == b'fmt ')
    encoding, channels, rate, _, align, bits = struct.unpack_from('<HHIIHH', fmt)
    if encoding == 2 and channels == 1 and rate == sr:
        # Source's opening lines are MS ADPCM. Decode once to lossless PCM16;
        # preserve all decoded samples outside edits, and retain facial metadata.
        import io
        import soundfile as sf
        decoded, _ = sf.read(io.BytesIO(original), dtype='int16')
        fmt = struct.pack('<HHIIHH', 1, 1, sr, sr*2, 2, 16)
        items = [(tag, fmt if tag == b'fmt ' else decoded.astype('<i2').tobytes() if tag == b'data' else value)
                 for tag, value in items if tag != b'fact']
        encoding, align, bits = 1, 2, 16
    if (encoding, channels, rate, bits) not in [(1, 1, sr, 8), (1, 1, sr, 16)]:
        raise ValueError('Only mono PCM8/PCM16 at the original sample rate is supported')
    pcm = bytearray(next(value for tag, value in items if tag == b'data'))
    if len(samples) != len(pcm)//align:
        raise ValueError('Replacement changed sample count')
    quantized = (np.rint(np.clip(samples, -1, 127/128)*128+128).astype('u1') if bits == 8
                 else np.rint(np.clip(samples, -1, 32767/32768)*32768).astype('<i2')).tobytes()
    for start, end in intervals:
        a, b = round(start*sr)*align, round(end*sr)*align
        pcm[a:b] = quantized[a:b]
    output = b'WAVE'
    for tag, value in items:
        if tag == b'data':
            value = bytes(pcm)
        output += tag + struct.pack('<I', len(value)) + value + (b'\0' if len(value)%2 else b'')
    return b'RIFF' + struct.pack('<I', len(output)) + output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workspace', type=Path, default=Path(__file__).resolve().parents[1]/'local/voice-patch')
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--seed', type=int, default=731)
    args = parser.parse_args()
    base = args.workspace.resolve()
    for repo in ('F5-TTS/src', 'vocos'):
        sys.path.insert(0, str(base/repo))
    import numpy as np
    import soundfile as sf
    import torch
    import torchaudio
    import yaml
    from safetensors.torch import load_file
    from f5_tts.model import CFM, DiT
    from f5_tts.model.utils import get_tokenizer, convert_char_to_pinyin
    from vocos.models import VocosBackbone
    from vocos.heads import ISTFTHead

    for name, digest in {
        'f5.safetensors': '670900fd14e6c458b95da6e9ed317cdb20dbaf7a1c02ac06a05475a9d32b6a38',
        'vocos.bin': '97ec976ad1fd67a33ab2682d29c0ac7df85234fae875aefcc5fb215681a91b2a',
        'vocos.yaml': 'da9033922f969a47f0c160010226919e59f27761fd5066f3828d46de6650b0fc',
    }.items():
        with (base/'models'/name).open('rb') as f:
            if hashlib.file_digest(f, 'sha256').hexdigest() != digest:
                raise ValueError(f'Checkpoint/config hash mismatch: {name}')
    torch.set_num_threads(4)
    device = 'cuda' if torch.cuda.is_available() else 'cpu'
    print('Local inference device:', device, flush=True)
    config = yaml.safe_load((base/'F5-TTS/src/f5_tts/configs/F5TTS_v1_Base.yaml').read_text())
    vocab, size = get_tokenizer(str(base/'F5-TTS/src/f5_tts/infer/examples/vocab.txt'), 'custom')
    model = CFM(transformer=DiT(**config['model']['arch'], text_num_embeds=size, mel_dim=100),
                vocab_char_map=vocab)
    state = {k.removeprefix('ema_model.'): v for k,v in load_file(str(base/'models/f5.safetensors')).items()
             if k not in ('initted', 'step')}
    for key in ('mel_spec.mel_stft.mel_scale.fb','mel_spec.mel_stft.spectrogram.window'):
        state.pop(key,None)
    model.load_state_dict(state)
    del state
    model = model.to(device=device, dtype=torch.float16 if device == 'cuda' else torch.float32).eval()
    voc_config = yaml.safe_load((base/'models/vocos.yaml').read_text())
    backbone = VocosBackbone(**voc_config['backbone']['init_args']).to(device).eval()
    head = ISTFTHead(**voc_config['head']['init_args']).to(device).eval()
    state = torch.load(base/'models/vocos.bin', map_location='cpu', weights_only=True)
    backbone.load_state_dict({k.removeprefix('backbone.'): v for k,v in state.items() if k.startswith('backbone.')})
    head.load_state_dict({k.removeprefix('head.'): v for k,v in state.items() if k.startswith('head.')})
    del state
    entries = json.loads((args.manifest or base/'edits.json').read_text())
    report=[]
    for entry in entries:
        source = base/'source'/entry['file']
        output = base/'generated'/entry['file']
        audio, sr = sf.read(source, dtype='float32')
        if audio.ndim != 1:
            raise ValueError('Expected mono audio')
        result = audio.copy()
        ranges=[]
        for edit in entry['edits']:
            start, end = edit['start'], edit['end']
            left, right = round(edit.get('context_start', 0)*sr), round(edit.get('context_end',len(audio)/sr)*sr)
            clip=torch.from_numpy(audio[left:right])[None]
            clip=torchaudio.functional.resample(clip, sr, 24000)
            rms=float(torch.sqrt(torch.mean(clip.square())))
            gain=max(1, 0.1/max(rms,1e-8))
            with torch.inference_mode():
                mel=model.mel_spec((clip*gain).to(device)).permute(0,2,1)
                mask=torch.ones(mel.shape[:2],dtype=torch.bool,device=device)
                a=round((start-left/sr)*24000/256); b=round((end-left/sr)*24000/256)
                mask[:,a:b]=False
                mel[:,a:b]=0
                gen, trajectory=model.sample(cond=mel,text=convert_char_to_pinyin([edit['text']]),
                    duration=mel.shape[1],steps=32,cfg_strength=2,sway_sampling_coef=-1,seed=edit.get('seed', args.seed),edit_mask=mask)
                del trajectory
                wav=head(backbone(gen.float().permute(0,2,1))).cpu()/gain
                wav=torchaudio.functional.resample(wav,24000,sr)[0].numpy()
            a=round(start*sr);b=round(end*sr)
            local_a=a-left;local_b=b-left
            if local_b>len(wav):
                raise ValueError('Generated audio is shorter than edit range')
            generated=wav[local_a:local_b]
            # 12 ms fades remove waveform-phase seams without shifting original dialogue.
            blend=np.ones(b-a,dtype=np.float32);fade=min(round(sr*.012),len(blend)//2)
            blend[:fade]=np.linspace(0,1,fade);blend[-fade:]=np.linspace(1,0,fade)
            result[a:b]=audio[a:b]*(1-blend)+generated*blend
            ranges.append([start,end])
            print(entry['file'],edit['text'],flush=True)
        output.parent.mkdir(parents=True,exist_ok=True)
        output.write_bytes(replace_pcm(source.read_bytes(),result,ranges,sr))
        check, rate = sf.read(output,dtype='float32')
        assert rate==sr and len(check)==len(audio)
        keep=np.ones(len(audio),dtype=bool)
        for a,b in ranges:keep[round(a*sr):round(b*sr)]=False
        assert np.array_equal(audio[keep],check[keep]),'Original audio changed outside name edits'
        assert np.isfinite(check).all()
        report.append({'file':entry['file'],'sample_rate':sr,'samples':len(audio),'edits':ranges,
                       'outside_edits_identical':True,'sha256':hashlib.sha256(output.read_bytes()).hexdigest()})
    (base/'generation-report.json').write_text(json.dumps(report,indent=2))


if __name__=='__main__':
    main()
