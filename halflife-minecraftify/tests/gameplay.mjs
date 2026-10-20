// Explicit opt-in integration acceptance against an isolated -Diagnostics runtime.
// Uses real Minecraft inputs/items; only native fixtures are diagnostic operations.
import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
const game=process.argv[2],api=run(game,{diagnostics:true}),checks=[];
let cube,lastSnapshot;
async function wait(predicate,timeout=10000){const end=Date.now()+timeout;while(Date.now()<end){const result=predicate();if(result)return result;await delay(25);}throw Error('Gameplay state timed out: '+JSON.stringify(api.state()));}
async function select(n,item){api.guest({t:'slot',n});await wait(()=>api.state()?.slot===n&&(!item||api.state().item===item));await delay(100);}
async function key(k,down){api.guest({t:'key',k,down});await delay(80);}
async function use(ms){await key('use',true);await delay(ms);await key('use',false);await delay(150);}
async function inspect(){lastSnapshot=await api.native(21);return lastSnapshot;}
async function damageAfter(before){const end=Date.now()+7000;while(Date.now()<end){const snapshot=await inspect();if(snapshot.f[0]<before)return snapshot;await delay(100);}throw Error('Real native enemy health did not decrease');}
async function click(ui,x,y){api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:1,modifiers:0});await delay(80);api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:0,modifiers:0});await delay(80);}
async function equipPotion(column){
  api.guest({t:'inventory'});const ui=await wait(()=>api.state()?.screen&&api.state()?.inventoryLeft!==undefined?api.state():false);
  const source=[ui.inventoryLeft+16+column*18,ui.inventoryTop+92],destination=[ui.inventoryLeft+160,ui.inventoryTop+150];
  await click(ui,...source);await click(ui,...destination);await click(ui,...source);
  await wait(()=>api.state()?.carried==='minecraft:air');await key('escape',true);await select(8,'minecraft:potion');
}
try{
  await wait(()=>api.state()?.active&&api.state()?.control&&api.state()?.hp>0&&api.pose(),30000);
  if(api.state().screen){await key('escape',true);await wait(()=>!api.state().screen);}
  const base=api.pose().f.slice(3,6);
  if(!process.argv.includes('--physics-only')){
  let target;
  for(let n=0;n<8;n++){
    const a=n*Math.PI/4,at=[base[0]+Math.cos(a)*2.15,base[1],base[2]+Math.sin(a)*2.15];
    const reply=await api.native(20,at);if(reply.i[0]===1){
      await delay(300);const current=await inspect(),eye=api.pose().f.slice(10,13),center=current.f.slice(2,5);
      const trace=await api.native(28,[...eye,...center],[100000+n,4]);
      if(Math.hypot(...center.map((v,k)=>v-eye[k]))<2.5&&trace.i[1]===current.i[1]){target=current;break;}
    }
  }
  assert(target,'No safe native test enemy location was found');
  await api.native(23,target.f.slice(2,5));await delay(350);
  await select(0,'minecraft:iron_sword');
  let before=(await inspect()).f[0];await key('attack',true);await key('attack',false);
  let hit=await damageAfter(before);checks.push({name:'real native zombie damaged by Minecraft sword',before,after:hit.f[0],nativeCrowbarSkill:hit.f[5]});
  await select(2,'minecraft:bow');const bowTarget=await inspect();await api.native(23,bowTarget.f.slice(2,5));await delay(350);before=bowTarget.f[0];const arrows=api.counts().projectile;
  await use(1300);hit=await damageAfter(before);assert(api.counts().projectile>arrows);
  assert(before-hit.f[0]>hit.f[6]*.7,'Charged Power V bow did not exceed ordinary zombie bullet damage');
  checks.push({name:'real Minecraft bow projectile damages native zombie',before,after:hit.f[0]});
  await select(3,'minecraft:crossbow');const crossbowTarget=await inspect();await api.native(23,crossbowTarget.f.slice(2,5));await delay(350);before=crossbowTarget.f[0];
  await use(1600);await delay(200);await use(80);hit=await damageAfter(before);
  checks.push({name:'real crossbow shot damages native zombie through explosive impact',before,after:hit.f[0]});
  await api.native(22);
  if(!process.argv.includes('--combat-only')){
  // A completed potion must consume its stack AND produce a native healing step.
  await api.native(24,[50]);await delay(350);await equipPotion(3);
  const healIndex=api.vitalJumps().length;
  await use(1900);await wait(()=>api.state()?.item==='minecraft:glass_bottle');
  assert(api.vitalJumps().slice(healIndex).some(value=>value>=15),'No native instant-healing step was observed');
  checks.push({name:'healing potion consumed into bottle and healed native player',nativeHealth:api.nativeState().f[0]});
  }
  }
  if(!process.argv.includes('--combat-only')){
  // Native physics acceptance: support, jump and walking off a full cube.
  const original=api.pose().f.slice(3,6);
  let support;
  for(let n=0;n<8;n++){
    const a=n*Math.PI/4,pos=[Math.floor(original[0]+Math.cos(a)*2),Math.floor(original[1]),Math.floor(original[2]+Math.sin(a)*2)];
    const placed=await api.native(27,[],[...pos,1]);if(placed.i[0]!==1)continue;cube=pos;
    const top=[pos[0]+.5,pos[1]+1.04,pos[2]+.5];const moved=await api.native(26,top);
    if(moved.i[0]===1){support={pos,top};break;}
    await api.native(27,[],[...pos,0]);cube=null;
  }
  assert(support,'No standing location was found for the native cube test');
  await delay(550);assert(Math.abs(api.pose().f[4]-(support.pos[1]+1))<.12,'Native player did not stand on the cube');
  const standing=api.pose().f[4];await api.native(25,[120],[3]);
  let peak=standing;const end=Date.now()+1600;while(Date.now()<end){peak=Math.max(peak,api.pose().f[4]);await delay(20);}
  assert(peak>standing+.12,'Native jump did not leave the cube');
  // A nearby wall can block one direction. Check for an actual open edge;
  // restoring the same cube top each attempt keeps the support test meaningful.
  let walkedOff=false;
  for(let n=0;n<8&&!walkedOff;n++){
    if(n){await api.native(26,support.top);await delay(400);}
    const a=n*Math.PI/4,feet=api.pose().f.slice(3,6);
    await api.native(23,[feet[0]+Math.cos(a)*4,api.pose().f[1],feet[2]+Math.sin(a)*4]);await api.native(25,[450],[1]);await delay(900);
    walkedOff=api.pose().f[4]<standing-.15;
  }
  assert(walkedOff,'No clear native walk-off edge was found for this fixture');
  checks.push({name:'native player stood/jumped/walked off Minecraft cube collision',standing,peak,afterWalkingOff:api.pose().f[4]});
  await api.native(27,[],[...cube,0]);cube=null;await api.native(26,original);await delay(350);
  }
  console.log(JSON.stringify({passed:true,game,checks,remaining:'TNT, fireworks, pearls, potion movement/expiry, third-person camera and boss-jump acceptance are separate checks.'},null,2));
}catch(error){console.error(JSON.stringify({passed:false,game,checks,counts:api.counts(),operations:api.operations(),enemy:lastSnapshot?.f,pose:api.pose()?.f,error:error.stack},null,2));process.exitCode=1;}
finally{
  api.guest({t:'key',k:'attack',down:false});api.guest({t:'key',k:'use',down:false});
  await api.native(25,[50],[0]).catch(()=>{});if(cube)await api.native(27,[],[...cube,0]).catch(()=>{});await api.native(22).catch(()=>{});api.close();
}
