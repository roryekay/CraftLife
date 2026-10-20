// Uses the fresh real item kit, native movement and native burn damage fixtures.
import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
const game=process.argv[2],api=run(game,{diagnostics:true}),checks=[];
async function wait(p,ms=12000){const end=Date.now()+ms;while(Date.now()<end){const v=p();if(v)return v;await delay(25);}throw Error('Timed out: '+JSON.stringify(api.state()));}
async function click(ui,x,y){api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:1,modifiers:0});await delay(100);api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:0,modifiers:0});await delay(100);}
async function drink(slot,item='minecraft:potion'){
  api.guest({t:'inventory'});const ui=await wait(()=>api.state()?.screen&&api.state()?.inventoryLeft!==undefined?api.state():false);
  const source=[ui.inventoryLeft+16+((slot-9)%9)*18,ui.inventoryTop+92+Math.floor((slot-9)/9)*18],dest=[ui.inventoryLeft+160,ui.inventoryTop+150];
  await click(ui,...source);await click(ui,...dest);await click(ui,...source);await wait(()=>api.state().carried==='minecraft:air');
  api.guest({t:'key',k:'escape',down:true});api.guest({t:'slot',n:8});await wait(()=>api.state().item===item&&!api.state().screen);
  api.guest({t:'key',k:'use',down:true});await delay(2000);api.guest({t:'key',k:'use',down:false});await wait(()=>api.state().item!==(item));await delay(650);
}
let base,heading;
async function restore(){const reply=await api.native(26,base);assert.equal(reply.i[0],1);await api.native(23,heading);await delay(400);}
async function walk(){await restore();const start=api.pose().f.slice(3,6);await api.native(25,[350],[1]);await delay(700);const end=api.pose().f.slice(3,6);const distance=Math.hypot(end[0]-start[0],end[2]-start[2]);await restore();return distance;}
async function jump(){await restore();const ground=api.pose().f[4];await api.native(25,[100],[3]);let peak=ground;const end=Date.now()+1700;while(Date.now()<end){peak=Math.max(peak,api.pose().f[4]);await delay(15);}await restore();return peak-ground;}
try{
  await wait(()=>api.state()?.control&&api.pose(),30000);if(api.state().screen){api.guest({t:'key',k:'escape',down:true});await delay(300);}await delay(1500);
  base=api.pose().f.slice(3,6);
  for(let n=0;n<8;n++){
    const a=n*Math.PI/4,end=[base[0]+Math.cos(a)*6,base[1]+1,base[2]+Math.sin(a)*6];
    const ray=await api.native(28,[base[0],base[1]+1,base[2],...end],[200000+n,4]);
    if(ray.i[0]===0){heading=[end[0],base[1]+2,end[2]];break;}
  }
  assert(heading,'No clear native walking test path');
  const normalWalk=((await walk())+(await walk()))/2,normalJump=await jump();assert(normalWalk>.5&&normalJump>.5);
  await drink(14);const fastWalk=((await walk())+(await walk()))/2;assert(fastWalk>normalWalk*1.12,`Speed did not increase: ${normalWalk} -> ${fastWalk}`);
  checks.push({name:'real swiftness potion increased native walking distance',normalWalk,fastWalk});
  await drink(15);const highJump=await jump();assert(highJump>normalJump*1.2,`Jump did not increase: ${normalJump} -> ${highJump}`);
  checks.push({name:'real leaping potion increased native jump height',normalJump,highJump});
  await api.native(24,[100]);await api.native(24,[15],[1]);await delay(250);const burned=api.nativeState().f[0];assert(burned<100,'Native burn fixture did not damage the player');
  await drink(16);await api.native(24,[100]);await delay(250);await api.native(24,[15],[1]);await delay(250);assert.equal(api.nativeState().f[0],100,'Fire resistance did not block native burn damage');
  checks.push({name:'real fire-resistance potion blocked native burn damage',unprotectedHealth:burned,protectedHealth:api.nativeState().f[0]});
  await drink(18,'minecraft:milk_bucket');const restoredWalk=await walk(),restoredJump=await jump();
  assert(Math.abs(restoredWalk-normalWalk)<normalWalk*.25&&Math.abs(restoredJump-normalJump)<normalJump*.25,'Milk did not clear native movement effects');
  checks.push({name:'real milk consumption cleared speed and jump effects in native physics',restoredWalk,restoredJump});
  console.log(JSON.stringify({passed:true,game,checks},null,2));
}catch(error){console.error(JSON.stringify({passed:false,game,checks,state:api.state(),pose:api.pose(),error:error.stack},null,2));process.exitCode=1;}
finally{api.guest({t:'key',k:'use',down:false});await api.native(25,[50],[0]).catch(()=>{});api.close();}
