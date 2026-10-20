package dev.fusion.bridge;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.projectile.Projectile;
import net.minecraft.world.entity.projectile.FireworkRocketEntity;
import net.minecraft.world.entity.projectile.arrow.AbstractArrow;
import net.minecraft.world.entity.projectile.throwableitemprojectile.ThrownEnderpearl;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.phys.Vec3;

/** Generic completed gameplay observations. Native balance and item kits live in adapters. */
public final class GameplayEvents {
    public static volatile boolean applyingHost;
    private static final Map<UUID,Vec3> positions=new HashMap<>();
    private static final Map<UUID,Float> health=new HashMap<>();
    private static final java.util.Set<BlockPos> terrain=new java.util.HashSet<>();
    public static JsonArray vector(Vec3 p){JsonArray a=new JsonArray();a.add(p.x);a.add(p.y);a.add(p.z);return a;}
    public static void projectile(Projectile p,Vec3 from,Vec3 to){
        if(!Bridge.active||!(p.getOwner() instanceof ServerPlayer))return;
        if(p instanceof FireworkRocketEntity&&((dev.fusion.bridge.mixin.FireworkInvoker)p).minecraftify$attached())return;
        JsonObject e=new JsonObject();e.addProperty("t","projectile");e.addProperty("id",p.getId());
        e.add("from",vector(from));e.add("to",vector(to));
        e.addProperty("kind",p instanceof ThrownEnderpearl?"pearl":p instanceof FireworkRocketEntity?"firework":"projectile");
        e.addProperty("damaging",p instanceof AbstractArrow);
        var weapon=p instanceof AbstractArrow?((dev.fusion.bridge.mixin.ArrowAccessor)p).minecraftify$weapon():null;
        e.addProperty("explosive",p.entityTags().contains("minecraftify:explosive"));
        double power=p.getDeltaMovement().length()/3.0;
        if(p instanceof AbstractArrow){
            double base=((dev.fusion.bridge.mixin.ArrowAccessor)p).minecraftify$baseDamage();
            // Modern Minecraft applies Power when hitting an entity, rather than
            // storing it in baseDamage. Native enemies have no guest entity.
            if(weapon!=null){
                int level=net.minecraft.world.item.enchantment.EnchantmentHelper.getItemEnchantmentLevel(p.level().registryAccess().lookupOrThrow(net.minecraft.core.registries.Registries.ENCHANTMENT).getOrThrow(net.minecraft.world.item.enchantment.Enchantments.POWER),weapon);
                if(level>0)base+=.5*(level+1);
            }
            power*=base/2;
        }
        e.addProperty("power",Math.clamp(power,0.1,4.0));Bridge.projectileProperties.accept(p,e);Bridge.emit(e);
    }
    public static void explosion(Vec3 at,float radius){
        JsonObject e=new JsonObject();e.addProperty("t","explosion");e.add("pos",vector(at));e.addProperty("radius",radius);Bridge.emit(e);
    }
    public static void initialize(){
        ServerTickEvents.END_SERVER_TICK.register(GameplayEvents::tick);
        net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents.SERVER_STOPPING.register(server->{
            applyingHost=true;try{for(var pos:terrain)if(server.overworld().getBlockState(pos).is(Blocks.BARRIER))server.overworld().setBlockAndUpdate(pos,Blocks.AIR.defaultBlockState());}finally{applyingHost=false;terrain.clear();}
        });
    }
    private static void tick(MinecraftServer server){
        if(!Bridge.active){positions.clear();health.clear();return;}
        for(var level:server.getAllLevels())for(Entity entity:level.getAllEntities())if(entity instanceof Projectile p&&p.getOwner() instanceof ServerPlayer player){
            Vec3 old=positions.put(p.getUUID(),p.position());
            if(old==null){
                old=p.position().subtract(p.getDeltaMovement());
            }
            if(old.distanceToSqr(p.position())>0.000001)projectile(p,old,p.position());
        }
        if(positions.size()>4096)positions.clear();
        for(ServerPlayer player:server.getPlayerList().getPlayers()){
            float now=player.getHealth();Float previous=health.put(player.getUUID(),now);
            if(previous!=null&&Math.abs(now-previous)>0.001){JsonObject e=new JsonObject();e.addProperty("t","vital_delta");e.addProperty("amount",now-previous);Bridge.emit(e);}
        }
    }
    public static void accept(MinecraftServer server,JsonObject m){
        String t=m.get("t").getAsString();
        if(t.equals("native_impact")){
            int id=m.get("id").getAsInt();
            for(var level:server.getAllLevels()){var entity=level.getEntity(id);if(entity instanceof Projectile p&&p.getOwner() instanceof ServerPlayer){
                if(p instanceof FireworkRocketEntity)((dev.fusion.bridge.mixin.FireworkInvoker)p).minecraftify$explode(level);
                p.discard();
            }}
        }else if(t.equals("host_health")){
            float hp=m.get("hp").getAsFloat(),max=m.get("max").getAsFloat();if(!Float.isFinite(hp)||!Float.isFinite(max)||max<=0)return;
            for(ServerPlayer current:java.util.List.copyOf(server.getPlayerList().getPlayers())){
                // Respawn on the authoritative server before restoring health. A
                // client respawn request can arrive after setHealth and be ignored.
                ServerPlayer player=current;
                if(hp>0&&player.isDeadOrDying()){
                    var connection=player.connection;
                    connection.handleClientCommand(new net.minecraft.network.protocol.game.ServerboundClientCommandPacket(net.minecraft.network.protocol.game.ServerboundClientCommandPacket.Action.PERFORM_RESPAWN));
                    player=connection.player;
                }
                player.setHealth(Math.clamp(hp/max*20,0,20));health.put(player.getUUID(),player.getHealth());
            }
        }else if(t.equals("scene")){
            for(var level:server.getAllLevels())for(var entity:level.getAllEntities())if(entity instanceof Projectile||entity instanceof net.minecraft.world.entity.item.PrimedTnt)entity.discard();
            applyingHost=true;try{for(var pos:terrain)if(server.overworld().getBlockState(pos).is(Blocks.BARRIER))server.overworld().setBlockAndUpdate(pos,Blocks.AIR.defaultBlockState());}finally{applyingHost=false;terrain.clear();}
            positions.clear();health.clear();Bridge.sceneEvents.accept(m);
        }else if(t.equals("terrain")){
            var a=m.getAsJsonArray("pos");if(a.size()!=3)return;
            BlockPos pos=new BlockPos(a.get(0).getAsInt(),a.get(1).getAsInt(),a.get(2).getAsInt());
            var level=server.overworld();if(level.isOutsideBuildHeight(pos))return;
            var state=level.getBlockState(pos);
            applyingHost=true;
            try{
                if(m.get("solid").getAsBoolean()&&state.isAir()){level.setBlockAndUpdate(pos,Blocks.BARRIER.defaultBlockState());terrain.add(pos.immutable());}
                else if(!m.get("solid").getAsBoolean()&&state.is(Blocks.BARRIER)){level.setBlockAndUpdate(pos,Blocks.AIR.defaultBlockState());terrain.remove(pos);}
            }finally{applyingHost=false;}
        }
    }
}
