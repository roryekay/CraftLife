import {run} from '../adapters/common/relay.mjs';
import {setTimeout as delay} from 'node:timers/promises';
const api=run(process.argv[2],{diagnostics:true});
try{
  for(let n=0;n<120&&!api.state()?.control;n++)await delay(250);
  if(api.state()?.screen){api.guest({t:'key',k:'escape',down:true});await delay(400);}
  console.log('start',JSON.stringify(api.state()));
  console.log('pose',JSON.stringify(api.pose()));
  if(process.argv[3]==='boss')console.log('boss',JSON.stringify(await api.native(21,[],[9])));
  if(process.argv[3]==='bow'){
    api.guest({t:'slot',n:2});await delay(400);api.guest({t:'key',k:'use',down:true});
    await delay(700);console.log('held',JSON.stringify(api.state()));await delay(700);
    api.guest({t:'key',k:'use',down:false});await delay(700);console.log('released',JSON.stringify(api.state()));
    console.log('events',api.counts(),api.operations());
  }else await delay(10000);
}finally{api.close();}
