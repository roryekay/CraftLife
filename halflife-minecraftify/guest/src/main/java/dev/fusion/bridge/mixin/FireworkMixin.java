package dev.fusion.bridge.mixin;
import dev.fusion.bridge.GameplayEvents;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.entity.projectile.FireworkRocketEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
@Mixin(FireworkRocketEntity.class)
abstract class FireworkMixin {
    @Inject(method="explode",at=@At("HEAD"))
    private void explode(ServerLevel level,CallbackInfo ci){if(!((FireworkInvoker)this).minecraftify$attached())GameplayEvents.explosion(((FireworkRocketEntity)(Object)this).position(),2);}
}
