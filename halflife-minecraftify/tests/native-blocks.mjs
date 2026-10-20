// Run only in the isolated native test runtime, with the production relay stopped.
import assert from 'node:assert/strict';
import dgram from 'node:dgram';
import {setTimeout as delay} from 'node:timers/promises';
import {encode,decode} from '../adapters/common/relay.mjs';
const game=process.argv[2];assert(['halflife','halflife2'].includes(game));
const hl1=game==='halflife',socket=dgram.createSocket('udp4'),server=hl1?25611:25612;
let scene=0,camera,seq=0,heartbeat,placed,vitals;
const acks=new Map();
socket.on('message',(buffer,from)=>{
  if(from.address!=='127.0.0.1')return;
  const p=decode(buffer);if(p.kind===4)scene=p.scene;if(p.kind===1)camera=p;
  if(p.kind===2)vitals=p;
  if(p.kind===100)acks.set(p.sequence,p);
});
await new Promise(resolve=>socket.bind(hl1?25621:25622,'127.0.0.1',resolve));
async function wait(predicate){const end=Date.now()+7000;while(Date.now()<end){const result=predicate();if(result)return result;await delay(20);}throw Error('Native response timed out: '+JSON.stringify({camera:camera?.i,vitals:vitals?.i}));}
async function command(kind,f=[],i=[]){const id=++seq;socket.send(encode(kind,id,scene,f,i),server,'127.0.0.1');return wait(()=>acks.get(id));}
try{
  await wait(()=>scene&&camera);
  heartbeat=setInterval(()=>socket.send(encode(9,0,scene),server,'127.0.0.1'),100);await delay(200);
  console.log(JSON.stringify({scene,camera:camera.i,vitals:vitals?.f.slice(0,2)}));
  let found;
  for(let angle=0;angle<8;angle++){
    const feet=camera.f.slice(3,6),from=camera.f.slice(0,3),a=angle*Math.PI/4;
    const pos=[Math.floor(feet[0]+Math.cos(a)*2),Math.floor(feet[1]+1),Math.floor(feet[2]+Math.sin(a)*2)];
    const center=pos.map(x=>x+.5);
    const ray=await command(11,[...from,...center,1],[1000+seq,4]);if(ray.i[0]!==0)continue;
    const placedAck=await command(14,[],[...pos,1]);assert.equal(placedAck.i[0],1);assert(placedAck.i[1]>0);placed=pos;
    const hit=await command(11,[...from,...center,1],[1000+seq,4]);
    assert.equal(hit.i[0],1);assert.equal(hit.i[1],placedAck.i[1]);
    const removed=await command(14,[],[...pos,0]);assert.equal(removed.i[0],1);placed=null;
    const cleared=await command(11,[...from,...center,1],[1000+seq,4]);assert.equal(cleared.i[0],0);
    found={pos,entity:placedAck.i[1]};break;
  }
  assert(found,'No clear test location was found');
  console.log(JSON.stringify({passed:true,game,scene,...found,checks:['native collider created','native projectile trace hits exactly that collider','removal restores clear native trace'],limits:'Does not verify standing/jumping, Minecraft placement input, or enemy damage.'},null,2));
}finally{
  if(placed)await command(14,[],[...placed,0]).catch(()=>{});
  clearInterval(heartbeat);socket.close();
}
