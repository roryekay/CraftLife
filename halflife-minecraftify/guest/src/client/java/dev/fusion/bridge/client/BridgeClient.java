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
    public static Runnable travelMenu=()->{};
    public static java.util.function.Consumer<JsonObject> menuAction=message->{};
    private static final String WORLD=System.getProperty("minecraftify.world","minecraftify-sandbox");
    private Vec3 animationFeet;
    private boolean requested;
    private int reports;
    private boolean remoteInput;
    private HostLink link;
    @Override public void onInitializeClient() {
        link=HostLink.launch();
        ClientTickEvents.START_CLIENT_TICK.register(GuestMovement::begin);
        ClientTickEvents.END_CLIENT_TICK.register(this::tick);
        ClientLifecycleEvents.CLIENT_STOPPING.register(mc->{try{link.stop(1000);}catch(InterruptedException e){Thread.currentThread().interrupt();}});
        ServerTickEvents.END_SERVER_TICK.register(server->{
            var p=HostState.live();
            Bridge.interactionEye=p==null?null:new Vec3(p.ex(),p.ey(),p.ez());
            boolean sandbox=Boolean.getBoolean("minecraftify.autoWorld")&&server.getWorldData().getLevelName().equals("Minecraftify Sandbox");
            for(var player:server.getPlayerList().getPlayers()) {
                // The camera-driven guest does not implement a second host movement simulation.
                // Apply only while connected. A void Creative sandbox avoids survival damage assumptions.
                if(p!=null) {
                    player.setNoGravity(true);player.noPhysics=true;player.setDeltaMovement(Vec3.ZERO);player.fallDistance=0;
                    if(player.position().distanceToSqr(new Vec3(p.px(),p.py(),p.pz()))>64)
                        player.connection.teleport(p.px(),p.py(),p.pz(),p.yaw(),p.pitch());
                    else player.setPos(p.px(),p.py(),p.pz());
                    player.setYRot(p.yaw());player.setXRot(p.pitch());
                    player.setOnGround(p.grounded());
                    if(!p.guestMovement()){player.setShiftKeyDown(p.crouched());player.setPose(p.crouched()?net.minecraft.world.entity.Pose.CROUCHING:net.minecraft.world.entity.Pose.STANDING);}
                    player.connection.resetPosition();
                    player.level().getChunkSource().move(player);
                } else {
                    player.setNoGravity(sandbox);player.noPhysics=sandbox;
                    if(sandbox) {
                        player.setDeltaMovement(Vec3.ZERO);player.fallDistance=0;
                        if(player.getY()<0)player.setPos(0,64,0);
                    }
                }
            }
        });
    }
    private void tick(Minecraft mc) {
        GuestMovement.end(mc);
        var p=HostState.live();
        boolean allowed=p!=null&&p.control();
        if(!allowed&&remoteInput)ClientInput.releaseAll(mc);
        remoteInput=allowed;
        if(p!=null&&mc.player!=null) {
            if(!mc.player.isDeadOrDying()&&mc.gui.screen() instanceof net.minecraft.client.gui.screens.DeathScreen)mc.gui.setScreen(null);
            mc.options.setCameraType(p.firstPerson()?net.minecraft.client.CameraType.FIRST_PERSON:net.minecraft.client.CameraType.THIRD_PERSON_BACK);
            if(!p.guestMovement()){mc.player.setNoGravity(true);mc.player.noPhysics=true;mc.player.setDeltaMovement(Vec3.ZERO);mc.player.fallDistance=0;}
            mc.player.setPos(p.px(),p.py(),p.pz());mc.player.setOldPosAndRot();
            mc.player.setYRot(p.yaw());mc.player.setXRot(p.pitch());
            mc.player.setOnGround(p.grounded());
            if(!p.guestMovement()){mc.player.setShiftKeyDown(p.crouched());mc.player.setPose(p.crouched()?net.minecraft.world.entity.Pose.CROUCHING:net.minecraft.world.entity.Pose.STANDING);}
            mc.player.setYHeadRot(p.yaw());mc.player.yBodyRot=p.yaw();
            Vec3 feet=new Vec3(p.px(),p.py(),p.pz());
            double distance=animationFeet==null?0:feet.subtract(animationFeet).horizontalDistance();
            mc.player.walkAnimation.update(distance<2?(float)Math.min(1,distance*4):0,.4f,1);animationFeet=feet;
        } else if(mc.player!=null) {
            animationFeet=null;
            var server=mc.getSingleplayerServer();
            boolean sandbox=Boolean.getBoolean("minecraftify.autoWorld")&&server!=null&&server.getWorldData().getLevelName().equals("Minecraftify Sandbox");
            mc.player.setNoGravity(sandbox);mc.player.noPhysics=sandbox;
            if(sandbox) {
                mc.player.setDeltaMovement(Vec3.ZERO);
                if(mc.player.isCreative()&&mc.gui.screen() instanceof net.minecraft.client.gui.screens.DeathScreen)mc.player.respawn();
            }
        }
        if(mc.player!=null&&++reports%4==0) {
            JsonObject event=new JsonObject();event.addProperty("t","guest_state");
            event.addProperty("active",p!=null);event.addProperty("control",allowed);
            event.addProperty("attack",mc.options.keyAttack.isDown());event.addProperty("use",mc.options.keyUse.isDown());
            event.addProperty("slot",mc.player.getInventory().getSelectedSlot());
            event.addProperty("item",BuiltInRegistries.ITEM.getKey(mc.player.getMainHandItem().getItem()).toString());
            event.addProperty("using",mc.player.isUsingItem());event.addProperty("blocking",mc.player.isBlocking());
            event.addProperty("screen",mc.gui.screen()!=null);
            event.addProperty("chat",mc.gui.screen() instanceof net.minecraft.client.gui.screens.ChatScreen);
            event.addProperty("gamemode",mc.gameMode.getPlayerMode().getName());
            event.addProperty("uiWidth",mc.getWindow().getGuiScaledWidth());event.addProperty("uiHeight",mc.getWindow().getGuiScaledHeight());
            event.addProperty("hp",mc.player.getHealth());
            event.addProperty("movement",p!=null&&p.guestMovement());event.addProperty("gliding",mc.player.isFallFlying());event.addProperty("sprinting",mc.player.isSprinting());
            event.addProperty("offhand",BuiltInRegistries.ITEM.getKey(mc.player.getOffhandItem().getItem()).toString());event.addProperty("offhandCount",mc.player.getOffhandItem().getCount());
            event.addProperty("wings",mc.player.getItemBySlot(net.minecraft.world.entity.EquipmentSlot.CHEST).is(net.minecraft.world.item.Items.ELYTRA));
            event.addProperty("crouching",mc.player.isCrouching());
            event.add("feet",dev.fusion.bridge.GameplayEvents.vector(mc.player.position()));event.add("velocity",dev.fusion.bridge.GameplayEvents.vector(mc.player.getDeltaMovement()));
            event.addProperty("count",mc.player.getMainHandItem().getCount());
            if(Boolean.getBoolean("minecraftify.diagnostics")){
                var inventory=new com.google.gson.JsonArray();
                for(int n=0;n<36;n++){var stack=mc.player.getInventory().getItem(n);inventory.add(BuiltInRegistries.ITEM.getKey(stack.getItem())+":"+stack.getCount());}
                event.add("inventory",inventory);
                if(mc.hitResult instanceof net.minecraft.world.phys.BlockHitResult hit){event.addProperty("target",hit.getBlockPos().toShortString());event.addProperty("targetBlock",BuiltInRegistries.BLOCK.getKey(mc.level.getBlockState(hit.getBlockPos()).getBlock()).toString());}
            }
            if(mc.gui.screen() instanceof net.minecraft.client.gui.screens.inventory.AbstractContainerScreen<?> container){
                var bounds=(dev.fusion.bridge.client.mixin.ContainerScreenAccessor)container;
                event.addProperty("inventoryLeft",bounds.minecraftify$left());event.addProperty("inventoryTop",bounds.minecraftify$top());
                event.addProperty("carried",BuiltInRegistries.ITEM.getKey(container.getMenu().getCarried().getItem()).toString());
            }
            Bridge.events.accept(event.toString());
        }
        if(!requested&&Boolean.getBoolean("minecraftify.autoWorld")&&mc.level==null&&mc.gui.screen() instanceof TitleScreen) {
            requested=true;
            mc.options.pauseOnLostFocus=false;mc.options.bobView().set(false);mc.options.cloudStatus().set(net.minecraft.client.CloudStatus.OFF);
            mc.options.enableVsync().set(false);mc.options.framerateLimit().set(60);
            mc.options.renderDistance().set(6);mc.options.simulationDistance().set(6);
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
