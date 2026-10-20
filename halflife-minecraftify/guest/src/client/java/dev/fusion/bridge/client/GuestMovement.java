package dev.fusion.bridge.client;

import com.google.gson.JsonObject;
import dev.fusion.bridge.Bridge;
import dev.fusion.bridge.GameplayEvents;
import net.minecraft.client.Minecraft;
import net.minecraft.world.phys.Vec3;

/** Vanilla client travel, including sprint/jump/glide/rocket acceleration.
 * Host hull sweeps resolve the resulting displacement against native geometry. */
public final class GuestMovement {
    private static boolean running;
    private static Vec3 lastHost;
    public static void begin(Minecraft mc){
        var p=HostState.live();
        Bridge.guestMovementActive=p!=null&&p.guestMovement();
        if(p==null||!p.guestMovement()||mc.player==null){running=false;lastHost=null;return;}
        boolean control=p.control()&&mc.gui.screen()==null&&!mc.player.isDeadOrDying();
        Vec3 host=new Vec3(p.px(),p.py(),p.pz()),velocity=mc.player.getDeltaMovement();
        if(!running||lastHost==null||lastHost.distanceToSqr(host)>64){velocity=Vec3.ZERO;mc.player.stopFallFlying();}
        if((p.blocked()&1)!=0)velocity=new Vec3(0,velocity.y,velocity.z);
        if((p.blocked()&2)!=0||(p.grounded()&&velocity.y<0))velocity=new Vec3(velocity.x,0,velocity.z);
        if((p.blocked()&4)!=0)velocity=new Vec3(velocity.x,velocity.y,0);
        mc.player.setPos(host);mc.player.setNoGravity(!control);mc.player.noPhysics=true;
        mc.player.setDeltaMovement(control?velocity:Vec3.ZERO);mc.player.setOnGround(p.grounded());
        mc.player.horizontalCollision=(p.blocked()&5)!=0;mc.player.verticalCollision=(p.blocked()&2)!=0;
        mc.player.setYRot(p.yaw());mc.player.setXRot(p.pitch());
        // UI cursor drags/releases must survive while locomotion is suspended.
        if(!control)ClientInput.releaseMovement(mc);
        running=true;lastHost=host;
    }
    public static void end(Minecraft mc){
        var p=HostState.live();
        if(!running||p==null||!p.guestMovement()||!p.control()||mc.player==null||mc.player.isDeadOrDying()||mc.gui.screen()!=null)return;
        var at=mc.player.position();var v=mc.player.getDeltaMovement();
        if(!Double.isFinite(at.x+at.y+at.z+v.x+v.y+v.z))return;
        JsonObject event=new JsonObject();event.addProperty("t","movement");
        event.add("delta",GameplayEvents.vector(at.subtract(lastHost)));event.add("velocity",GameplayEvents.vector(v));
        event.addProperty("flags",(mc.player.isFallFlying()?1:0)|(mc.player.isCrouching()?2:0));Bridge.emit(event);
    }
}
