package dev.fusion.bridge;

import com.google.gson.JsonObject;
import java.util.function.Consumer;
import net.fabricmc.api.ModInitializer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/** Shared event seam. Host adapters decide what an event means in their game. */
public final class Bridge implements ModInitializer {
    public static final Logger LOG = LoggerFactory.getLogger("minecraftify");
    public static volatile boolean active;
    public static volatile boolean guestMovementActive;
    /** Adapter has handed the player back to ordinary saved Minecraft gameplay. */
    public static volatile boolean freeMovement;
    public static volatile boolean renderHostSky=true;
    public static volatile net.minecraft.world.phys.Vec3 interactionEye;
    public static volatile Consumer<String> events = message -> {};
    public static Consumer<JsonObject> sceneEvents = message -> {};
    public static Consumer<JsonObject> blockEvents = message -> {};
    public static java.util.function.BiConsumer<net.minecraft.world.entity.projectile.Projectile,JsonObject> projectileProperties=(projectile,event)->{};
    public static void emit(JsonObject event) { if (active) events.accept(event.toString()); }
    @Override public void onInitialize() { LOG.info("MinecraftifyAnything guest initialized"); GameplayEvents.initialize(); }
}
