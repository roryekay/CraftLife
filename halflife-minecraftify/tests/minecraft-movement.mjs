// Real Minecraft travel + native swept collision, with opt-in native fixtures.
import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
const game=process.argv[2],api=run(game,{diagnostics:true,movement:true}),checks=[];
async function wait(p,ms=12000){const end=Date.now()+ms;while(Date.now()<end){const v=p();if(v)return v;await delay(25);}throw Error('Timed out: '+JSON.stringify({state:api.state(),movement:api.movementState(),pose:api.pose()?.f}));}
async function key(k,down){api.guest({t:'key',k,down});await delay(100);}
async function click(ui,x,y){api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:1,modifiers:0});await delay(100);api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:0,modifiers:0});await delay(100);}
const feet=()=>api.pose().f.slice(3,6);
const horizontal=(a,b)=>Math.hypot(a[0]-b[0],a[2]-b[2]);
let origin,heading;
async function restore(){await key('forward',false);await key('sprint',false);const r=await api.native(26,origin);assert.equal(r.i[0],1);await api.native(23,heading);await delay(700);}
async function walk(sprint){await restore();if(sprint)await key('sprint',true);const start=feet();await key('forward',true);await delay(800);await key('forward',false);await key('sprint',false);const end=feet();return horizontal(start,end);}
try{
  await wait(()=>api.state()?.movement&&api.state()?.control&&api.state()?.hp>0&&api.pose(),30000);if(api.state().screen){await key('escape',true);await delay(350);}
  if(process.argv.includes('--boss-area')){
    // Real invisibility protects this movement fixture from the campaign boss.
    api.guest({t:'inventory'});const ui=await wait(()=>api.state()?.inventoryLeft!==undefined?api.state():false);
    const x=ui.inventoryLeft+160;await click(ui,x,ui.inventoryTop+92);await click(ui,x,ui.inventoryTop+150);await click(ui,x,ui.inventoryTop+92);
    await key('escape',true);api.guest({t:'slot',n:8});await wait(()=>api.state().item==='minecraft:potion');await key('use',true);await delay(1900);await key('use',false);await wait(()=>api.state().item==='minecraft:glass_bottle');
    api.guest({t:'travel_menu'});const menu=await wait(()=>api.state()?.screen?api.state():false),index=23,rows=Math.max(1,Math.min(8,Math.floor((menu.uiHeight-108)/22))),old=api.pose().scene;
    for(let n=0;n<Math.floor(index/rows);n++)await click(menu,menu.uiWidth/2+105,menu.uiHeight-14);
    await click(menu,menu.uiWidth/2,80+(index%rows)*22);
    await wait(()=>api.pose()?.scene!==old&&api.operations().travel?.status===1,30000);api.guest({t:'slot',n:0});await wait(()=>api.state().item==='minecraft:iron_sword');
    checks.push({name:'real boss-area travel preserved Minecraft movement authority'});
  }
  await delay(2000);await wait(()=>api.movementState()?.i[0]===1);
  api.guest({t:'slot',n:0});await wait(()=>api.state().slot===0&&api.state().item==='minecraft:iron_sword');
  assert(api.state().wings,'Real elytra is not equipped');assert.equal(api.state().offhand,'minecraft:firework_rocket');
  origin=feet();let clear=false;
  for(let n=0;n<12;n++){
    const a=n*Math.PI/6,to=[origin[0]+Math.cos(a)*10,origin[1]+1,origin[2]+Math.sin(a)*10];
    const ray=await api.native(28,[origin[0],origin[1]+1,origin[2],...to],[50000+n,4]);
    if(ray.i[0]===0){heading=[to[0],origin[1]+1.62,to[2]];clear=true;break;}
  }
  assert(clear,'No open native walking direction for this fixture');
  const normal=await walk(false),sprint=await walk(true);console.log('walk',JSON.stringify({normal,sprint}));assert(normal>1,'Minecraft forward input did not move the native player');assert(sprint>normal*1.12,'Minecraft sprint did not increase native travel');
  checks.push({name:'actual Minecraft walking and sprint acceleration move the native player',normal,sprint});
  await restore();await key('sneak',true);await wait(()=>api.state().crouching);await key('sneak',false);checks.push({name:'actual Minecraft crouching follows native-window input'});
  await restore();const floor=feet()[1];await key('jump',true);await key('jump',false);let peak=floor;
  const jumpEnd=Date.now()+1500;while(Date.now()<jumpEnd){peak=Math.max(peak,feet()[1]);await delay(20);}assert(peak>floor+.8&&peak<floor+1.7,'Jump did not have Minecraft height');
  await wait(()=>api.movementState()?.i[0]===1);checks.push({name:'real Minecraft jump rises and lands against native ground',floor,peak});
  await restore();const air=[origin[0],origin[1]+6,origin[2]];assert.equal((await api.native(26,air)).i[0],1,'No clear air space above native fixture');
  await wait(()=>api.movementState()?.i[0]===0);await api.native(23,[heading[0],air[1]+4,heading[2]]);await delay(200);
  await key('jump',true);await key('jump',false);await wait(()=>api.state()?.gliding,2500);
  const before=feet(),beforeSpeed=Math.hypot(...api.state().velocity),rockets=api.state().offhandCount;
  await key('use',true);await key('use',false);await wait(()=>api.state().offhandCount<rockets);await delay(850);
  const after=feet(),afterSpeed=Math.hypot(...api.state().velocity);
  assert(afterSpeed>beforeSpeed*1.15,'Real firework did not accelerate elytra flight');assert(horizontal(before,after)>2,'Gliding did not move the native player');assert.equal(api.counts().explosion,0,'Attached flight rocket caused a native blast');
  checks.push({name:'equipped real elytra glides; consumed firework accelerates native flight without a weapon blast',before,after,beforeSpeed,afterSpeed,rocketsBefore:rockets,rocketsAfter:api.state().offhandCount});
  api.setMovement(false);await wait(()=>!api.state()?.movement);await delay(250);const fallback=await api.native(21,[],[98]);assert.equal(fallback.i[7],0,'Native movement ownership was not restored');
  assert.notEqual(fallback.i[6],0,'Native player remained in MOVETYPE_NONE'); // -1 means public getter unavailable.
  checks.push({name:'native movement fallback restores engine movement ownership',moveType:fallback.i[6]});
  console.log(JSON.stringify({passed:true,game,checks},null,2));
}catch(error){console.error(JSON.stringify({passed:false,game,checks,state:api.state(),movement:api.movementState(),pose:api.pose(),error:error.stack},null,2));process.exitCode=1;}
finally{for(const k of ['forward','back','left','right','jump','sneak','sprint','use'])api.guest({t:'key',k,down:false});api.close();}
