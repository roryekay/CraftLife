package dev.fusion.valve;
import com.google.gson.*;
import dev.fusion.bridge.Bridge;
import dev.fusion.bridge.client.BridgeClient;
import java.io.InputStreamReader;
import net.fabricmc.api.ClientModInitializer;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.components.Button;
import net.minecraft.client.gui.screens.Screen;
import net.minecraft.network.chat.Component;

public final class ValveUi implements ClientModInitializer {
    @Override public void onInitializeClient(){
        if(System.getProperty("minecraftify.overscan")==null)System.setProperty("minecraftify.overscan","1.12");
        BridgeClient.travelMenu=()->Minecraft.getInstance().gui.setScreen(new TravelScreen());
        BridgeClient.menuAction=m->{if(Minecraft.getInstance().gui.screen() instanceof TravelScreen screen){
            int value=m.get("value").getAsInt();switch(m.get("action").getAsString()){
                case "group"->{if(value==0||value==1)screen.group(value);}
                case "page"->{if(value==-1||value==1)screen.turnPage(value);}
                case "select"->screen.selectRow(value);
            }
        }};
    }
    static final class TravelScreen extends Screen {
        int page;
        int rows=8;
        JsonArray entries;
        String game=System.getProperty("minecraftify.game","halflife");
        TravelScreen(){super(Component.literal("Half-Life travel"));}
        void group(int value){game=value==0?"halflife":"halflife2";page=0;rebuildWidgets();}
        void turnPage(int value){page=Math.clamp(page+value,0,(entries.size()-1)/rows);rebuildWidgets();}
        void selectRow(int row){
            int index=page*rows+row;if(row<0||row>=rows||index>=entries.size())return;
            JsonObject event=new JsonObject();event.addProperty("t","travel_request");event.addProperty("game",game);event.addProperty("index",index);Bridge.emit(event);onClose();
        }
        @Override protected void init(){
            try(var input=getClass().getResourceAsStream("/minecraftify-moments.json")){
                var catalog=JsonParser.parseReader(new InputStreamReader(input,java.nio.charset.StandardCharsets.UTF_8)).getAsJsonObject();
                entries=catalog.getAsJsonArray(game);if(entries==null)throw new IllegalArgumentException("Unknown host game");
            }catch(Exception e){throw new IllegalStateException("Travel catalog unavailable",e);}
            var first=addRenderableWidget(Button.builder(Component.literal("Half-Life 1 [F9]"),b->group(0)).bounds(width/2-155,20,150,20).build());first.active=!game.equals("halflife");
            var second=addRenderableWidget(Button.builder(Component.literal("Half-Life 2 [F10]"),b->group(1)).bounds(width/2+5,20,150,20).build());second.active=!game.equals("halflife2");
            rows=Math.clamp((height-132)/22,1,8);page=Math.min(page,(entries.size()-1)/rows);
            addRenderableWidget(Button.builder(Component.literal("Bosses / moments — page "+(page+1)+" — choose 1-"+rows),b->{}).bounds(width/2-155,43,310,20).build());
            addRenderableWidget(Button.builder(Component.literal("Fresh map; other game switches automatically"),b->{}).bounds(width/2-170,66,340,20).build());
            for(int row=0;row<rows;row++){
                int index=page*rows+row;if(index>=entries.size())break;var entry=entries.get(index).getAsJsonObject();
                final int selectedRow=row;
                addRenderableWidget(Button.builder(Component.literal((row+1)+". "+entry.get("label").getAsString()),b->selectRow(selectedRow)).bounds(width/2-155,94+row*22,310,20).build());
            }
            addRenderableWidget(Button.builder(Component.literal("Prev [PgUp]"),b->turnPage(-1)).bounds(width/2-155,height-24,100,20).build());
            addRenderableWidget(Button.builder(Component.literal("Close"),b->onClose()).bounds(width/2-50,height-24,100,20).build());
            addRenderableWidget(Button.builder(Component.literal("Next [PgDn]"),b->turnPage(1)).bounds(width/2+55,height-24,100,20).build());
        }
        @Override public boolean isPauseScreen(){return false;}
    }
}
