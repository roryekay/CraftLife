// Opt-in tests against the owned, isolated -Diagnostics native runtime.
import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
import {readFileSync} from 'node:fs';
const game=process.argv[2],mode=process.argv[3]??'blocks',api=run(game,{diagnostics:true,movement:process.argv.includes('--minecraft-movement')?true:undefined}),checks=[];
async function wait(p,ms=10000){const end=Date.now()+ms;while(Date.now()<end){const v=p();if(v)return v;await delay(25);}throw Error('Timed out: '+JSON.stringify(api.state()));}
async function key(k,down){api.guest({t:'key',k,down});await delay(100);}
async function select(n,item){api.guest({t:'slot',n});await wait(()=>api.state()?.slot===n&&(!item||api.state()?.item===item));}
async function use(ms=1){await key('use',true);await delay(ms);await key('use',false);}
async function aim(at){await api.native(23,at);await delay(350);}
async function click(ui,x,y){api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:1,modifiers:0});await delay(100);api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:0,modifiers:0});await delay(100);}
async function swapReserve(column,row=0){api.guest({t:'inventory'});const ui=await wait(()=>api.state()?.screen&&api.state()?.inventoryLeft!==undefined?api.state():false);const source=[ui.inventoryLeft+16+column*18,ui.inventoryTop+92+row*18],dest=[ui.inventoryLeft+160,ui.inventoryTop+150];await click(ui,...source);await click(ui,...dest);await click(ui,...source);await wait(()=>api.state()?.carried==='minecraft:air');await key('escape',true);await select(8);}
try{
  await wait(()=>api.state()?.active&&api.state()?.control&&api.state()?.hp>0&&api.pose(),30000);
  if(api.state().screen)await key('escape',true);
  await delay(1500); // Drain the per-map block replay before measuring new input.
  const base=api.pose().f.slice(3,6);
  if(mode==='blocks'||mode==='tnt'){
    const slot=mode==='blocks'?4:6,item=mode==='blocks'?'minecraft:cobblestone':'minecraft:tnt';
    await select(slot,item);let placed;
    for(let n=0;n<8&&!placed;n++){
      const a=n*Math.PI/4,at=[base[0]+Math.cos(a)*2.5,base[1]-.6,base[2]+Math.sin(a)*2.5],start=api.blocks().length,count=api.state().count;
      await aim(at);await use();await delay(500);
      placed=api.blocks().slice(start).filter(e=>!e.air&&e.solid&&e.block===item).at(-1);
      if(placed)assert(api.state().count<count,'Placement did not consume a real Minecraft block');
    }
    assert(placed,'Minecraft placement did not create a block');
    await wait(()=>api.operations().block?.status>=0);await delay(300);
    assert(api.nativeState().i[1]>0,'No native collider was installed for the placed block');
    checks.push({name:'real Minecraft block placement created native collision',pos:placed.pos});
    await aim(placed.pos.map(v=>v+.5));
    if(mode==='blocks'){
      const count=api.nativeState().i[1];await select(1,'minecraft:iron_pickaxe');await key('attack',true);await delay(3200);await key('attack',false);
      await wait(()=>api.blocks().some(e=>e.air&&String(e.pos)===String(placed.pos))&&api.nativeState().i[1]<count);
      checks.push({name:'real pickaxe broke the placed block and removed its native collider'});
    }else{
      let enemy;
      for(let n=0;n<8&&!enemy;n++){const a=n*Math.PI/4,at=[placed.pos[0]+.5+Math.cos(a)*2.15,base[1],placed.pos[2]+.5+Math.sin(a)*2.15];const result=await api.native(20,at);if(result.i[0]===1)enemy=result;}
      assert(enemy,'No safe native blast target location');await delay(350);const enemyBefore=(await api.native(21)).f[0];
      await api.native(24,[100]);await select(7,'minecraft:flint_and_steel');const before=api.counts().explosion;await use();
      const away=[base[0]-(placed.pos[0]+.5-base[0]),base[1]+.1,base[2]-(placed.pos[2]+.5-base[2])];await api.native(26,away).catch(()=>{});
      await wait(()=>api.counts().explosion>before,8000);await wait(()=>api.operations().explosion?.status===1);
      await delay(250);const enemyAfter=(await api.native(21)).f[0];assert(enemyAfter<enemyBefore,'TNT did not damage the real native zombie');
      checks.push({name:'real primed Minecraft TNT exploded and damaged a native zombie',enemyBefore,enemyAfter,nativeHealth:api.nativeState().f[0]});
    }
  }else if(mode==='pearl'){
    await api.native(24,[100]);await select(5,'minecraft:ender_pearl');const before=api.state().count;
    await aim([base[0]+3,base[1]-.5,base[2]]);await use();
    await wait(()=>api.state().count<before);await wait(()=>Math.hypot(...api.pose().f.slice(3,6).map((v,n)=>v-base[n]))>.5,8000);
    checks.push({name:'real thrown ender pearl consumed and moved the native player',from:base,to:api.pose().f.slice(3,6)});
  }else if(mode==='firework'){
    await swapReserve(2);await select(8,'minecraft:firework_rocket');await key('swap',true);await key('swap',false);
    await select(3,'minecraft:crossbow');await aim([base[0]+3,base[1]+.3,base[2]]);
    await use(1600);const before=api.counts().explosion;await use(100);
    await wait(()=>api.counts().explosion>before,8000);await wait(()=>api.operations().explosion?.status===1);
    checks.push({name:'real crossbow firework burst produced native explosion'});
  }else if(mode==='potion'){
    // Swiftness is reserve slot 14 in the original kit.
    await swapReserve(5);await select(8,'minecraft:potion');await use(1900);await wait(()=>api.state().item==='minecraft:glass_bottle');
    checks.push({name:'real swiftness potion completed consumption',nativeAck:api.acks().filter(e=>e.type==='effects').at(-1)});
  }else if(mode==='travel'){
    const catalog=JSON.parse(readFileSync(new URL('../adapters/common/moments.json',import.meta.url)))[game];
    const index=Number(process.argv[4]??catalog.findIndex(e=>e.kind==='boss')),destination=catalog[index];assert(destination);
    const old=api.pose().scene;api.guest({t:'travel_menu'});const ui=await wait(()=>api.state()?.screen?api.state():false);
    const rows=Math.max(1,Math.min(8,Math.floor((ui.uiHeight-108)/22))),page=Math.floor(index/rows);
    for(let n=0;n<page;n++){await click(ui,ui.uiWidth/2+105,ui.uiHeight-14);await delay(100);}
    await click(ui,ui.uiWidth/2,80+(index%rows)*22);
    await wait(()=>api.pose()?.scene!==old&&api.state()?.active,30000);
    await wait(()=>api.operations().change_map?.status===1,30000);
    if(destination.encounter)await wait(()=>api.operations().travel?.status===1,15000);
    if(destination.encounter){
      await delay(game==='halflife2'?2000:250);const boss=await api.native(21,[],[destination.encounter]);console.log('nativeBoss',JSON.stringify(boss));
      if((game==='halflife2'&&destination.encounter===9)||(game==='halflife'&&destination.encounter===2)){
        assert(boss.f[0]>0,'Boss is not alive');if(game==='halflife2')assert(boss.f[7]===0&&!(boss.i[3]&4),'Guard is not unburrowed');
        await aim(boss.f.slice(2,5));await select(3,'minecraft:crossbow');await use(1600);await delay(200);await use(80);
        let after;const end=Date.now()+7000;while(Date.now()<end){after=await api.native(21,[],[destination.encounter]);if(after.f[0]<boss.f[0])break;await delay(150);}
        assert(after.f[0]<boss.f[0],'Crossbow did not damage the native boss');checks.push({name:'real explosive Minecraft crossbow damaged the active native boss',before:boss.f[0],after:after.f[0]});
      }
    }
    checks.push({name:'actual travel menu loaded native map and reconnected Minecraft camera',destination});
  }else if(mode==='milk'){
    await swapReserve(0,1);await select(8,'minecraft:milk_bucket');await use(1900);
    await wait(()=>api.state().item==='minecraft:bucket');
    checks.push({name:'real milk bucket completed consumption'});
  }else if(mode==='third'){
    console.log('READY: press F5 in native game');await wait(()=>api.pose().i[1]===0,25000);
    // Source publishes the camera mode before its next rendered view catches up.
    await wait(()=>Math.hypot(...api.pose().f.slice(0,3).map((v,n)=>v-api.pose().f[10+n]))>.5,3000);
    checks.push({name:'native F5 third-person camera moved behind player while interaction eye stayed separate'});
    console.log('THIRD: inspect avatar, then press F5');await wait(()=>api.pose().i[1]===1,25000);
  }else throw Error('Unknown feature test');
  if(process.argv.includes('--inspect')){console.log('INSPECT: keeping the real guest connected for visual verification');await delay(20000);}
  console.log(JSON.stringify({passed:true,game,mode,checks},null,2));
}catch(error){console.error(JSON.stringify({passed:false,game,mode,checks,blocks:api.blocks(),counts:api.counts(),operations:api.operations(),pose:api.pose(),error:error.stack},null,2));process.exitCode=1;}
finally{api.guest({t:'key',k:'attack',down:false});api.guest({t:'key',k:'use',down:false});if(mode==='tnt')await api.native(22).catch(()=>{});api.close();}
