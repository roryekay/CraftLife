// Double-click entry point: uses the existing Node runtime, no shell policy changes.
import {spawn,execFileSync} from 'node:child_process';
import {readFileSync,writeFileSync,existsSync,openSync,closeSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
import dgram from 'node:dgram';
import {setTimeout as delay} from 'node:timers/promises';
import {destination} from '../adapters/common/travel-catalog.mjs';
const root=fileURLToPath(new URL('../',import.meta.url));
const game=process.argv[2];
try {
  if(!['halflife','halflife2'].includes(game))throw Error('Choose halflife or halflife2.');
  if(typeof WebSocket!=='function')throw Error('Node 22 or newer is required.');
  const hl1=game==='halflife',runtime=path.join(root,'local',hl1?'runtime-halflife':'runtime-halflife2-v2');
  const executable=path.join(runtime,hl1?'hl.exe':'hl2.exe');
  if(!existsSync(executable))throw Error('Prepare the isolated native runtime first.');
  const flag=name=>process.argv.find(s=>s.startsWith(`--${name}=`))?.slice(name.length+3);
  const selected=flag('destination');
  const entry=selected===undefined?{map:hl1?'c1a0':'d1_trainstation_01',encounter:0}:destination(game,Number(selected));
  const {map,encounter}=entry;
  const socket=dgram.createSocket('udp4');
  try{await new Promise((resolve,reject)=>{socket.once('error',reject);socket.bind(hl1?25621:25622,'127.0.0.1',resolve);});}finally{socket.close();}
  const replaceGame=flag('replace-game'),replacePid=Number(flag('replace-pid'));
  if(replaceGame!==undefined){
    if(!['halflife','halflife2'].includes(replaceGame)||replaceGame===game||!Number.isInteger(replacePid)||replacePid<=0||replacePid>2147483647)throw Error('Invalid session replacement');
    const recorded=Number(readFileSync(path.join(root,`local/${replaceGame}-native.pid`),'utf8').trim());
    if(recorded!==replacePid)throw Error('The previous game no longer matches this session.');
    const previous=path.join(root,'local',replaceGame==='halflife'?'runtime-halflife/hl.exe':'runtime-halflife2-v2/hl2.exe');
    // Check identity and stop in the same local process. Never stop a Steam/original
    // game, an unrecorded PID, or a process whose executable cannot be verified.
    const literal=previous.replaceAll("'","''");
    execFileSync('powershell.exe',['-NoProfile','-NonInteractive','-Command',`$ErrorActionPreference='Stop'; $owned=Get-CimInstance Win32_Process -Filter 'ProcessId = ${replacePid}'; if(!$owned -or $owned.ExecutablePath -ine '${literal}'){throw 'Native session identity mismatch'}; Stop-Process -Id ${replacePid} -ErrorAction Stop`],{windowsHide:true,stdio:'pipe'});
    const deadline=Date.now()+10000;
    while(Date.now()<deadline){try{process.kill(replacePid,0);}catch(e){if(e.code==='ESRCH')break;throw e;}await delay(100);}
    try{process.kill(replacePid,0);throw Error('The previous game did not close.');}catch(e){if(e.code!=='ESRCH')throw e;}
    // Its relay observes host exit every 100ms and releases the guest owner.
    await delay(350);
  }else if(flag('replace-pid')!==undefined)throw Error('Missing previous game identity');
  for(const name of ['hl.exe','hl2.exe']){
    const listing=execFileSync('tasklist.exe',['/FI',`IMAGENAME eq ${name}`,'/FO','CSV','/NH'],{encoding:'utf8',windowsHide:true});
    if(listing.split(/\r?\n/).some(line=>line.toLowerCase().startsWith(`"${name}"`)))throw Error('Close the running Half-Life game before starting another session.');
  }
  const args=['-game','minecraftify','-windowed','-w','1280','-h','720','-novid','-console','-condebug','+exec','minecraftify.cfg','+map',map];
  if(hl1)args.unshift('-gl','-nofbo');
  const native=spawn(executable,args,{cwd:runtime,stdio:'ignore',detached:true});
  await new Promise((resolve,reject)=>{native.once('spawn',resolve);native.once('error',reject);});
  writeFileSync(path.join(root,`local/${game}-native.pid`),String(native.pid));
  const output=openSync(path.join(root,`local/${game}-relay.log`),'w');
  const errors=openSync(path.join(root,`local/${game}-relay-errors.log`),'w');
  try{
    const relay=spawn(process.execPath,[path.join(root,'adapters/common/relay.mjs'),game,`--host-pid=${native.pid}`,`--encounter=${encounter}`,...(process.argv.includes('--input-trace')?['--input-trace']:[])],{cwd:root,stdio:['ignore',output,errors],detached:true,windowsHide:true});
    await new Promise((resolve,reject)=>{relay.once('spawn',resolve);relay.once('error',reject);});relay.unref();
  }catch(error){native.kill();throw error;}finally{closeSync(output);closeSync(errors);}
  native.unref();console.log(`Started ${game}, map ${map}. Keep Minecraft running; F8 offers both games.`);
}catch(error){console.error(error.message);process.exitCode=1;}
