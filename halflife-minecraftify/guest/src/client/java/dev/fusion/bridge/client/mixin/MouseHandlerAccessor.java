package dev.fusion.bridge.client.mixin;
import net.minecraft.client.MouseHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;
@Mixin(MouseHandler.class)
public interface MouseHandlerAccessor {
    @Accessor("xpos") void minecraftify$x(double value);
    @Accessor("ypos") void minecraftify$y(double value);
}
