// Observer only: use the real native F8 menu for both switches.
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,statSync} from 'node:fs';
import {setTimeout as delay} from 'node:timers/promises';
let latest,pid;const requests=[],checks=[];
const ws=new WebSocket('ws://127.0.0.1:25610');
ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.t==='hello')pid=m.pid;if(m.t==='guest_state')latest=m;if(m.t==='travel_request')requests.push({...m,time:Date.now()});};
async function wait(predicate){const end=Date.now()+300000;while(Date.now()<end){if(predicate())return;await delay(100);}throw Error('Timed out: '+JSON.stringify({latest,requests}));}
function log(game){try{return readFileSync(new URL(`../local/${game}-relay.log`,import.meta.url),'utf8');}catch{return '';}}
function nativePid(game){try{return Number(readFileSync(new URL(`../local/${game}-native.pid`,import.meta.url),'utf8'));}catch{return 0;}}
try{
  await wait(()=>latest?.active&&latest?.control&&latest?.movement&&latest?.wings);
  const initial={pid,item:latest.item,count:latest.count,offhand:latest.offhand,offhandCount:latest.offhandCount};
  const moments=process.argv.includes('--moments');
  const destinations=moments?[['halflife2',0,'d1_trainstation_01',0],['halflife',1,'c1a0',0]]:[['halflife2',32,'d2_coast_11',9],['halflife',23,'c2a5g',2]];
  if(process.argv.includes('--from-hl2'))destinations.reverse();
  console.log('READY_FOR_'+destinations[0][0]);
  for(const [game,index,map,encounter] of destinations){
    const previousPid=nativePid(game);
    await wait(()=>requests.some(r=>r.game===game&&r.index===index));
    const request=requests.find(r=>r.game===game&&r.index===index);
    const arrival=encounter?`Boss travel accepted: ${game}:${map}, encounter ${encounter}`:`Scene ${map}, epoch`;
    await wait(()=>nativePid(game)!==previousPid&&statSync(new URL(`../local/${game}-relay.log`,import.meta.url)).mtimeMs>=request.time&&log(game).includes(arrival)&&latest?.active&&latest?.control&&latest?.movement);
    assert.equal(pid,initial.pid);assert.equal(latest.item,initial.item);assert.equal(latest.count,initial.count);assert(latest.wings);
    assert.equal(latest.offhand,initial.offhand);assert.equal(latest.offhandCount,initial.offhandCount);
    checks.push({game,map,encounter,nativePid:nativePid(game),guestPid:pid,wings:latest.wings,item:latest.item,offhandCount:latest.offhandCount});
    console.log('ARRIVED_'+game);
  }
  const result={passed:true,testedAt:new Date().toISOString(),input:process.argv.includes('--keyboard')?'actual native keyboard':'actual native mouse menu',initial,checks};writeFileSync(new URL(`../local/cross-game-${moments?'menu':'travel'}-acceptance.json`,import.meta.url),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));
}finally{ws.close();}
