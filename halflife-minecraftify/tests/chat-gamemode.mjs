import assert from 'node:assert/strict';
import {setTimeout as delay} from 'node:timers/promises';
import {run} from '../adapters/common/relay.mjs';
const api=run(process.argv[2],{diagnostics:true,movement:true});
async function wait(predicate,ms=15000){const end=Date.now()+ms;while(Date.now()<end){if(predicate())return;await delay(25);}throw Error('Chat state timeout: '+JSON.stringify(api.state()));}
async function command(text,mode){
  api.guest({t:'chat'});await wait(()=>api.state()?.chat);
  for(const char of text)api.guest({t:'text',code:char.codePointAt(0),modifiers:0,character:true});
  api.guest({t:'text',code:13,modifiers:0,character:false});
  await wait(()=>!api.state()?.screen&&api.state()?.gamemode===mode);
}
try{
  await wait(()=>api.state()?.control&&api.state()?.hp>0,60000);
  await api.native(24,[100]);
  if(api.state().screen){api.guest({t:'key',k:'escape',down:true});await wait(()=>!api.state()?.screen);}
  const before=api.state().gamemode;
  await command('/gamemode creative','creative');
  await command('/gamemode survival','survival');
  api.guest({t:'inventory'});await wait(()=>api.state()?.screen&&!api.state()?.chat);
  api.guest({t:'key',k:'escape',down:true});await wait(()=>!api.state()?.screen);
  console.log(JSON.stringify({passed:true,game:process.argv[2],before,checks:['real Minecraft chat submits creative command','real Minecraft chat submits survival command','inventory remains separately accessible']},null,2));
}finally{api.close();}
