package dev.fusion.bridge.mixin;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import dev.fusion.bridge.Bridge;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.Level;
import net.minecraft.world.level.block.state.BlockState;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** Observation only: the host decides whether/how to create collision. */
@Mixin(Level.class)
abstract class LevelMixin {
    @Inject(method="setBlock(Lnet/minecraft/core/BlockPos;Lnet/minecraft/world/level/block/state/BlockState;II)Z",at=@At("RETURN"))
    private void changed(BlockPos pos,BlockState state,int flags,int limit,CallbackInfoReturnable<Boolean> cir) {
        if(!Bridge.active||dev.fusion.bridge.GameplayEvents.applyingHost||state.is(net.minecraft.world.level.block.Blocks.BARRIER)||!cir.getReturnValueZ()||!((Object)this instanceof ServerLevel))return;
        JsonObject event=new JsonObject();event.addProperty("t","block_changed");
        JsonArray at=new JsonArray();at.add(pos.getX());at.add(pos.getY());at.add(pos.getZ());event.add("pos",at);
        event.addProperty("block",BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString());
        event.addProperty("air",state.isAir());event.addProperty("solid",state.isCollisionShapeFullBlock((Level)(Object)this,pos));Bridge.blockEvents.accept(event);Bridge.emit(event);
    }
}
