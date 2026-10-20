package dev.fusion.bridge.client;

import com.google.gson.JsonObject;
import dev.fusion.bridge.Bridge;
import dev.fusion.bridge.client.mixin.KeyMappingAccessor;
import java.util.Set;
import net.minecraft.client.KeyMapping;
import net.minecraft.client.Minecraft;

/** Minecraft input only. No host weapon suppression or host movement assumptions. */
final class ClientInput {
    static final Set<String> TYPES=Set.of("key","slot","scroll");
    private static KeyMapping[] held(Minecraft mc) { return new KeyMapping[]{mc.options.keyAttack,mc.options.keyUse,mc.options.keyPickItem,mc.options.keyDrop,mc.options.keySwapOffhand,mc.options.keyInventory}; }
    static void releaseAll(Minecraft mc) {
        for(KeyMapping key:held(mc)) { key.setDown(false);((KeyMappingAccessor)key).passthrough$setClickCount(0); }
    }
    static void handle(Minecraft mc,JsonObject m) {
        String t=m.get("t").getAsString();
        boolean release=t.equals("key") && m.has("down") && !m.get("down").getAsBoolean();
        if(!HostState.control()&&!release)return;
        if(t.equals("slot")&&mc.player!=null)mc.player.getInventory().setSelectedSlot(Math.clamp(m.get("n").getAsInt(),0,8));
        else if(t.equals("scroll")&&mc.player!=null)mc.player.getInventory().setSelectedSlot(Math.floorMod(mc.player.getInventory().getSelectedSlot()-Integer.signum(m.get("d").getAsInt()),9));
        else if(t.equals("key")) {
            String action=m.get("k").getAsString();boolean down=!release;
            if(action.equals("escape")) { if(down&&mc.gui.screen()!=null)mc.gui.screen().onClose();return; }
            KeyMapping key=switch(action) {
                case "attack"->mc.options.keyAttack;case "use"->mc.options.keyUse;
                case "pick"->mc.options.keyPickItem;case "inventory"->mc.options.keyInventory;
                case "drop"->mc.options.keyDrop;case "swap"->mc.options.keySwapOffhand;default->null;
            };
            if(key==null)return;
            if(down&&!key.isDown()) { var a=(KeyMappingAccessor)key;a.passthrough$setClickCount(a.passthrough$getClickCount()+1); }
            key.setDown(down);
            JsonObject event=new JsonObject();event.addProperty("t","action");event.addProperty("k",action);event.addProperty("down",down);Bridge.emit(event);
        }
    }
}
