package dev.fusion.bridge.mixin;
import dev.fusion.bridge.Bridge;
import net.minecraft.world.entity.Entity;
import net.minecraft.world.entity.player.Player;
import net.minecraft.world.phys.Vec3;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
/** Interaction origin stays at the host player's eyes, including third person/crouch. */
@Mixin(Entity.class)
abstract class InteractionEyeMixin {
    @Inject(method={"getEyePosition()Lnet/minecraft/world/phys/Vec3;","getEyePosition(F)Lnet/minecraft/world/phys/Vec3;"},at=@At("HEAD"),cancellable=true)
    private void eye(CallbackInfoReturnable<Vec3> ci){if(Bridge.active&&Bridge.interactionEye!=null&&(Object)this instanceof Player)ci.setReturnValue(Bridge.interactionEye);}
}
