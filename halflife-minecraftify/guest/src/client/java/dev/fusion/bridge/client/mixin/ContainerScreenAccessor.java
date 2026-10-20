package dev.fusion.bridge.client.mixin;
import net.minecraft.client.gui.screens.inventory.AbstractContainerScreen;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
@Mixin(AbstractContainerScreen.class)
public interface ContainerScreenAccessor {
    @Accessor("leftPos") int minecraftify$left();
    @Accessor("topPos") int minecraftify$top();
}
