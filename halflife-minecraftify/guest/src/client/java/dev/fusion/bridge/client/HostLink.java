package dev.fusion.bridge.client;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import dev.fusion.bridge.Bridge;
import java.net.InetSocketAddress;
import net.minecraft.client.Minecraft;
import org.java_websocket.WebSocket;
import org.java_websocket.handshake.ClientHandshake;
import org.java_websocket.server.WebSocketServer;

/** One local adapter owns input/camera; observers can receive events without taking over. */
public final class HostLink extends WebSocketServer {
    private volatile WebSocket host;
    public synchronized void releaseOwner() {
        var previous=host;host=null;HostState.disconnect(previous);Bridge.active=false;
        ClientInput.releaseAll(Minecraft.getInstance());
    }
    private HostLink(int port) { super(new InetSocketAddress("127.0.0.1",port)); setReuseAddr(true); setDaemon(true); }
    static HostLink launch() {
        int port=Integer.getInteger("minecraftify.port",25610);
        if (port<1024 || port>65535) throw new IllegalArgumentException("invalid local port");
        HostLink link=new HostLink(port); link.start(); Bridge.events=link::broadcast; return link;
    }
    @Override public void onStart() { Bridge.LOG.info("local adapter link ready on {}",getPort()); }
    @Override public void onOpen(WebSocket conn, ClientHandshake handshake) {
        JsonObject hello=new JsonObject(); hello.addProperty("t","hello");hello.addProperty("v",1);
        hello.addProperty("mapping",FrameExporter.NAME);hello.addProperty("pid",ProcessHandle.current().pid());conn.send(hello.toString());
    }
    @Override public void onClose(WebSocket conn,int code,String reason,boolean remote) {
        if (host==conn) { host=null; HostState.disconnect(conn); Minecraft.getInstance().execute(()->ClientInput.releaseAll(Minecraft.getInstance())); }
    }
    @Override public void onError(WebSocket conn,Exception error) { Bridge.LOG.warn("local adapter error: {}",error.toString()); }
    @Override public synchronized void onMessage(WebSocket conn,String raw) {
        if (raw.length()>16384) { conn.close(1009,"message too large"); return; }
        try {
            JsonObject m=JsonParser.parseString(raw).getAsJsonObject(); String type=m.get("t").getAsString();
            if (type.equals("attach")) {
                JsonObject reply=new JsonObject();reply.addProperty("t","attached");
                boolean ok=m.get("v").getAsInt()==1 && (host==null || host==conn);
                reply.addProperty("ok",ok);if(ok)host=conn;conn.send(reply.toString());return;
            }
            if(conn!=host)return;
            if(type.equals("cam"))HostState.update(m,conn);
            else if(java.util.Set.of("host_health","native_impact","terrain","scene").contains(type)) {
                var mc=Minecraft.getInstance();mc.execute(()->{
                    var server=mc.getSingleplayerServer();if(conn==host&&server!=null)server.execute(()->{if(conn==host)dev.fusion.bridge.GameplayEvents.accept(server,m);});
                });
            }
            else if(ClientInput.TYPES.contains(type)) {
                Minecraft mc=Minecraft.getInstance();mc.execute(()->{if(conn==host)try{ClientInput.handle(mc,m);}catch(RuntimeException e){Bridge.LOG.warn("invalid adapter input");}});
            } else if(type.equals("adapter_event")) {
                Minecraft.getInstance().execute(()->{if(conn==host)BridgeClient.adapterMessage.accept(m);});
            } else { JsonObject reply=new JsonObject();reply.addProperty("t","unsupported");reply.addProperty("type",type);conn.send(reply.toString()); }
        } catch(RuntimeException e) { conn.send("{\"t\":\"error\",\"reason\":\"invalid_message\"}"); }
    }
}
