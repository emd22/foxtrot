#pragma once

#include <FxEntity.hpp>
#include <FxObject.hpp>
#include <Renderer/FxCamera.hpp>
#include <Renderer/FxLight.hpp>

class FxScene
{
public:
    FxScene() { Create(); }

    void Create();

    void Attach(const FxRef<FxObject>& object);
    void Attach(const FxRef<FxLight>& light);

    void SelectCamera(const FxRef<FxCamera>& camera) { mpCurrentCamera = camera; }
    void Render();

    void Destroy();

    ~FxScene() { Destroy(); }

private:
    FxPagedArray<FxRef<FxObject>> mObjects;
    FxPagedArray<FxRef<FxLight>> mLights;

    FxRef<FxCamera> mpCurrentCamera { nullptr };
};
