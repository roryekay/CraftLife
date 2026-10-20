package dev.fusion.bridge.mixin;
import dev.fusion.bridge.Bridge;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.tags.DamageTypeTags;
import net.minecraft.world.damagesource.DamageSource;
import net.minecraft.world.entity.LivingEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;
/** Native explosions already damage the host player. Do not apply that blast twice. */
@Mixin(LivingEntity.class)
abstract class PlayerDamageMixin {
    @Inject(method="hurtServer",at=@At("HEAD"),cancellable=true)
    private void damage(ServerLevel level,DamageSource source,float amount,CallbackInfoReturnable<Boolean> ci){
        if(!Bridge.active&&(Object)this instanceof ServerPlayer&&Boolean.getBoolean("minecraftify.autoWorld")&&level.getServer().getWorldData().getLevelName().equals("Minecraftify Sandbox")){ci.setReturnValue(false);return;}
        if(Bridge.active&&(Object)this instanceof ServerPlayer&&(source.is(DamageTypeTags.IS_EXPLOSION)||source.is(net.minecraft.world.damagesource.DamageTypes.ENDER_PEARL)||source.is(net.minecraft.world.damagesource.DamageTypes.FALL)||source.is(net.minecraft.world.damagesource.DamageTypes.FIREWORKS)))ci.setReturnValue(false);
    }
}
