package dev.fusion.bridge.client.mixin;
import dev.fusion.bridge.client.HostState;
import net.minecraft.client.Minecraft;
import net.minecraft.client.MouseHandler;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Redirect;
/** A live adapter owns mouse capture in the host window. Held mining still
 * goes through Minecraft's normal block damage, tool speed and durability. */
@Mixin(Minecraft.class)
abstract class RemoteMiningMixin {
    @Redirect(method="handleKeybinds",at=@At(value="INVOKE",target="Lnet/minecraft/client/MouseHandler;isMouseGrabbed()Z"))
    private boolean captured(MouseHandler mouse){var pose=HostState.live();return (pose!=null&&pose.control())||mouse.isMouseGrabbed();}
}
