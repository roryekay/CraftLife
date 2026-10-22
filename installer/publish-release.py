"""Publish the explicitly requested package to the existing private CraftLife repo.

Uses the OS Git credential helper in memory; never stores or prints credentials.
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,urllib.request
REPO='roryekay/CraftLife'
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('--commit',required=True);p.add_argument('--tag',default='v0.3.0-test.1');args=p.parse_args()
    if args.archive.name!='CraftLife-0.3.0-test.1-windows-x64.zip':raise ValueError('Unexpected release file')
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
    body='Windows private test build matching the current CraftLife gameplay, approved Steve voices and G-Man Overworld ending, including the halo fix.\n\nDownload the ZIP, Extract All, run Install.cmd. Start a CraftLife profile in the official Minecraft Launcher, then use the matching Play button. Steam and owned Half-Life 1, Half-Life 2 and Minecraft Java are required. No development tools or paid APIs.\n\nFresh/repeat installer fixtures passed; all ten voice patches and both ending maps match the approved local outputs. Live installed-runtime smoke checks and limitations are recorded in TEST_RESULTS.md inside the ZIP. Full campaign certification is not claimed. Original game files, accounts and saves are excluded.\n\nSHA-256: `'+digest+'`'
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
