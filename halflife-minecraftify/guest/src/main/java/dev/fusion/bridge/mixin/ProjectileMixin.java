package dev.fusion.bridge.mixin;
import dev.fusion.bridge.GameplayEvents;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.entity.projectile.Projectile;
import net.minecraft.world.phys.HitResult;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(Projectile.class)
abstract class ProjectileMixin {
    @Inject(method="shoot(DDDFF)V",at=@At("HEAD"))
    private void launch(double x,double y,double z,float speed,float spread,CallbackInfo ci){
        var p=(Projectile)(Object)this;var eye=dev.fusion.bridge.Bridge.interactionEye;
        if(dev.fusion.bridge.Bridge.active&&eye!=null&&p.level() instanceof ServerLevel&&p.getOwner() instanceof net.minecraft.server.level.ServerPlayer)p.setPos(eye.x,eye.y-.1,eye.z);
    }
    @Inject(method="onHit",at=@At("HEAD"))
    private void hit(HitResult hit,CallbackInfo ci){
        var p=(Projectile)(Object)this;if(!(p.level() instanceof ServerLevel))return;
        GameplayEvents.projectile(p,p.oldPosition(),hit.getLocation().add(p.getDeltaMovement().normalize().scale(.05)));
    }
}
