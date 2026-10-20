// Observer only: actual native-window F6 inputs drive this acceptance check.
import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
let latest;
const ws=new WebSocket('ws://127.0.0.1:25610');
ws.onmessage=e=>{const m=JSON.parse(e.data);if(m.t==='guest_state')latest=m;};
async function wait(p){const end=Date.now()+45000;while(Date.now()<end){if(p())return;await delay(30);}throw Error('Timed out: '+JSON.stringify(latest));}
try{
  await wait(()=>latest?.active&&latest?.control&&latest?.movement);console.log('READY_F6');
  await wait(()=>latest?.movement===false);console.log('RETURN_F6');
  await wait(()=>latest?.movement===true);assert(latest.wings);
  console.log(JSON.stringify({passed:true,checks:['actual F6 selects native movement','actual F6 restores Minecraft movement with equipped elytra']},null,2));
}finally{ws.close();}
