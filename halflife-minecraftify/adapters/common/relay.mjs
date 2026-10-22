import dgram from 'node:dgram';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {openSync,closeSync} from 'node:fs';
import {spawn} from 'node:child_process';
import {destination} from './travel-catalog.mjs';

export const MAGIC=0x31564d43;
export function encode(kind,sequence,scene,f=[],i=[]){
  const b=Buffer.alloc(128);b.writeUInt32LE(MAGIC);b.writeUInt16LE(1,4);b.writeUInt16LE(kind,6);
  b.writeUInt32LE(sequence>>>0,8);b.writeUInt32LE(scene>>>0,12);
  f.forEach((x,n)=>{if(n>=20||!Number.isFinite(x)||Math.abs(x)>3e7)throw Error('invalid float');b.writeFloatLE(x,16+4*n);});
  i.forEach((x,n)=>{if(n>=8||!Number.isInteger(x)||x<-(2**31)||x>=2**31)throw Error('invalid integer');b.writeInt32LE(x,96+4*n);});return b;
}
export function decode(b){
  if(b.length!==128||b.readUInt32LE()!==MAGIC||b.readUInt16LE(4)!==1)throw Error('invalid packet');
  const p={kind:b.readUInt16LE(6),sequence:b.readUInt32LE(8),scene:b.readUInt32LE(12),f:[],i:[]};
  for(let n=0;n<20;n++){let x=b.readFloatLE(16+4*n);if(p.kind!==4&&(!Number.isFinite(x)||Math.abs(x)>3e7))throw Error('invalid numeric data');p.f.push(x);}
  for(let n=0;n<8;n++)p.i.push(b.readInt32LE(96+4*n));
  if(p.kind===4){p.map=b.subarray(16,80).toString('ascii').split('\0')[0];if(!/^[a-z0-9_]{1,63}$/.test(p.map))throw Error('invalid map');}
  return p;
}
export function run(game,options={}){
  if(!['halflife','halflife2'].includes(game))throw Error('Choose halflife or halflife2');
  const hl1=game==='halflife',port=hl1?25621:25622,server=hl1?25611:25612;
  let encounter=Number(process.argv.find(s=>s.startsWith('--encounter='))?.split('=')[1]??0),travelPending=encounter>0;
  let travelTargetMap='',travelAttempts=0,lastTravel=0,travelFromScene=0;
  if(!Number.isInteger(encounter)||encounter<0||encounter>12)throw Error('Invalid encounter');
  const socket=dgram.createSocket('udp4');
  const hostPid=Number(process.argv.find(s=>s.startsWith('--host-pid='))?.split('=')[1]??0);
  if(!Number.isInteger(hostPid)||hostPid<0||hostPid>2147483647)throw Error('Invalid host process ID');
  let ws,attached=false,scene=0,seq=0,mask=0,guestState=0,cameraTime=0,screen=false,chat=false,item='',map='',lastVitals=0;
  const pending=new Map();
  const knownBlocks=new Map(),installedBlocks=new Set(),pendingBlocks=new Set(),blockAttempts=new Map();
  let feet;
  let lastState,lastNativeState,lastPose;
  const vitalJumps=[];
  const eventCounts={projectile:0,explosion:0,vital_delta:0,melee:0};
  const recentAcks=[];
  const lastOperations=new Map();
  const recentBlocks=[];
  let reconnectTimer,stopped=false;
  let diagnosticInputUntil=0;
  let switching=false;
  let endingPhase=0,guestPid=0,endingReady=0,endingLast=0;
  let inputTraceTime=0;
  let minecraftMovement=options.movement??!options.diagnostics,lastMovement;
  function heartbeat(){return encode(9,0,scene,[],[screen?1:0,minecraftMovement?1:0,chat?1:0]);}
  function send(m){if(attached&&ws?.readyState===1)ws.send(JSON.stringify(m));}
  function command(kind,f=[],i=[],tag){
    if(!scene||Date.now()-guestState>1000)return;
    // Establish/renew control before the operation, including the first input
    // immediately after attach. Both datagrams use the same ordered local socket.
    const beat=heartbeat();socket.send(beat,server,'127.0.0.1');if(hl1)socket.send(beat,25613,'127.0.0.1');
    let id=++seq;socket.send(encode(kind,id,scene,f,i),server,'127.0.0.1');
    if(tag)pending.set(id,{tag,time:Date.now()});
    return id;
  }
  function inputs(p){
    if(Date.now()<diagnosticInputUntil)return;
    const current=p.i[0]>>>0,changed=current^mask,pressed=current&changed;
    if(process.argv.includes('--input-trace')&&screen&&((pressed&3)||Date.now()-inputTraceTime>1000)){inputTraceTime=Date.now();console.log(`Menu pointer ${p.f[0]},${p.f[1]} buttons ${current&3}, native flags ${p.i[7]}`);}
    if(screen){
      send({t:'cursor',x:Math.max(0,Math.min(1,p.f[0])),y:Math.max(0,Math.min(1,p.f[1])),buttons:current&3,modifiers:(current&(1<<8)?1:0)|(current&(1<<9)?64:0)});
    } else {
      for(const [bit,k] of [[0,'attack'],[1,'use'],[5,'drop'],[6,'swap'],[7,'pick']])if(changed&(1<<bit))send({t:'key',k,down:!!(current&(1<<bit))});
    }
    if(!screen&&(pressed&0x80000000))send({t:'chat'});
    if(chat){mask=current;if(pressed&16)send({t:'key',k:'escape',down:true});return;}
    if(pressed&4)send({t:'inventory'});
    if(pressed&(1<<20))send({t:'travel_menu'});
    if(pressed&(1<<21)){
      minecraftMovement=!minecraftMovement;lastMovement=null;
      for(const k of ['forward','back','left','right','jump','sneak','sprint'])send({t:'key',k,down:false});
      console.log(`Movement: ${minecraftMovement?'Minecraft':'native'} (F6)`);
    }
    if(minecraftMovement&&!screen){for(const [bit,k] of [[22,'forward'],[23,'back'],[24,'left'],[25,'right'],[26,'jump'],[8,'sneak'],[9,'sprint']])if(changed&(1<<bit))send({t:'key',k,down:!!(current&(1<<bit))});}
    if(pressed&16)send({t:'key',k:'escape',down:true});
    for(let n=0;n<9;n++)if(pressed&(1<<(10+n)))send(screen?{t:'menu_action',action:'select',value:n}:{t:'slot',n});
    if(screen){
      if(pressed&(1<<27))send({t:'menu_action',action:'group',value:0});
      if(pressed&(1<<28))send({t:'menu_action',action:'group',value:1});
      if(pressed&(1<<29))send({t:'menu_action',action:'page',value:-1});
      if(pressed&(1<<30))send({t:'menu_action',action:'page',value:1});
    }
    mask=current;
  }
  socket.on('message',(b,from)=>{
    if(from.address!=='127.0.0.1'||![server,...(hl1?[25613]:[])].includes(from.port))return;
    try{
      const p=decode(b);
      if(endingPhase&&[2,3,5,32].includes(p.kind))return;
      if(p.kind===4){
        if(p.scene!==scene){endingPhase=endingReady=0;scene=p.scene;seq=0;mask=0;lastMovement=null;pending.clear();knownBlocks.clear();installedBlocks.clear();pendingBlocks.clear();blockAttempts.clear();send({t:'scene',name:`${game}:${p.map}`,epoch:scene});console.log(`Scene ${p.map}, epoch ${scene}`);}
        map=p.map;if(travelPending&&!travelTargetMap)travelTargetMap=map;
      }else if(p.kind===1){
        cameraTime=Date.now();lastPose=p;feet=p.f.slice(3,6);if(p.f[9]>=10&&p.f[9]<=170&&p.i[2]>0&&p.i[3]>0)send({t:'cam',f:++lastCamera,p:p.f.slice(0,3),pl:feet,r:p.f.slice(6,9),eye:p.f.slice(10,13).some(v=>v!==0)?p.f.slice(10,13):p.f.slice(0,3),fov:p.f[9],fp:!!p.i[1],ctl:!!p.i[0],gd:lastMovement&&minecraftMovement?!!lastMovement.i[0]:!!p.i[4],sn:!!p.i[5],gm:minecraftMovement&&!!p.i[6],mb:lastMovement?.i[1]??0});
      }else if(p.kind===33&&!hl1&&['mc_gman_ending','d3_breen_01'].includes(map)&&p.scene===scene&&[1,2].includes(p.i[0])){
        // Source's authored scene events, never wall-clock guesses, drive the handoff.
        if(p.i[0]===1||endingPhase)endingPhase=p.i[0];
        if(endingPhase&&(endingReady<endingPhase||(endingPhase===1&&p.i[1]===1)))send({t:'adapter_event',event:'valve_overworld',scene:'halflife2:mc_gman_ending',epoch:scene,phase:endingPhase,portal:p.i[1]===1,anchor:p.f.slice(0,3)});
      }else if(p.kind===31&&p.scene===scene){lastMovement=p;
      }else if(p.kind===2){if(lastNativeState&&p.f[0]!==lastNativeState.f[0])vitalJumps.push(p.f[0]-lastNativeState.f[0]);if(vitalJumps.length>1024)vitalJumps.shift();lastNativeState=p;if(Date.now()-lastVitals>100){lastVitals=Date.now();send({t:'host_health',hp:p.f[0],max:p.f[1]});}}
      else if(p.kind===3)inputs(p);
      else if(p.kind===32&&chat&&p.scene===scene)send({t:'text',code:p.i[0],modifiers:p.i[1]&0xc3,character:p.i[2]===1});
      else if(p.kind===5)send({t:'terrain',pos:p.i.slice(0,3),solid:!!p.i[3]});
      else if(p.kind===17&&hl1&&from.port===server&&p.scene===scene)socket.send(b,25613,'127.0.0.1');
      else if(p.kind===100){
        const event=pending.get(p.sequence);pending.delete(p.sequence);
        if(event){recentAcks.push({type:event.tag.t,status:p.i[0],entity:p.i[1]});if(recentAcks.length>64)recentAcks.shift();}
        if(event&&event.tag.t!=='diagnostic'&&event.tag.t!=='effects')lastOperations.set(event.tag.t,{status:p.i[0],entity:p.i[1]});
        event?.tag?.resolve?.(p);
        if(event?.tag?.t==='travel'&&p.i[0]===1){travelPending=false;console.log(`Boss travel accepted: ${game}:${map}, encounter ${encounter}`);}
        if(event?.tag?.t==='block'&&event.tag.solid){
          pendingBlocks.delete(event.tag.key);
          if(p.i[0]>=0){if(knownBlocks.has(event.tag.key))installedBlocks.add(event.tag.key);else command(14,[],[...event.tag.pos,0]);}
        }
        if(event?.tag?.t==='projectile'&&p.i[0]===1)send({t:'native_impact',id:event.tag.id,pos:p.f.slice(0,3),visual:event.tag.visual});
        if(p.i[0]<0)console.error(`Native operation ${event?.tag?.t??p.sequence} refused (${p.i[0]})`);
      }
    }catch(error){console.error(error.message);}
  });
  let lastCamera=0;
  function connect(){
    if(stopped)return;
    const connection=new WebSocket('ws://127.0.0.1:25610');ws=connection;
    ws.onmessage=e=>{try{
      if(connection!==ws||stopped)return;
      const m=JSON.parse(e.data);
      if(m.t==='hello'){guestPid=Number.isInteger(m.pid)&&m.pid>0&&m.pid<2147483647?m.pid:0;ws.send('{"t":"attach","v":1}');}
      else if(m.t==='attached'){
        attached=!!m.ok;if(!attached){console.error('Another adapter owns Minecraft');ws.close();return;}
        console.log(`Minecraft attached for ${game}`);if(scene)send({t:'scene',name:`${game}:${map}`,epoch:scene});
      }else if(m.t==='overworld_ready'&&!hl1&&m.epoch===scene&&m.phase===endingPhase&&[1,2].includes(m.phase)){
        if(endingReady!==m.phase)console.log(`Overworld phase ${m.phase} saved/ready`);endingReady=m.phase;
        if(endingReady===2&&guestPid){command(34,[],[guestPid],{t:'ending'});endingLast=Date.now();}
      }else if(m.t==='guest_state'){guestState=Date.now();screen=!!m.screen;chat=!!m.chat;item=m.item;lastState=m;}
      else if(m.t==='movement'&&minecraftMovement&&lastPose?.i[6]){
        if(!Array.isArray(m.delta)||m.delta.length!==3||!m.delta.every(v=>Number.isFinite(v)&&Math.abs(v)<=16)||!Array.isArray(m.velocity)||m.velocity.length!==3||!m.velocity.every(v=>Number.isFinite(v)&&Math.abs(v)<=16)||!Number.isInteger(m.flags)||m.flags<0||m.flags>3)throw Error('Invalid movement');
        command(30,[...m.delta,...m.velocity],[m.flags]);
      }
      else if(m.t==='action'&&m.k==='attack'&&m.down&&!m.screen&&(m.item?.endsWith('_sword')||m.item?.endsWith('_pickaxe'))){eventCounts.melee++;command(10,[],[],{t:'melee'});}
      else if(m.t==='projectile'){
        eventCounts.projectile++;
        const kind=m.kind==='pearl'?3:m.explosive?5:m.kind==='firework'?2:m.damaging?1:4;
        command(11,[...m.from,...m.to,m.power??1],[m.id,kind],{t:'projectile',id:m.id,visual:kind===2||kind===5});
      }else if(m.t==='explosion'){eventCounts.explosion++;command(12,[...m.pos,0,0,0,Math.min(2,m.radius/4)],[],{t:'explosion'});}
      else if(m.t==='block_changed'){
        recentBlocks.push(m);if(recentBlocks.length>128)recentBlocks.shift();
        if(!Array.isArray(m.pos)||m.pos.length!==3||!m.pos.every(Number.isInteger))throw Error('Invalid block position');
        const key=m.pos.join(','),solid=m.solid??!m.air;
        if(solid)knownBlocks.set(key,m.pos);
        else {knownBlocks.delete(key);installedBlocks.delete(key);command(14,[],[...m.pos,0],{t:'block',key,pos:m.pos,solid:false});}
      }
      else if(m.t==='vital_delta'){eventCounts.vital_delta++;command(15,[m.amount*(lastNativeState?.f[1]??100)/20],[],{t:'health'});}
      else if(m.t==='effects')command(16,m.values??[],[m.flags??0],{t:'effects'});
      else if(m.t==='travel_request'){
        const target=m.game??game,selected=destination(target,m.index);
        if(switching)return;
        if(target!==game){
          if(!hostPid)throw Error('Cross-game travel requires a launcher-owned native session');
          const log=openSync(new URL('../../local/cross-game-travel.log',import.meta.url),'a');
          try{
            const child=spawn(process.execPath,[fileURLToPath(new URL('../../tools/start-native.mjs',import.meta.url)),target,`--destination=${m.index}`,`--replace-game=${game}`,`--replace-pid=${hostPid}`],{stdio:['ignore',log,log],detached:true,windowsHide:true});
            switching=true;child.on('error',e=>{switching=false;console.error(`Game switch failed: ${e.message}`);});
            child.on('exit',code=>{if(code){switching=false;console.error('Game switch failed; see local/cross-game-travel.log');}});child.unref();
            console.log(`Switching to ${target}: ${selected.label}`);
          }finally{closeSync(log);}
          return;
        }
        encounter=selected.encounter;travelPending=encounter>0;travelTargetMap=selected.map;travelAttempts=0;lastTravel=0;travelFromScene=scene;
        command(19,[],[m.index],{t:'change_map'});
      }
    }catch(error){console.error(`Guest message refused: ${error.message}`);}};
    const retry=()=>{
      if(connection!==ws)return;
      attached=false;guestState=0;screen=false;chat=false;
      if(!stopped&&!reconnectTimer)reconnectTimer=setTimeout(()=>{reconnectTimer=null;connect();},1000);
    };
    ws.onerror=e=>{console.error(`Minecraft link unavailable: ${e.message??'connection failed'}`);retry();};
    ws.onclose=retry;
  }
  socket.bind(port,'127.0.0.1',()=>{console.log(`Listening on 127.0.0.1:${port}`);connect();});
  socket.on('error',e=>{console.error(e);process.exitCode=1;socket.close();});
  const timer=setInterval(()=>{
    if(endingReady===2&&guestPid&&Date.now()-endingLast>500){command(34,[],[guestPid],{t:'ending'});endingLast=Date.now();}
    if(hostPid){try{process.kill(hostPid,0);}catch(error){if(error.code==='ESRCH'){close();return;}}}
    if(attached&&Date.now()-guestState<500&&Date.now()-cameraTime<500){
      const beat=heartbeat();socket.send(beat,server,'127.0.0.1');if(hl1)socket.send(beat,25613,'127.0.0.1');
      if(travelPending&&scene!==travelFromScene&&map===travelTargetMap&&lastPose?.scene===scene&&Date.now()-lastTravel>500){
        if(travelAttempts>=12){travelPending=false;console.error('Boss travel could not find a safe active encounter in this fresh map.');}
        else {command(18,[],[encounter],{t:'travel'});lastTravel=Date.now();travelAttempts++;}
      }
      if(feet){let sent=0;for(const [key,pos]of knownBlocks){
        if(sent>=8||installedBlocks.size>=512)break;
        if(installedBlocks.has(key)||pendingBlocks.has(key)||Date.now()-(blockAttempts.get(key)??0)<1000)continue;
        if(Math.hypot(...pos.map((v,n)=>v+.5-feet[n]))>7)continue;
        const id=command(14,[],[...pos,1],{t:'block',key,pos,solid:true});
        if(id){pendingBlocks.add(key);blockAttempts.set(key,Date.now());sent++;}
      }}
    }
    for(const [id,event]of pending)if(Date.now()-event.time>2000){console.error(`Native acknowledgement timed out: ${event.tag.t}`);event.tag.reject?.(Error('Native acknowledgement timed out'));if(event.tag.t==='block')pendingBlocks.delete(event.tag.key);pending.delete(id);}
  },100);
  function close(){stopped=true;clearTimeout(reconnectTimer);clearInterval(timer);ws?.close();socket.close();}
  process.once('SIGINT',()=>{close();process.exit();});
  const api={close};
  if(options.diagnostics){
    api.state=()=>lastState;api.nativeState=()=>lastNativeState;api.pose=()=>lastPose;api.counts=()=>({...eventCounts});api.vitalJumps=()=>[...vitalJumps];api.acks=()=>[...recentAcks];api.operations=()=>Object.fromEntries(lastOperations);
    api.blocks=()=>[...recentBlocks];
    api.movementState=()=>lastMovement;
    api.setMovement=enabled=>{if(typeof enabled!=='boolean')throw Error('Movement mode must be boolean');minecraftMovement=enabled;lastMovement=null;};
    api.guest=m=>{if(!['key','slot','cursor','inventory','travel_menu','chat','text'].includes(m.t))throw Error('Unsupported diagnostic guest input');diagnosticInputUntil=Date.now()+500;send(m);};
    api.native=(kind,f=[],i=[])=>new Promise((resolve,reject)=>{
      if(kind<20||kind>29){reject(Error('Only explicitly enabled native test fixtures are permitted'));return;}
      const id=command(kind,f,i,{t:'diagnostic',resolve,reject});if(!id)reject(Error('Bridge is not ready'));
    });
  }
  return api;
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href)run(process.argv[2]??'halflife');
