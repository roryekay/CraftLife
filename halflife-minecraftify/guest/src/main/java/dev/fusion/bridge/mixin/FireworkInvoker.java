package dev.fusion.bridge.mixin;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.entity.projectile.FireworkRocketEntity;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Invoker;
@Mixin(FireworkRocketEntity.class)
public interface FireworkInvoker {
    @Invoker("explode") void minecraftify$explode(ServerLevel level);
    @Invoker("isAttachedToEntity") boolean minecraftify$attached();
}
