import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
const api=run('halflife2',{diagnostics:true,movement:true});
async function wait(predicate){const end=Date.now()+60000;while(Date.now()<end){if(predicate())return;await delay(25);}throw Error('Platform fixture not ready: '+JSON.stringify(api.state()));}
try{
  await wait(()=>api.state()?.control&&api.state()?.hp>0&&api.movementState()?.i[0]);
  if(api.state().screen){api.guest({t:'key',k:'escape',down:true});await delay(300);}
  assert.equal((await api.native(24,[100])).i[0],1);
  assert.equal((await api.native(29,[],[1])).i[0],1,'No clear space for elevated moving platform');
  await delay(800);
  const samples=[];
  for(let n=0;n<25;n++){const p=await api.native(29,[],[2]);assert.equal(p.i[0],1);assert.equal(p.i[2],1,'Player lost the platform support handle: '+JSON.stringify(p));assert(p.f[0]<.02,'Feet did not follow translated/rotated platform: '+JSON.stringify(p));samples.push({error:p.f[0],feet:p.f.slice(2,5),expected:p.f.slice(5,8)});await delay(120);}
  assert(Math.hypot(...samples.at(-1).feet.map((v,n)=>v-samples[0].feet[n]))>.25,'Fixture did not move');
  api.guest({t:'key',k:'jump',down:true});await delay(100);api.guest({t:'key',k:'jump',down:false});await delay(100);
  const detached=await api.native(29,[],[2]);assert.equal(detached.i[2],0,'Jump did not detach from moving support');
  assert(detached.f[3]>samples.at(-1).feet[1]+.3,'Jump did not rise from the moving platform');
  assert.equal((await api.native(29,[],[0])).i[0],1);await wait(()=>api.movementState()?.i[0]===1);
  console.log(JSON.stringify({passed:true,game:'halflife2',checks:['real native collider translates, rises and rotates','Minecraft-driven feet stay attached without double movement','jump detaches from support','support removal returns to native ground'],maxError:Math.max(...samples.map(s=>s.error)),first:samples[0],last:samples.at(-1)},null,2));
}finally{await api.native(29,[],[0]).catch(()=>{});api.close();}
