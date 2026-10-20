package dev.fusion.bridge.client.mixin;

import dev.fusion.bridge.client.FrameExporter;
import dev.fusion.bridge.client.HostState;
import net.minecraft.client.Camera;
import net.minecraft.client.DeltaTracker;
import org.joml.Quaternionf;
import org.joml.Vector3f;
import org.joml.Vector3fc;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

/** Direct host pose. Movement, rigging, vehicles and camera collision belong to adapters. */
@Mixin(Camera.class)
abstract class CameraMixin {
    @Shadow @Final private static Vector3fc FORWARDS,UP,LEFT;
    @Shadow @Final private Vector3f forwards,up,left;
    @Shadow @Final private Quaternionf rotation;
    @Shadow private float xRot,yRot,depthFar;
    @Shadow private boolean detached;
    @Shadow private int matrixPropertiesDirty;
    @Shadow protected abstract void setPosition(double x,double y,double z);
    @Inject(method="update",at=@At("HEAD"))
    private void begin(DeltaTracker delta,CallbackInfo ci) { HostState.beginFrame(); }
    @Inject(method="alignWithEntity",at=@At("TAIL"))
    private void camera(float partial,CallbackInfo ci) {
        var p=HostState.frame();if(p==null)return;
        float rad=(float)(Math.PI/180);xRot=p.pitch();yRot=p.yaw();
        rotation.rotationYXZ((float)Math.PI-p.yaw()*rad,-p.pitch()*rad,p.roll()*rad);
        FORWARDS.rotate(rotation,forwards);UP.rotate(rotation,up);LEFT.rotate(rotation,left);matrixPropertiesDirty|=3;
        setPosition(p.x(),p.y(),p.z());detached=!p.firstPerson();FrameExporter.setCameraPosition(p.x(),p.y(),p.z());
    }
    @Inject(method="calculateFov",at=@At("HEAD"),cancellable=true)
    private void fov(float partial,CallbackInfoReturnable<Float> cir) { var p=HostState.frame();if(p!=null)cir.setReturnValue(p.fov()); }
    @Inject(method="update",at=@At("TAIL"))
    private void planes(DeltaTracker delta,CallbackInfo ci) { FrameExporter.setFar(depthFar); }
}
