package dev.fusion.valve;

import com.google.gson.JsonObject;
import dev.fusion.bridge.Bridge;
import dev.fusion.bridge.GameplayEvents;
import dev.fusion.bridge.client.BridgeClient;
import net.minecraft.client.Minecraft;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.levelgen.Heightmap;
import net.minecraft.world.phys.Vec3;

/** Original Source actor/door stay in Source. Minecraft supplies the saved landscape. */
final class ValveEnding {
    static final ResourceKey<Level> OVERWORLD=ResourceKey.create(Registries.DIMENSION,Identifier.parse("minecraftify:overworld"));
    private static int epoch,phase;
    private static BlockPos landing;
    static void initialize(){BridgeClient.adapterMessage=ValveEnding::message;}
    private static void reply(int next,int token){
        JsonObject e=new JsonObject();e.addProperty("t","overworld_ready");e.addProperty("phase",next);e.addProperty("epoch",token);Bridge.events.accept(e.toString());
    }
    private static void message(JsonObject m){
        if(!"valve_overworld".equals(m.get("event").getAsString())||!"halflife2:mc_gman_ending".equals(m.get("scene").getAsString()))return;
        int token=m.get("epoch").getAsInt(),next=m.get("phase").getAsInt();
        if(next!=1&&next!=2)return;
        if(token!=epoch){if(next!=1)return;epoch=token;phase=0;landing=null;}
        if(phase<0)return;
        if(next<=phase){reply(next,token);return;}
        if(next==2&&phase!=1)return;
        var mc=Minecraft.getInstance();var server=mc.getSingleplayerServer();if(server==null)return;
        if(next==1){
            var a=m.getAsJsonArray("anchor");if(a==null||a.size()!=3)return;
            double x=a.get(0).getAsDouble(),y=a.get(1).getAsDouble(),z=a.get(2).getAsDouble();
            if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z)||Math.abs(x)>30000000||Math.abs(y)>30000000||Math.abs(z)>30000000)return;
            // Idempotent while chunk generation runs; native repeats until acknowledged.
            phase=-1;
            server.execute(()->{
                var level=server.getLevel(OVERWORLD);
                if(level==null){phase=0;Bridge.LOG.error("Ending dimension missing; keeping the native scene active");return;}
                int cx=0,cz=0,top=64;
                // Find a dry naturally generated landing instead of an ocean spawn.
                for(int n=0;n<64;n++){
                    cx=(n%8)*32;cz=(n/8)*32;
                    level.getChunk(cx>>4,cz>>4);
                    top=level.getHeight(Heightmap.Types.MOTION_BLOCKING_NO_LEAVES,cx,cz);
                    if(top>63&&top<150&&level.getBlockState(new BlockPos(cx,top-1,cz)).getFluidState().isEmpty())break;
                }
                if(top<64||top>200){phase=0;Bridge.LOG.error("No safe Overworld landing generated; keeping native control");return;}
                landing=new BlockPos(cx,top,cz);
                GameplayEvents.applyingHost=true;
                try{
                    // Small clearing for the original actor's walk to his door.
                    for(int dx=-10;dx<=10;dx++)for(int dz=-10;dz<=10;dz++){
                        for(int dy=-3;dy<0;dy++)level.setBlockAndUpdate(landing.offset(dx,dy,dz),(dy==-1?Blocks.GRASS_BLOCK:Blocks.DIRT).defaultBlockState());
                        for(int dy=0;dy<7;dy++)level.setBlockAndUpdate(landing.offset(dx,dy,dz),Blocks.AIR.defaultBlockState());
                    }
                    // Surround the original 48x104-inch Source door with a
                    // Minecraft portal frame; the real actor walks through it.
                    int doorX=(int)Math.floor(cx+.5+(-15128.0/32-x));
                    int doorZ=(int)Math.floor(cz+.5+(-14113.0/32-z));
                    for(int dx=-2;dx<=1;dx++)for(int dy=-1;dy<=3;dy++)
                        if(dx==-2||dx==1||dy==-1||dy==3)level.setBlockAndUpdate(new BlockPos(doorX+dx,top+dy,doorZ),Blocks.OBSIDIAN.defaultBlockState());
                }finally{GameplayEvents.applyingHost=false;}
                BridgeClient.hostOrigin=new Vec3(cx+.5-x,top-y,cz+.5-z);
                Bridge.LOG.info("Ending clearing prepared at {}, native anchor {} {} {}",landing,x,y,z);
                Bridge.renderHostSky=false;
                for(var p:server.getPlayerList().getPlayers()){
                    if(!p.isPermanentlyInvulnerable())p.addTag("minecraftify:ending-protection");
                    p.setPermanentlyInvulnerable(true);
                    p.teleportTo(level,cx+.5,top,cz+.5,java.util.Set.of(),p.getYRot(),0,false);
                }
                phase=1;reply(1,token);
            });
        }else{
            phase=-2;
            BridgeClient.releaseHost.run();Bridge.freeMovement=true;Bridge.renderHostSky=true;
            server.execute(()->{
                var level=server.getLevel(OVERWORLD);if(level==null||landing==null){phase=1;return;}
                for(var p:server.getPlayerList().getPlayers()){
                    p.teleportTo(level,landing.getX()+.5,landing.getY(),landing.getZ()+.5,java.util.Set.of(),p.getYRot(),0,false);
                    p.setNoGravity(false);p.noPhysics=false;p.setDeltaMovement(Vec3.ZERO);p.fallDistance=0;
                    if(p.entityTags().contains("minecraftify:ending-protection")){p.setPermanentlyInvulnerable(false);p.removeTag("minecraftify:ending-protection");}
                    p.setGameMode(GameType.SURVIVAL);p.addTag("minecraftify:overworld-ending");
                    p.setRespawnPosition(new net.minecraft.server.level.ServerPlayer.RespawnConfig(net.minecraft.world.level.storage.LevelData.RespawnData.of(OVERWORLD,landing,0,0),true),false);
                    p.setHealth(p.getMaxHealth());
                }
                server.saveEverything(false,true,true);BridgeClient.hostOrigin=Vec3.ZERO;phase=2;
                mc.execute(()->{mc.gui.setScreen(null);mc.mouseHandler.grabMouse();reply(2,token);});
            });
        }
    }
}
