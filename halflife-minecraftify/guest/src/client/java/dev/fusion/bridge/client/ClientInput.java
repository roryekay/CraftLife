package dev.fusion.bridge.client;

import com.google.gson.JsonObject;
import dev.fusion.bridge.Bridge;
import dev.fusion.bridge.client.mixin.KeyMappingAccessor;
import java.util.Set;
import net.minecraft.client.KeyMapping;
import net.minecraft.client.Minecraft;

/** Minecraft input only. No host weapon suppression or host movement assumptions. */
final class ClientInput {
    static final Set<String> TYPES=Set.of("key","slot","scroll","cursor","inventory","travel_menu","menu_action");
    private static double cursorX,cursorY;
    private static int buttons;
    private static KeyMapping[] held(Minecraft mc) { return new KeyMapping[]{mc.options.keyAttack,mc.options.keyUse,mc.options.keyPickItem,mc.options.keyDrop,mc.options.keySwapOffhand,mc.options.keyInventory,mc.options.keyUp,mc.options.keyDown,mc.options.keyLeft,mc.options.keyRight,mc.options.keyJump,mc.options.keyShift,mc.options.keySprint}; }
    static void releaseAll(Minecraft mc) {
        for(KeyMapping key:held(mc)) { key.setDown(false);((KeyMappingAccessor)key).passthrough$setClickCount(0); }
        buttons=0;
    }
    static void releaseMovement(Minecraft mc){
        for(KeyMapping key:new KeyMapping[]{mc.options.keyUp,mc.options.keyDown,mc.options.keyLeft,mc.options.keyRight,mc.options.keyJump,mc.options.keyShift,mc.options.keySprint}){
            key.setDown(false);((KeyMappingAccessor)key).passthrough$setClickCount(0);
        }
    }
    static void handle(Minecraft mc,JsonObject m) {
        String t=m.get("t").getAsString();
        boolean release=t.equals("key") && m.has("down") && !m.get("down").getAsBoolean();
        if(!HostState.control()&&!release)return;
        if(t.equals("travel_menu")){releaseAll(mc);BridgeClient.travelMenu.run();}
        else if(t.equals("menu_action")&&mc.gui.screen()!=null)BridgeClient.menuAction.accept(m);
        else if(t.equals("inventory")&&mc.player!=null){
            releaseAll(mc);
            if(mc.gui.screen()!=null)mc.gui.screen().onClose();
            else mc.gui.setScreen(new net.minecraft.client.gui.screens.inventory.InventoryScreen(mc.player));
        }
        else if(t.equals("cursor")&&mc.gui.screen()!=null){
            var screen=mc.gui.screen();double nx=m.get("x").getAsDouble(),ny=m.get("y").getAsDouble();
            if(!Double.isFinite(nx)||!Double.isFinite(ny)||nx<0||nx>1||ny<0||ny>1)return;
            double x=nx*mc.getWindow().getGuiScaledWidth(),y=ny*mc.getWindow().getGuiScaledHeight();
            int next=m.get("buttons").getAsInt()&3,mods=m.get("modifiers").getAsInt()&0xc3;
            var mouse=(dev.fusion.bridge.client.mixin.MouseHandlerAccessor)mc.mouseHandler;
            mouse.minecraftify$x(nx*mc.getWindow().getWidth());mouse.minecraftify$y(ny*mc.getWindow().getHeight());
            screen.mouseMoved(x,y);
            for(int b=0;b<2;b++){
                // Minecraft 26.3 uses SDL button IDs: left=1, right=3.
                var event=new net.minecraft.client.input.MouseButtonEvent(x,y,new net.minecraft.client.input.MouseButtonInfo(b==0?1:3,mods));
                if((next&(1<<b))!=(buttons&(1<<b))){if((next&(1<<b))!=0)screen.mouseClicked(event,false);else screen.mouseReleased(event);}
                else if((next&(1<<b))!=0)screen.mouseDragged(event,x-cursorX,y-cursorY);
            }
            buttons=next;cursorX=x;cursorY=y;
        }
        else if(t.equals("slot")&&mc.player!=null)mc.player.getInventory().setSelectedSlot(Math.clamp(m.get("n").getAsInt(),0,8));
        else if(t.equals("scroll")&&mc.player!=null)mc.player.getInventory().setSelectedSlot(Math.floorMod(mc.player.getInventory().getSelectedSlot()-Integer.signum(m.get("d").getAsInt()),9));
        else if(t.equals("key")) {
            String action=m.get("k").getAsString();boolean down=!release;
            if(action.equals("escape")) { if(down&&mc.gui.screen()!=null)mc.gui.screen().onClose();return; }
            KeyMapping key=switch(action) {
                case "attack"->mc.options.keyAttack;case "use"->mc.options.keyUse;
                case "pick"->mc.options.keyPickItem;case "inventory"->mc.options.keyInventory;
                case "drop"->mc.options.keyDrop;case "swap"->mc.options.keySwapOffhand;default->null;
                case "forward"->mc.options.keyUp;case "back"->mc.options.keyDown;case "left"->mc.options.keyLeft;case "right"->mc.options.keyRight;
                case "jump"->mc.options.keyJump;case "sneak"->mc.options.keyShift;case "sprint"->mc.options.keySprint;
            };
            if(key==null)return;
            if(down&&!key.isDown()) { var a=(KeyMappingAccessor)key;a.passthrough$setClickCount(a.passthrough$getClickCount()+1); }
            key.setDown(down);
            JsonObject event=new JsonObject();event.addProperty("t","action");event.addProperty("k",action);event.addProperty("down",down);
            if(mc.player!=null)event.addProperty("item",net.minecraft.core.registries.BuiltInRegistries.ITEM.getKey(mc.player.getMainHandItem().getItem()).toString());
            event.addProperty("screen",mc.gui.screen()!=null);Bridge.emit(event);
        }
    }
}
