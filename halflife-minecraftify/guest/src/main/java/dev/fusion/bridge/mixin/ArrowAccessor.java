package dev.fusion.bridge.mixin;
import net.minecraft.world.entity.projectile.arrow.AbstractArrow;
import net.minecraft.world.item.ItemStack;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
@Mixin(AbstractArrow.class)
public interface ArrowAccessor {
    @Accessor("baseDamage") double minecraftify$baseDamage();
    @Accessor("firedFromWeapon") ItemStack minecraftify$weapon();
}
