import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
const game=process.argv[2],api=run(game,{diagnostics:true});
async function wait(p,ms=12000){const end=Date.now()+ms;while(Date.now()<end){const v=p();if(v)return v;await delay(25);}throw Error('Timed out: '+JSON.stringify(api.state()));}
async function click(ui,x,y){api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:1,modifiers:0});await delay(100);api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:0,modifiers:0});await delay(100);}
try{
  await wait(()=>api.state()?.control&&api.pose(),30000);if(api.state().screen){api.guest({t:'key',k:'escape',down:true});await delay(300);}
  await api.native(24,[50]);await delay(300);const first=api.vitalJumps().length;
  api.guest({t:'inventory'});const ui=await wait(()=>api.state()?.screen&&api.state()?.inventoryLeft!==undefined?api.state():false);
  const source=[ui.inventoryLeft+70,ui.inventoryTop+92],dest=[ui.inventoryLeft+160,ui.inventoryTop+150];await click(ui,...source);await click(ui,...dest);await click(ui,...source);await wait(()=>api.state().carried==='minecraft:air');
  api.guest({t:'key',k:'escape',down:true});api.guest({t:'slot',n:8});await wait(()=>api.state().item==='minecraft:potion'&&!api.state().screen);
  api.guest({t:'key',k:'use',down:true});await delay(2000);api.guest({t:'key',k:'use',down:false});await wait(()=>api.state().item==='minecraft:glass_bottle');
  const jumps=api.vitalJumps().slice(first);assert(jumps.some(v=>Math.abs(v-20)<.5),'A correctly scaled +20 native instant-healing step was not observed: '+jumps);
  console.log(JSON.stringify({passed:true,game,nativeHealth:api.nativeState().f[0],nativeHealingSteps:jumps},null,2));
}catch(error){console.error(JSON.stringify({passed:false,game,error:error.stack,state:api.state(),steps:api.vitalJumps()},null,2));process.exitCode=1;}
finally{api.guest({t:'key',k:'use',down:false});api.close();}
