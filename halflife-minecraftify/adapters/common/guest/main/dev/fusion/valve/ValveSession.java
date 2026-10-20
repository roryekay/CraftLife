package dev.fusion.valve;

import com.google.gson.*;
import dev.fusion.bridge.Bridge;
import dev.fusion.bridge.GameplayEvents;
import java.nio.file.*;
import java.util.*;
import net.fabricmc.api.ModInitializer;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.core.BlockPos;
import net.minecraft.core.component.DataComponents;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.world.effect.MobEffects;
import net.minecraft.world.item.*;
import net.minecraft.world.item.alchemy.*;
import net.minecraft.world.item.component.*;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.storage.LevelResource;
import com.mojang.serialization.JsonOps;

/** Valve balance/profile data stays outside the Minecraftify shared core. */
public final class ValveSession implements ModInitializer {
    private MinecraftServer server;
    private String scene="";
    private int ticks;
    private boolean dirty;
    private final Set<BlockPos> replay=new HashSet<>();
    private final Map<String,Map<BlockPos,BlockState>> scenes=new HashMap<>();
    @Override public void onInitialize(){
        net.fabricmc.fabric.api.entity.event.v1.ServerPlayerEvents.COPY_FROM.register((oldPlayer,newPlayer,alive)->{
            for(String tag:oldPlayer.entityTags())if(tag.startsWith("minecraftify:"))newPlayer.addTag(tag);
        });
        ServerLifecycleEvents.SERVER_STARTED.register(s->{
            server=s;load();
            if(Boolean.getBoolean("minecraftify.autoWorld")&&s.getWorldData().getLevelName().equals("Minecraftify Sandbox"))
                s.getGameRules().set(net.minecraft.world.level.gamerules.GameRules.KEEP_INVENTORY,true,s);
        });
        ServerLifecycleEvents.SERVER_STOPPING.register(s->{
            save();GameplayEvents.applyingHost=true;
            try{for(var pos:scenes.getOrDefault(scene,Map.of()).keySet())s.overworld().setBlockAndUpdate(pos,Blocks.AIR.defaultBlockState());}finally{GameplayEvents.applyingHost=false;}
        });
        Bridge.sceneEvents=this::scene;
        Bridge.projectileProperties=(projectile,event)->{
            if(projectile instanceof net.minecraft.world.entity.projectile.arrow.AbstractArrow){var weapon=((dev.fusion.bridge.mixin.ArrowAccessor)projectile).minecraftify$weapon();if(weapon!=null&&weapon.is(Items.CROSSBOW))event.addProperty("explosive",true);}
        };
        Bridge.blockEvents=m->{
            if(server==null||scene.isEmpty()||GameplayEvents.applyingHost)return;
            var p=m.getAsJsonArray("pos");var pos=new BlockPos(p.get(0).getAsInt(),p.get(1).getAsInt(),p.get(2).getAsInt());
            var map=scenes.computeIfAbsent(scene,k->new HashMap<>());
            var state=server.overworld().getBlockState(pos);
            if(state.isAir())map.remove(pos);else if(map.size()<65536)map.put(pos,state);dirty=true;
        };
        ServerTickEvents.END_SERVER_TICK.register(s->{
            if(!Bridge.active)return;
            var iterator=replay.iterator();
            for(int count=0;count<32&&iterator.hasNext();count++){
                var pos=iterator.next();iterator.remove();var state=scenes.getOrDefault(scene,Map.of()).get(pos);if(state==null)continue;
                JsonObject event=new JsonObject();event.addProperty("t","block_changed");JsonArray at=new JsonArray();at.add(pos.getX());at.add(pos.getY());at.add(pos.getZ());event.add("pos",at);event.addProperty("air",false);event.addProperty("solid",state.isCollisionShapeFullBlock(s.overworld(),pos));Bridge.emit(event);
            }
            for(var player:s.getPlayerList().getPlayers()){
                if(!player.entityTags().contains("minecraftify:valve-kit-v1"))kit(player);
                if(!player.entityTags().contains("minecraftify:milk-v1")){player.getInventory().add(new ItemStack(Items.MILK_BUCKET));player.addTag("minecraftify:milk-v1");}
                if(!player.entityTags().contains("minecraftify:movement-kit-v1")){
                    var wings=new ItemStack(Items.ELYTRA);
                    if(player.getItemBySlot(net.minecraft.world.entity.EquipmentSlot.CHEST).isEmpty())player.setItemSlot(net.minecraft.world.entity.EquipmentSlot.CHEST,wings);else player.getInventory().add(wings);
                    var boost=new ItemStack(Items.FIREWORK_ROCKET,64);boost.set(DataComponents.FIREWORKS,new Fireworks(1,List.of()));
                    if(player.getOffhandItem().isEmpty())player.setItemSlot(net.minecraft.world.entity.EquipmentSlot.OFFHAND,boost);else player.getInventory().add(boost);
                    player.addTag("minecraftify:movement-kit-v1");
                }
                if(!player.entityTags().contains("minecraftify:power-v")){
                    for(int n=0;n<36;n++){var stack=player.getInventory().getItem(n);if(stack.is(Items.BOW))stack.enchant(player.level().registryAccess().lookupOrThrow(net.minecraft.core.registries.Registries.ENCHANTMENT).getOrThrow(net.minecraft.world.item.enchantment.Enchantments.POWER),5);}
                    player.addTag("minecraftify:power-v");
                }
                if(++ticks%10==0)effects(player);
            }
            if(dirty&&ticks%100==0)save();
        });
    }
    private void kit(ServerPlayer p){
        p.setGameMode(GameType.SURVIVAL);
        p.getInventory().clearContent();
        ItemStack[] bar={new ItemStack(Items.IRON_SWORD),new ItemStack(Items.IRON_PICKAXE),new ItemStack(Items.BOW),new ItemStack(Items.CROSSBOW),new ItemStack(Items.COBBLESTONE,64),new ItemStack(Items.ENDER_PEARL,16),new ItemStack(Items.TNT,16),new ItemStack(Items.FLINT_AND_STEEL),new ItemStack(Items.COOKED_BEEF,32)};
        bar[2].enchant(p.level().registryAccess().lookupOrThrow(net.minecraft.core.registries.Registries.ENCHANTMENT).getOrThrow(net.minecraft.world.item.enchantment.Enchantments.POWER),5);
        for(int i=0;i<bar.length;i++)p.getInventory().setItem(i,bar[i]);
        p.getInventory().add(new ItemStack(Items.ARROW,64));p.getInventory().add(new ItemStack(Items.GOLDEN_APPLE,8));
        var rockets=new ItemStack(Items.FIREWORK_ROCKET,64);
        rockets.set(DataComponents.FIREWORKS,new Fireworks(1,List.of(new FireworkExplosion(FireworkExplosion.Shape.LARGE_BALL,it.unimi.dsi.fastutil.ints.IntList.of(0xff6600),it.unimi.dsi.fastutil.ints.IntList.of(),true,true))));
        p.getInventory().add(rockets);
        for(var potion:List.of(Potions.HEALING,Potions.REGENERATION,Potions.SWIFTNESS,Potions.LEAPING,Potions.FIRE_RESISTANCE,Potions.INVISIBILITY)){
            var stack=new ItemStack(Items.POTION);stack.set(DataComponents.POTION_CONTENTS,new PotionContents(potion));p.getInventory().add(stack);
        }
        p.addTag("minecraftify:valve-kit-v1");
    }
    private void effects(ServerPlayer p){
        int flags=(p.hasEffect(MobEffects.FIRE_RESISTANCE)?1:0)|(p.hasEffect(MobEffects.INVISIBILITY)?2:0)|(p.hasEffect(MobEffects.RESISTANCE)?4:0);
        float speed=p.hasEffect(MobEffects.SPEED)?1+.2f*(p.getEffect(MobEffects.SPEED).getAmplifier()+1):1;
        float jump=p.hasEffect(MobEffects.JUMP_BOOST)?1+.25f*(p.getEffect(MobEffects.JUMP_BOOST).getAmplifier()+1):1;
        JsonObject e=new JsonObject();e.addProperty("t","effects");e.addProperty("flags",flags);JsonArray values=new JsonArray();values.add(speed);values.add(jump);e.add("values",values);Bridge.emit(e);
    }
    private void scene(JsonObject m){
        if(server==null)return;String next=m.get("name").getAsString();if(!next.matches("halflife2?:[a-z0-9_]{1,63}"))return;
        System.setProperty("minecraftify.game",next.substring(0,next.indexOf(':')));
        GameplayEvents.applyingHost=true;
        try{
            if(!scene.equals(next)){
                for(var p:scenes.getOrDefault(scene,Map.of()).keySet())server.overworld().setBlockAndUpdate(p,Blocks.AIR.defaultBlockState());
                scene=next;
                for(var e:scenes.getOrDefault(scene,Map.of()).entrySet())server.overworld().setBlockAndUpdate(e.getKey(),e.getValue());
            }
        }finally{GameplayEvents.applyingHost=false;}
        // Full per-scene block snapshot lets the native adapter rebuild its colliders.
        replay.clear();replay.addAll(scenes.getOrDefault(scene,Map.of()).keySet());
    }
    private Path path(){return server.getWorldPath(LevelResource.ROOT).resolve("minecraftify-valve-scenes.json");}
    private void load(){
        try{
            if(!Files.exists(path())||Files.size(path())>16*1024*1024)return;
            var root=JsonParser.parseString(Files.readString(path())).getAsJsonObject();
            for(var entry:root.entrySet()){
                if(!entry.getKey().matches("halflife2?:[a-z0-9_]{1,63}"))continue;
                Map<BlockPos,BlockState> blocks=new HashMap<>();
                for(var value:entry.getValue().getAsJsonArray()){
                    if(blocks.size()>=65536)break;var row=value.getAsJsonObject();var p=row.getAsJsonArray("p");
                    var state=BlockState.CODEC.parse(JsonOps.INSTANCE,row.get("s")).result();
                    if(state.isPresent())blocks.put(new BlockPos(p.get(0).getAsInt(),p.get(1).getAsInt(),p.get(2).getAsInt()),state.get());
                }scenes.put(entry.getKey(),blocks);
            }
        }catch(Exception e){Bridge.LOG.error("Scene storage could not be read; refusing to overwrite it",e);throw new IllegalStateException(e);}
    }
    private void save(){
        if(server==null||!dirty)return;
        try{
            JsonObject root=new JsonObject();for(var entry:scenes.entrySet()){
                JsonArray rows=new JsonArray();for(var b:entry.getValue().entrySet()){
                    JsonObject row=new JsonObject();JsonArray p=new JsonArray();p.add(b.getKey().getX());p.add(b.getKey().getY());p.add(b.getKey().getZ());row.add("p",p);
                    row.add("s",BlockState.CODEC.encodeStart(JsonOps.INSTANCE,b.getValue()).getOrThrow());rows.add(row);
                }root.add(entry.getKey(),rows);
            }
            Path tmp=path().resolveSibling(path().getFileName()+".tmp");Files.writeString(tmp,root.toString());Files.move(tmp,path(),StandardCopyOption.REPLACE_EXISTING);dirty=false;
        }catch(Exception e){Bridge.LOG.error("Could not save scene blocks",e);}
    }
}
