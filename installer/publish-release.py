"""Publish the explicitly requested package to the existing private CraftLife repo.

Uses the OS Git credential helper in memory; never stores or prints credentials.
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,urllib.request
REPO='roryekay/CraftLife'
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('--commit',required=True);p.add_argument('--tag',default='v0.3.0-test.2');args=p.parse_args()
    if args.archive.name!='CraftLife-'+args.tag.removeprefix('v')+'-windows-x64.zip':raise ValueError('Unexpected release file')
    digest=hashlib.sha256(args.archive.read_bytes()).hexdigest();checksum=args.archive.with_suffix('.zip.sha256')
    if checksum.read_text().split()[0]!=digest:raise ValueError('Archive checksum differs')
    auth=subprocess.run(['git','credential','fill'],input='protocol=https\nhost=github.com\npath='+REPO+'.git\n\n',text=True,capture_output=True,check=True,env=dict(os.environ,GIT_TERMINAL_PROMPT='0',GCM_INTERACTIVE='Never'))
    credentials=dict(line.split('=',1) for line in auth.stdout.splitlines() if '=' in line)
    headers={'Authorization':'Bearer '+credentials['password'],'Accept':'application/vnd.github+json','User-Agent':'CraftLife-private-release','X-GitHub-Api-Version':'2022-11-28'}
    def request(url,data=None,content_type='application/json'):
        h=dict(headers)
        if data is not None:h['Content-Type']=content_type
        with urllib.request.urlopen(urllib.request.Request(url,data=data,headers=h),timeout=120) as r:return json.load(r)
    base='https://api.github.com/repos/'+REPO
    repository=request(base)
    if repository['private'] is not True:raise ValueError('Repository must remain private')
    body='Private Windows build with automatic detection of Half-Life 1 only, Half-Life 2 only, or both. Missing games are skipped; both installed games enable cross-game travel. Approved Steve voices and the G-Man Overworld ending/halo fix are retained.\n\nDownload the ZIP, Extract All, double-click Install.cmd. All bridge runtimes and mods are included. Start a CraftLife profile in the official Minecraft Launcher, which handles sign-in and any Minecraft/Fabric runtime downloads; then use the matching Play button. Requires Minecraft Java and at least one supported owned Steam Half-Life game. No development tools or paid APIs.\n\nInstaller fixtures cover HL1-only, HL2-only, both games, repair, missing-game launch refusal and adding the second game. TEST_RESULTS.md records validation and existing gameplay limits. Original game files, accounts and saves are excluded.\n\nSHA-256: `'+digest+'`'
    release=request(base+'/releases',json.dumps({'tag_name':args.tag,'target_commitish':args.commit,'name':'CraftLife '+args.tag+' — simple Windows installer','body':body,'draft':True,'prerelease':True}).encode())
    uploaded=[]
    for file in (args.archive,checksum):
        url=release['upload_url'].split('{')[0]+'?name='+file.name
        asset=request(url,file.read_bytes(),'application/zip' if file.suffix=='.zip' else 'text/plain')
        if asset['size']!=file.stat().st_size:raise ValueError('Uploaded asset size differs')
        expected='sha256:'+hashlib.sha256(file.read_bytes()).hexdigest()
        if asset.get('digest') and asset['digest']!=expected:raise ValueError('GitHub upload digest differs')
        uploaded.append({'name':asset['name'],'bytes':asset['size'],'digest':asset.get('digest')})
    req=urllib.request.Request(base+'/releases/'+str(release['id']),data=json.dumps({'draft':False}).encode(),headers=dict(headers,**{'Content-Type':'application/json'}),method='PATCH')
    with urllib.request.urlopen(req,timeout=60) as r:release=json.load(r)
    if release['draft']:raise ValueError('Release remained a draft')
    print(json.dumps({'private':repository['private'],'release':release['html_url'],'assets':uploaded},indent=2))
if __name__=='__main__':main()
