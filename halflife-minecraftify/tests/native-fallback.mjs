import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
const game=process.argv[2],api=run(game,{diagnostics:true,movement:true});
async function wait(p){const end=Date.now()+20000;while(Date.now()<end){if(p())return;await delay(25);}throw Error('Timed out: '+JSON.stringify(api.state()));}
const feet=()=>api.pose().f.slice(3,6);
try{
 await wait(()=>api.state()?.movement&&api.state()?.control&&api.state()?.hp>0&&api.movementState()?.i[0]);
 api.setMovement(false);await wait(()=>api.state()?.movement===false);await delay(300);
 const mode=await api.native(21,[],[98]);assert.equal(mode.i[7],0);assert.notEqual(mode.i[6],0,'Engine remained in MOVETYPE_NONE'); // Source may not expose this key; real movement is checked below.
 const from=feet();let to,moved=false;
 for(let n=0;n<8&&!moved;n++){
  const a=n*Math.PI/4,heading=[from[0]+Math.cos(a)*3,api.pose().f[1],from[2]+Math.sin(a)*3];
  await api.native(23,heading);await api.native(25,[250],[1]);await delay(550);to=feet();moved=Math.hypot(to[0]-from[0],to[2]-from[2])>.15;
 }
 assert(moved,'Engine movement did not respond in any direction');api.setMovement(true);await wait(()=>api.state()?.movement===true);
 console.log(JSON.stringify({passed:true,game,moveType:mode.i[6],from,to,checks:['Native locomotion mode restored','Bounded native forward input moved the player on standing ground','Minecraft locomotion reattached']},null,2));
}catch(error){console.error(JSON.stringify({passed:false,game,state:api.state(),pose:api.pose(),error:error.stack},null,2));process.exitCode=1;}finally{api.close();}
