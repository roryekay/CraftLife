package dev.fusion.bridge.client;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import dev.fusion.bridge.Bridge;

/** Host adapters convert their units/axes before sending this game-neutral pose. */
public final class HostState {
    public record Pose(long hostFrame, double x, double y, double z,
        float yaw, float pitch, float roll, float fov, boolean firstPerson,
        double px, double py, double pz, boolean control,double ex,double ey,double ez,boolean grounded,boolean crouched,boolean guestMovement,int blocked,long receivedNanos) {}
    private static volatile Pose latest;
    private static Pose frame;
    private static volatile Object owner;
    public static final long TIMEOUT_NANOS = 500_000_000L;

    static double number(JsonArray a, int i) {
        double n = a.get(i).getAsDouble();
        if (!Double.isFinite(n) || Math.abs(n) > 30_000_000) throw new IllegalArgumentException("invalid coordinate");
        return n;
    }
    static void update(JsonObject m, Object source) {
        JsonArray p=m.getAsJsonArray("p"), r=m.getAsJsonArray("r"), feet=m.getAsJsonArray("pl");
        if (p.size()!=3 || r.size()!=3 || feet==null || feet.size()!=3) throw new IllegalArgumentException("pose needs p/r/pl triples");
        float fov=m.get("fov").getAsFloat();
        if (!Float.isFinite(fov) || fov<10 || fov>170) throw new IllegalArgumentException("invalid vertical fov");
        JsonArray eye=m.has("eye")?m.getAsJsonArray("eye"):p;if(eye.size()!=3)throw new IllegalArgumentException("eye needs a triple");
        Pose next=new Pose(m.get("f").getAsLong(),number(p,0),number(p,1),number(p,2),
            (float)number(r,0),(float)number(r,1),(float)number(r,2),fov,
            !m.has("fp") || m.get("fp").getAsBoolean(),number(feet,0),number(feet,1),number(feet,2),
            !m.has("ctl") || m.get("ctl").getAsBoolean(),number(eye,0),number(eye,1),number(eye,2),m.has("gd")&&m.get("gd").getAsBoolean(),m.has("sn")&&m.get("sn").getAsBoolean(),m.has("gm")&&m.get("gm").getAsBoolean(),m.has("mb")?(m.get("mb").getAsInt()&7):0,System.nanoTime());
        owner=source; latest=next;
    }
    static void disconnect(Object source) { if (owner==source) { latest=null; owner=null; } }
    public static Pose live() {
        Pose p=latest;
        return p!=null && System.nanoTime()-p.receivedNanos()<TIMEOUT_NANOS ? p : null;
    }
    static boolean control() { Pose p=live(); return p!=null && p.control(); }
    public static Pose frame() { return frame; }
    public static void beginFrame() {
        frame=live(); Bridge.active=frame!=null;
        var mc=net.minecraft.client.Minecraft.getInstance();
        if(frame!=null&&frame.guestMovement()&&mc.player!=null){mc.player.setPos(frame.px(),frame.py(),frame.pz());mc.player.setOldPosAndRot();}
    }
}
