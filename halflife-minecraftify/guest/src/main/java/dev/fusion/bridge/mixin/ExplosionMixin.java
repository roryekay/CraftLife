package dev.fusion.bridge.mixin;
import dev.fusion.bridge.GameplayEvents;
import net.minecraft.world.level.ServerExplosion;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
@Mixin(ServerExplosion.class)
abstract class ExplosionMixin {
    @Inject(method="explode",at=@At("HEAD"))
    private void blast(CallbackInfoReturnable<Integer> ci){var e=(ServerExplosion)(Object)this;GameplayEvents.explosion(e.center(),e.radius());}
}
