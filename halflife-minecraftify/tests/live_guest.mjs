// Run against the isolated developer sandbox, never a user's ordinary world.
import assert from 'node:assert/strict';
import { setTimeout as delay } from 'node:timers/promises';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { fileURLToPath } from 'node:url';
const exec=promisify(execFile),url=`ws://127.0.0.1:${process.env.MINECRAFTIFY_PORT??25610}`;
const connections=[];
async function connect() {
  const socket=new WebSocket(url),messages=[];
  socket.onmessage=e=>messages.push(JSON.parse(e.data));connections.push(socket);
  const wait=async(predicate,timeout=5000)=>{
    const end=Date.now()+timeout;
    while(Date.now()<end){const i=messages.findIndex(predicate);if(i>=0)return messages.splice(i,1)[0];await delay(20);}
    throw Error('Expected guest response did not arrive');
  };
  const send=m=>socket.send(JSON.stringify(m));await wait(m=>m.t==='hello');return {socket,send,wait,messages};
}
let timer;
const checks=[];
try {
  const host=await connect();host.send({t:'attach',v:1});assert.equal((await host.wait(m=>m.t==='attached')).ok,true);checks.push('host attaches using protocol v1');
  const other=await connect();other.send({t:'attach',v:1});assert.equal((await other.wait(m=>m.t==='attached')).ok,false);checks.push('second host cannot take camera/input ownership');
  let frame=0;
  const camera=()=>host.send({t:'cam',f:++frame,p:[0,65.62,0],pl:[0,64,0],r:[0,0,0],fov:70,fp:true,ctl:true});
  camera();timer=setInterval(camera,20);
  await host.wait(m=>m.t==='guest_state'&&m.active&&m.control);
  host.send({t:'cam',f:0,p:[0,0],pl:[0,64,0],r:[0,0,0],fov:70});await host.wait(m=>m.t==='error');checks.push('malformed camera refused while valid stream remains alive');
  host.send({t:'cmd',c:'not a supported protocol operation'});assert.equal((await host.wait(m=>m.t==='unsupported')).type,'cmd');checks.push('legacy arbitrary command path absent');
  host.send({t:'slot',n:4});await host.wait(m=>m.t==='guest_state'&&m.slot===4);checks.push('real Minecraft hotbar selection follows host input');
  host.send({t:'key',k:'attack',down:true});await host.wait(m=>m.t==='guest_state'&&m.attack);checks.push('forwarded held attack reaches actual Minecraft key state');
  host.send({t:'key',k:'attack',down:false});
  host.send({t:'key',k:'escape',down:true});
  host.send({t:'slot',n:0});await host.wait(m=>m.t==='guest_state'&&m.slot===0);
  host.send({t:'inventory'});const ui=await host.wait(m=>m.t==='guest_state'&&m.screen);checks.push('actual Minecraft inventory opens through the adapter protocol');
  const click=async(x,y)=>{
    host.send({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:1,modifiers:0});await delay(60);
    host.send({t:'cursor',x:x/ui.uiWidth,y:y/ui.uiHeight,buttons:0,modifiers:0});await delay(60);
  };
  const sword=[ui.inventoryLeft+16,ui.inventoryTop+150],reserve=[ui.inventoryLeft+160,ui.inventoryTop+128];
  if(process.argv.includes('--restore-sword')){await click(...reserve);await click(...sword);host.messages.length=0;await host.wait(m=>m.t==='guest_state'&&m.item==='minecraft:iron_sword'&&m.carried==='minecraft:air');}
  await click(...sword);await click(...reserve);
  await host.wait(m=>m.t==='guest_state'&&m.item==='minecraft:air'&&m.carried==='minecraft:air');
  await click(...reserve);await click(...sword);
  await host.wait(m=>m.t==='guest_state'&&m.item==='minecraft:iron_sword'&&m.carried==='minecraft:air');checks.push('real sword moved from hotbar into inventory and back using forwarded mouse clicks');
  host.send({t:'key',k:'escape',down:true});await host.wait(m=>m.t==='guest_state'&&!m.screen);checks.push('inventory closes through the adapter protocol');
  host.send({t:'inventory'});await host.wait(m=>m.t==='guest_state'&&m.screen);
  await click(...sword);await click(...reserve);await host.wait(m=>m.t==='guest_state'&&m.item==='minecraft:air'&&m.carried==='minecraft:air');
  host.send({t:'key',k:'escape',down:true});await host.wait(m=>m.t==='guest_state'&&!m.screen);
  host.messages.length=0;host.send({t:'host_health',hp:0,max:100});await host.wait(m=>m.t==='guest_state'&&m.hp===0);
  host.messages.length=0;host.send({t:'host_health',hp:100,max:100});await host.wait(m=>m.t==='guest_state'&&m.hp>0&&!m.screen,10000);
  host.messages.length=0;host.send({t:'inventory'});await host.wait(m=>m.t==='guest_state'&&m.screen&&m.item==='minecraft:air');
  await click(...reserve);await click(...sword);await host.wait(m=>m.t==='guest_state'&&m.item==='minecraft:iron_sword'&&m.carried==='minecraft:air');
  host.send({t:'key',k:'escape',down:true});await host.wait(m=>m.t==='guest_state'&&!m.screen);
  checks.push('native death/respawn health synchronization preserves a sword stored outside the original kit slot');
  host.send({t:'travel_menu'});await host.wait(m=>m.t==='guest_state'&&m.screen);checks.push('chapter/boss travel screen opens');
  host.send({t:'key',k:'escape',down:true});await host.wait(m=>m.t==='guest_state'&&!m.screen);
  const probe=fileURLToPath(new URL('../local/renderer32/Release/frame_probe.exe',import.meta.url));
  const live=await exec(probe,['2'],{timeout:6000});const reports=live.stdout.trim().split(/\r?\n/).map(JSON.parse);
  assert(reports.at(-1).frames>10);assert(reports[0].hostFrame>0);checks.push(`actual Minecraft colour/depth/HUD snapshots: ${reports.at(-1).frames} frames in 2s`);
  clearInterval(timer);timer=null;
  await host.wait(m=>m.t==='guest_state'&&!m.active&&!m.attack&&!m.use);checks.push('expired camera releases held item inputs');
  host.send({t:'key',k:'use',down:true});await delay(250);
  host.messages.length=0;await host.wait(m=>m.t==='guest_state'&&!m.active&&!m.use);checks.push('expired host cannot begin a new item action');
  host.socket.close();await delay(200);other.send({t:'attach',v:1});assert.equal((await other.wait(m=>m.t==='attached')).ok,true);checks.push('ownership becomes available after disconnect');
  other.send({t:'cam',f:1,p:[0,65.62,0],pl:[0,64,0],r:[0,0,0],fov:70});
  await other.wait(m=>m.t==='guest_state'&&m.active);checks.push('new owner reconnects without restarting Minecraft');
  console.log(JSON.stringify({passed:true,checks},null,2));
} finally {clearInterval(timer);for(const socket of connections)socket.close();}
