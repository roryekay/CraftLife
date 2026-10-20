import assert from 'node:assert/strict';
import {readFileSync,writeFileSync} from 'node:fs';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {destination} from '../adapters/common/travel-catalog.mjs';
const catalog=JSON.parse(readFileSync(new URL('../adapters/common/moments.json',import.meta.url),'utf8'));
for(const game of ['halflife','halflife2'])for(let i=0;i<catalog[game].length;i++)assert.deepEqual(destination(game,i),catalog[game][i]);
for(const [game,index] of [['__proto__',0],['halflife;quit',0],['halflife',-1],['halflife',.5],['halflife','1'],['halflife2',Infinity],['halflife2',999]])assert.throws(()=>destination(game,index));
const start=fileURLToPath(new URL('../tools/start-native.mjs',import.meta.url));
for(const args of [['halflife2','--destination=999'],['halflife2','--replace-game=halflife','--replace-pid=-1'],['halflife2','--replace-game=halflife',`--replace-pid=${process.pid}`]]){
  const child=spawnSync(process.execPath,[start,...args],{encoding:'utf8',windowsHide:true});
  assert.equal(child.status,1,child.stdout+child.stderr);
  assert.match(child.stderr,/Invalid travel destination|Invalid session replacement|no longer matches this session/);
}
console.log('PASS: both complete catalogs, invalid destinations, command injection, and unowned process replacement refused');
if(process.argv.includes('--identity-guard')){
  // Run only with both native games closed. A matching PID record alone must
  // never authorize stopping a different executable (including a reused PID).
  const record=new URL('../local/halflife-native.pid',import.meta.url),previous=readFileSync(record);
  try{
    writeFileSync(record,String(process.pid));
    const child=spawnSync(process.execPath,[start,'halflife2','--replace-game=halflife',`--replace-pid=${process.pid}`],{encoding:'utf8',windowsHide:true});
    assert.equal(child.status,1);assert.match(child.stderr,/Native session identity mismatch/);process.kill(process.pid,0);
    console.log('PASS: matching PID with wrong executable is refused and remains alive');
  }finally{writeFileSync(record,previous);}
}
