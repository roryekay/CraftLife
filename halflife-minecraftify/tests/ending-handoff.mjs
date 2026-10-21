// Read-only live acceptance: run against the production relay and owned ending.
import assert from 'node:assert/strict';
import {writeFileSync,readFileSync} from 'node:fs';
const receipt=new URL('../local/voice-patch/ending-handoff-validation.json',import.meta.url);
const resume=process.argv.includes('--resume');
const previous=resume?JSON.parse(readFileSync(receipt)):null;
const phases=[],samples=[];let initial,finished=false;
const ws=new WebSocket('ws://127.0.0.1:25610');
const timer=setTimeout(()=>finish(Error('Timed out waiting for the actual ending and playable landing')),180000);
function finish(error){
  if(finished)return;finished=true;clearTimeout(timer);ws.close();
  if(error){console.error(error.message);process.exitCode=1;return;}
  const report={passed:true,phases,initial,last:samples.at(-1),samples,checked_at:new Date().toISOString()};
  if(resume){previous.resume={passed:true,state:report.last,checked_at:report.checked_at};writeFileSync(receipt,JSON.stringify(previous,null,2));}
  else writeFileSync(receipt,JSON.stringify(report,null,2));
  console.log(`PASS: ${resume?'saved Overworld resumed':'authored door cue, inventory preservation and ordinary Minecraft control'}`);
}
ws.onerror=()=>finish(Error('Minecraft local link unavailable'));
ws.onmessage=e=>{try{
  const m=JSON.parse(e.data);
  if(m.t==='overworld_ready'){phases.push(m.phase);console.log(`Saved Overworld phase ${m.phase}`);}
  if(m.t!=='guest_state')return;
  if(!initial&&m.active)initial={item:m.item,count:m.count,offhand:m.offhand,offhandCount:m.offhandCount};
  if(samples.length<1000)samples.push({active:m.active,dimension:m.dimension,freeMovement:m.freeMovement,noGravity:m.noGravity,noPhysics:m.noPhysics,feet:m.feet,hp:m.hp,item:m.item,count:m.count,offhand:m.offhand,offhandCount:m.offhandCount});
  if((resume||phases.includes(2))&&!m.active&&m.freeMovement){
    assert.equal(m.dimension,'minecraftify:overworld');assert.equal(m.noGravity,false);assert.equal(m.noPhysics,false);
    assert(m.feet[1]>=64&&m.feet[1]<200,`Invalid landing height ${m.feet[1]}`);assert(m.hp>0);
    if(!resume){assert(phases.includes(1));for(const k of ['item','count','offhand','offhandCount'])assert.equal(m[k],initial[k],`Inventory changed: ${k}`);}
    finish();
  }
}catch(error){finish(error);}};
