// Node 22+ built-in WebSocket. No npm packages or host game required.
import { setTimeout as delay } from 'node:timers/promises';
const port=Number(process.env.MINECRAFTIFY_PORT??25610);
const seconds=Number(process.argv[2]??30);
if(!Number.isFinite(seconds)||seconds<1||seconds>3600)throw Error('Use: node tools/mock-host.mjs [seconds 1..3600]');
const socket=new WebSocket(`ws://127.0.0.1:${port}`);
let attached=false,frame=0,timer;
socket.onmessage=e=>{
  const m=JSON.parse(e.data);
  if(m.t==='hello')socket.send(JSON.stringify({t:'attach',v:1}));
  else if(m.t==='attached') {
    if(!m.ok){console.error('Another adapter owns the guest');socket.close();process.exitCode=1;return;}
    attached=true;console.log('Attached mock host. Camera is fixed at [0,65.62,0], looking +Z.');
    timer=setInterval(()=>socket.send(JSON.stringify({t:'cam',f:++frame,p:[0,65.62,0],pl:[0,64,0],r:[0,0,0],fov:70,fp:true,ctl:true})),16);
  } else if(m.t!=='guest_state')console.log(JSON.stringify(m));
};
socket.onerror=()=>{console.error('Start the isolated Minecraft guest world first.');process.exitCode=1;};
socket.onclose=()=>{clearInterval(timer);if(!attached)process.exitCode=1;};
await delay(seconds*1000);clearInterval(timer);socket.close();
