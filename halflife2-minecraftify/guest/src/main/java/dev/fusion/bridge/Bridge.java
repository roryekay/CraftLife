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
    public static volatile Consumer<String> events = message -> {};
    public static void emit(JsonObject event) { if (active) events.accept(event.toString()); }
    @Override public void onInitialize() { LOG.info("MinecraftifyAnything guest initialized"); }
}
