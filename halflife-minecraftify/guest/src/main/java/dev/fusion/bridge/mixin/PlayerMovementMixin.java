package dev.fusion.bridge.mixin;
import dev.fusion.bridge.Bridge;
import net.minecraft.world.entity.player.Player;
import org.objectweb.asm.Opcodes;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
/** Native hull sweeps replace the coarse sampled terrain collision, while
 * vanilla Minecraft retains travel, acceleration and elytra physics. */
@Mixin(Player.class)
abstract class PlayerMovementMixin {
    @Inject(method="tick",at=@At(value="FIELD",target="Lnet/minecraft/world/entity/player/Player;noPhysics:Z",opcode=Opcodes.PUTFIELD,shift=At.Shift.AFTER))
    private void collisionOwner(CallbackInfo ci){if(Bridge.active&&Bridge.guestMovementActive)((Player)(Object)this).noPhysics=true;}
}
