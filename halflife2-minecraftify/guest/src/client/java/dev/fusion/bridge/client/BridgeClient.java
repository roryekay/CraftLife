package dev.fusion.bridge.client;

import com.google.gson.JsonObject;
import dev.fusion.bridge.Bridge;
import java.util.List;
import java.util.Optional;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientLifecycleEvents;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.screens.TitleScreen;
import net.minecraft.core.HolderLookup;
import net.minecraft.core.HolderSet;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.core.registries.Registries;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.LevelSettings;
import net.minecraft.world.level.WorldDataConfiguration;
import net.minecraft.world.level.biome.Biomes;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.levelgen.FlatLevelSource;
import net.minecraft.world.level.levelgen.WorldDimensions;
import net.minecraft.world.level.levelgen.WorldOptions;
import net.minecraft.world.level.levelgen.flat.FlatLayerInfo;
import net.minecraft.world.level.levelgen.flat.FlatLevelGeneratorSettings;
import net.minecraft.world.level.levelgen.presets.WorldPresets;
import net.minecraft.world.phys.Vec3;

/** Small isolated guest. No starter kit, host health rules or native gameplay implementation. */
public final class BridgeClient implements ClientModInitializer {
    private static final String WORLD="minecraftify-sandbox";
    private boolean requested;
    private int reports;
    private boolean remoteInput;
    private HostLink link;
    @Override public void onInitializeClient() {
        link=HostLink.launch();
        ClientTickEvents.END_CLIENT_TICK.register(this::tick);
        ClientLifecycleEvents.CLIENT_STOPPING.register(mc->{try{link.stop(1000);}catch(InterruptedException e){Thread.currentThread().interrupt();}});
        ServerTickEvents.END_SERVER_TICK.register(server->{
            var p=HostState.live();
            boolean sandbox=Boolean.getBoolean("minecraftify.autoWorld")&&server.getWorldData().getLevelName().equals("Minecraftify Sandbox");
            for(var player:server.getPlayerList().getPlayers()) {
                // The camera-driven guest does not implement a second host movement simulation.
                // Apply only while connected. A void Creative sandbox avoids survival damage assumptions.
                if(p!=null) {
                    player.setNoGravity(true);player.setDeltaMovement(Vec3.ZERO);player.fallDistance=0;
                    player.setPos(p.px(),p.py(),p.pz());
                } else {
                    player.setNoGravity(sandbox);
                    if(sandbox) {
                        player.setDeltaMovement(Vec3.ZERO);player.fallDistance=0;
                        if(player.getY()<0)player.setPos(0,64,0);
                    }
                }
            }
        });
    }
    private void tick(Minecraft mc) {
        var p=HostState.live();
        boolean allowed=p!=null&&p.control();
        if(!allowed&&remoteInput)ClientInput.releaseAll(mc);
        remoteInput=allowed;
        if(p!=null&&mc.player!=null) {
            mc.player.setNoGravity(true);mc.player.setDeltaMovement(Vec3.ZERO);mc.player.fallDistance=0;
            mc.player.setPos(p.px(),p.py(),p.pz());mc.player.setOldPosAndRot();
            mc.player.setYRot(p.yaw());mc.player.setXRot(p.pitch());
        } else if(mc.player!=null) {
            var server=mc.getSingleplayerServer();
            boolean sandbox=Boolean.getBoolean("minecraftify.autoWorld")&&server!=null&&server.getWorldData().getLevelName().equals("Minecraftify Sandbox");
            mc.player.setNoGravity(sandbox);
            if(sandbox) {
                mc.player.setDeltaMovement(Vec3.ZERO);
                if(mc.gui.screen() instanceof net.minecraft.client.gui.screens.DeathScreen)mc.player.respawn();
            }
        }
        if(mc.player!=null&&++reports%4==0) {
            JsonObject event=new JsonObject();event.addProperty("t","guest_state");
            event.addProperty("active",p!=null);event.addProperty("control",allowed);
            event.addProperty("attack",mc.options.keyAttack.isDown());event.addProperty("use",mc.options.keyUse.isDown());
            event.addProperty("slot",mc.player.getInventory().getSelectedSlot());
            event.addProperty("item",BuiltInRegistries.ITEM.getKey(mc.player.getMainHandItem().getItem()).toString());
            event.addProperty("using",mc.player.isUsingItem());event.addProperty("blocking",mc.player.isBlocking());
            event.addProperty("screen",mc.gui.screen()!=null);Bridge.events.accept(event.toString());
        }
        if(!requested&&Boolean.getBoolean("minecraftify.autoWorld")&&mc.level==null&&mc.gui.screen() instanceof TitleScreen) {
            requested=true;
            mc.options.pauseOnLostFocus=false;mc.options.bobView().set(false);mc.options.cloudStatus().set(net.minecraft.client.CloudStatus.OFF);
            mc.options.enableVsync().set(false);mc.options.framerateLimit().set(60);
            if(mc.getLevelSource().levelExists(WORLD))mc.createWorldOpenFlows().openWorld(WORLD,()->mc.gui.setScreen(new TitleScreen()));
            else mc.createWorldOpenFlows().createFreshLevel(WORLD,new LevelSettings("Minecraftify Sandbox",GameType.CREATIVE,LevelSettings.DifficultySettings.DEFAULT,true,WorldDataConfiguration.DEFAULT),new WorldOptions(0,false,false),BridgeClient::voidWorld,mc.gui.screen());
        }
    }
    private static WorldDimensions voidWorld(HolderLookup.Provider registries) {
        var flat=new FlatLevelGeneratorSettings(Optional.of(HolderSet.direct()),registries.lookupOrThrow(Registries.BIOME).getOrThrow(Biomes.PLAINS),List.of());
        flat.getLayersInfo().add(new FlatLayerInfo(1,Blocks.AIR));flat.updateLayers();
        return WorldPresets.createNormalWorldDimensions(registries).replaceOverworldGenerator(registries,new FlatLevelSource(flat));
    }
}
