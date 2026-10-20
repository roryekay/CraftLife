import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {readFileSync} from 'node:fs';
import dgram from 'node:dgram';
import {run,encode,decode} from '../adapters/common/relay.mjs';
const game=process.argv[2],api=run(game,{diagnostics:true}),checks=[];
const log=new URL('../local/runtime-halflife2-v2/minecraftify/console.log',import.meta.url);
async function wait(p,ms=12000){const end=Date.now()+ms;while(Date.now()<end){const v=p();if(v)return v;await delay(25);}throw Error('Timed out: '+JSON.stringify(api.state()));}
async function click(ui,x,y){api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:1,modifiers:0});await delay(100);api.guest({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:0,modifiers:0});await delay(100);}
let closed=false;
try{
  await wait(()=>api.state()?.control&&api.pose(),30000);if(api.state().screen){api.guest({t:'key',k:'escape',down:true});await delay(300);}
  const offset=game==='halflife2'?readFileSync(log).length:0;
  const resume=process.argv.includes('--resume');
  if(!resume){
  if(game==='halflife')assert.equal((await api.native(21,[],[99])).i[1],0,'Fixture player was already notarget');
  api.guest({t:'inventory'});const ui=await wait(()=>api.state()?.screen&&api.state()?.inventoryLeft!==undefined?api.state():false);
  const x=ui.inventoryLeft+160;await click(ui,x,ui.inventoryTop+92);await click(ui,x,ui.inventoryTop+150);await click(ui,x,ui.inventoryTop+92);await wait(()=>api.state().carried==='minecraft:air');
  api.guest({t:'key',k:'escape',down:true});api.guest({t:'slot',n:8});await wait(()=>api.state().item==='minecraft:potion'&&!api.state().screen);
  api.guest({t:'key',k:'use',down:true});await delay(2000);api.guest({t:'key',k:'use',down:false});await wait(()=>api.state().item==='minecraft:glass_bottle');await delay(650);
  }else await delay(1000);
  if(game==='halflife')assert.equal((await api.native(21,[],[99])).i[1],1,'Actual native FL_NOTARGET was not set');
  else await wait(()=>/notarget ON/i.test(readFileSync(log).subarray(offset).toString()));
  checks.push('real invisibility potion enabled native NPC target protection');
  const scene=api.nativeState().scene;api.close();closed=true;await delay(1600);
  if(game==='halflife'){
    const socket=dgram.createSocket('udp4');
    try{
      await new Promise((resolve,reject)=>{socket.once('error',reject);socket.bind(25621,'127.0.0.1',resolve);});
      const result=new Promise((resolve,reject)=>{const timer=setTimeout(()=>reject(Error('Native release inspection timed out')),2500);socket.on('message',data=>{const p=decode(data);if(p.kind===100&&p.sequence===1){clearTimeout(timer);resolve(p);}});});
      socket.send(encode(9,0,scene),25611,'127.0.0.1');socket.send(encode(21,1,scene,[],[99]),25611,'127.0.0.1');
      assert.equal((await result).i[1],0,'Native target protection remained after disconnect');
    }finally{socket.close();}
  }else await wait(()=>/notarget OFF/i.test(readFileSync(log).subarray(offset).toString()));
  checks.push('adapter disconnect cleared native target protection');
  console.log(JSON.stringify({passed:true,game,checks},null,2));
}catch(error){console.error(JSON.stringify({passed:false,game,checks,error:error.stack,state:api.state()},null,2));process.exitCode=1;}
finally{if(!closed){api.guest({t:'key',k:'use',down:false});api.close();}}
