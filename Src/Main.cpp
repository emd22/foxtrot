
#define VMA_DEBUG_LOG(...) FxLogWarning(__VA_ARGS__)

#include "Core/FxDefines.hpp"
#include "Renderer/FxCamera.hpp"
#include "Renderer/FxLight.hpp"

/* Vulkan Memory Allocator */
#include <ThirdParty/vk_mem_alloc.h>

#include <Core/FxDefer.hpp>
#include <Core/FxLinkedList.hpp>
#include <Core/FxMemory.hpp>
#include <FxEngine.hpp>
#include <Renderer/Backend/RxFrameData.hpp>
#include <Renderer/Backend/RxShader.hpp>

#define SDL_DISABLE_OLD_NAMES
#include "FxControls.hpp"
#include "FxMaterial.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_revision.h>

#include <Asset/FxAssetManager.hpp>
#include <Asset/FxConfigFile.hpp>
#include <Asset/FxMeshGen.hpp>
#include <Asset/FxShaderCompiler.hpp>
#include <Core/FxBitset.hpp>
#include <Core/FxLog.hpp>
#include <Core/FxTypes.hpp>
#include <FxObject.hpp>
#include <FxScene.hpp>
#include <Math/Mat4.hpp>
#include <Renderer/FxPrimitiveMesh.hpp>
#include <Renderer/FxWindow.hpp>
#include <Renderer/Renderer.hpp>
#include <Renderer/RxDeferred.hpp>
#include <Script/FxScript.hpp>
#include <chrono>
#include <csignal>


FX_SET_MODULE_NAME("Main")

static bool Running = true;

static uint64 LastTick = 0;
static float DeltaTime = 1.0f;

void CheckGeneralControls()
{
    if (FxControlManager::IsKeyPressed(FxKey::FX_KEY_Q)) {
        Running = false;
    }

    // Click to lock mouse
    if (FxControlManager::IsKeyPressed(FxKey::FX_MOUSE_LEFT) && !FxControlManager::IsMouseLocked()) {
        FxControlManager::CaptureMouse();
    }
    // Escape to unlock mouse
    else if (FxControlManager::IsKeyPressed(FxKey::FX_KEY_ESCAPE) && FxControlManager::IsMouseLocked()) {
        FxControlManager::ReleaseMouse();
    }
}

#pragma clang optimize off

template <typename FuncType>
void FxSpeedTest(FuncType ft, int iterations)
{
    using namespace std::chrono;
    uint64 time_in_ns = 0;

    auto t1 = high_resolution_clock::now();
    for (int32 i = 0; i < iterations; i++) {
        ft(i);
    }
    auto t2 = high_resolution_clock::now();
    auto ns = duration_cast<nanoseconds>(t2 - t1);
    time_in_ns = ns.count() / iterations;

    const char* raw_name = typeid(FuncType).name();
    char name_buffer[256];

    FxUtil::DemangleName(raw_name, name_buffer, 256);

    FxLogInfo("Speed test took {} ns.", time_in_ns);
}

#pragma clang optimize on

void TestQuatFromEuler()
{
    FxVec3f angles(0.1, 0.0, 0.2);
    FxLogInfo("Angles: {}", angles);

    FxQuat rot_quat = FxQuat::FromAxisAngle(FxVec3f(1, 0, 0), 0.1);
    FxLogInfo("RotQuat: {}", rot_quat);

    FxQuat quat = FxQuat::FromEulerAngles(angles);
    FxLogInfo("Quaternion: {}", quat);

    FxLogInfo("Returned angles: {}", quat.GetEulerAngles());
}

// struct DeleteOnExit
// {
//     ~DeleteOnExit() { FxEngineGlobalsDestroy(); }
// };

int main()
{
    FxMemPool::GetGlobalPool().Create(50, FxUnitMebibyte);

#ifdef FX_LOG_OUTPUT_TO_FILE
    FxLogCreateFile("FoxtrotLog.log");
#endif

    // TestQuatFromEuler();
    // FxQuat quat = FxQuat::FromEulerAngles(FxVec3f(1.24, 1.24, 0));

    // FxLogInfo("Quat result: {}", quat);


    // TestNeonSin();
    // return 0;


    // TestScript();
    // return 0;

    FxConfigFile config;
    config.Load("../Config/Main.conf");

    FxShaderCompiler::CompileAllShaders("../Shaders/");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        FxModulePanic("Could not initialize SDL! (SDL err: {})\n", SDL_GetError());
    }

    FxEngineGlobalsInit();
    // Destroy the engine's globals at the end of scope
    FxDefer([] { FxEngineGlobalsDestroy(); });

    FxControlManager::Init();
    FxControlManager::GetInstance().OnQuit = [] { Running = false; };

    // catch sigabrt to avoid macOS showing "report" popup
    signal(SIGABRT,
           [](int signum)
           {
               FxLogError("Aborted!");
               exit(1);
           });

    const uint32 window_width = config.GetValue<uint32>("Width");
    const uint32 window_height = config.GetValue<uint32>("Height");

    FxRef<FxWindow> window = FxWindow::New(config.GetValue<const char*>("WindowTitle"), window_width, window_height);

    // RxRenderBackend renderer_state;
    // SetRendererBackend(&renderer_state);

    gRenderer->SelectWindow(window);
    gRenderer->Init(FxVec2u(window_width, window_height));

    gRenderer->Swapchain.CreateSwapchainFramebuffers();

    // Initialize the physics system
    gPhysics->Create();

    // gRenderer->OffscreenSemaphore.Create(gRenderer->GetDevice());

    FxRef<RxDeferredRenderer> deferred_renderer = FxMakeRef<RxDeferredRenderer>();
    deferred_renderer->Create(gRenderer->Swapchain.Extent);
    gRenderer->DeferredRenderer = deferred_renderer;

    FxAssetManager& asset_manager = FxAssetManager::GetInstance();
    FxMaterialManager& material_manager = FxMaterialManager::GetGlobalManager();

    asset_manager.Start(2);
    material_manager.Create();

    FxRef<FxPerspectiveCamera> camera = FxMakeRef<FxPerspectiveCamera>();

    // FxRef<FxObject> fireplace_object = FxAssetManager::LoadObject("../models/FireplaceRoom.glb");
    // fireplace_object->WaitUntilLoaded();

    FxScene main_scene;


    FxRef<FxAssetImage> skybox_texture = FxAssetManager::LoadImage(RxImageType::Image, "../Textures/TestCubemap.png");
    skybox_texture->WaitUntilLoaded();

    RxImage cubemap_image;
    cubemap_image.CreateLayeredImageFromCubemap(skybox_texture->Texture.Image, VK_FORMAT_R8G8B8A8_SRGB);

    auto generated_cube = FxMeshGen::MakeCube({ .Scale = 5 });

    auto generated_quad = FxMeshGen::MakeQuad({ .Scale = 1 });

    for (int i = 0; i < RendererFramesInFlight; i++) {
        RxImage& skybox_output_image = gRenderer->Swapchain.OutputImages[i];
        gRenderer->DeferredRenderer->SkyboxRenderer.SkyboxAttachments.Insert(&skybox_output_image);
    }

    gRenderer->DeferredRenderer->SkyboxRenderer.SkyAttachment = &cubemap_image;

    auto skybox_mesh = generated_cube->AsLightVolume();
    gRenderer->DeferredRenderer->SkyboxRenderer.Create(gRenderer->Swapchain.Extent, skybox_mesh);


    auto generated_sphere = FxMeshGen::MakeIcoSphere(2);

    camera->SetAspectRatio(((float32)window_width) / (float32)window_height);

    camera->Position.Z += 5.0f;
    camera->Position.Y += 4.0f;

    main_scene.SelectCamera(camera);

    // Mat4f model_matrix = Mat4f::AsTranslation(FxVec3f(0, 0, 0));
    //

    FxRef<FxMaterial> cube_material = material_manager.New("Cube Test Material",
                                                           &deferred_renderer->PlGeometryWireframe);

    cube_material->Properties.BaseColor = FxColor::FromRGBA(255, 255, 255, 255);
    cube_material->DiffuseComponent.pTexture = FxAssetImage::GetEmptyImage();

    FxRef<FxPrimitiveMesh<>> generated_cube_mesh = generated_cube->AsMesh();

    FxRef<FxObject> ground_object = FxMakeRef<FxObject>();
    ground_object->Create(generated_cube_mesh, cube_material);
    // ground_object.Scale(FxVec3f(10, 1, 10));
    ground_object->MoveBy(FxVec3f(0, -8, 0));
    ground_object->Scale(FxVec3f(10, 1.0, 10));

    ground_object->PhysicsObjectCreate(static_cast<FxPhysicsObject::PhysicsFlags>(FxPhysicsObject::PF_CreateInactive),
                                       FxPhysicsObject::PhysicsType::Static, {});
    main_scene.Attach(ground_object);

    FxRef<FxObject> helmet_object = FxAssetManager::LoadObject("../models/DamagedHelmet.glb", { .KeepInMemory = true });
    helmet_object->MoveBy(FxVec3f(0, 0, 0));
    helmet_object->RotateX(M_PI_2);

    helmet_object->WaitUntilLoaded();

    helmet_object->PhysicsObjectCreate(static_cast<FxPhysicsObject::PhysicsFlags>(0),
                                       FxPhysicsObject::PhysicsType::Dynamic, {});
    helmet_object->SetPhysicsEnabled(false);
    main_scene.Attach(helmet_object);

    gPhysics->OptimizeBroadPhase();

    // fireplace_object->RotateX(M_PI_2);

    FxSizedArray<VkDescriptorSet> sets_to_bind;
    sets_to_bind.InitSize(2);

    FxRef<FxLight> light = FxMakeRef<FxLight>();
    light->SetLightVolume(generated_sphere, true);
    light->MoveBy(FxVec3f(0.0, 2.80, 1.20));
    light->Scale(FxVec3f(25));
    light->Color = FxColor::FromRGBA(255, 10, 10, 255);

    main_scene.Attach(light);

    FxRef<FxLight> light2 = FxMakeRef<FxLight>();
    light2->SetLightVolume(generated_sphere, true);
    light2->Scale(FxVec3f(25));
    light2->MoveBy(FxVec3f(1, 0, -0.5));
    light2->Color = FxColor::sWhite;

    main_scene.Attach(light2);
    // gPhysics->bPhysicsPaused = true;

    while (Running) {
        const uint64 CurrentTick = SDL_GetTicksNS();

        DeltaTime = static_cast<float>(CurrentTick - LastTick) / 1'000'000.0;

        FxControlManager::Update();

        if (FxControlManager::IsMouseLocked()) {
            FxVec2f mouse_delta = FxControlManager::GetMouseDelta();
            mouse_delta.SetX(DeltaTime * mouse_delta.GetX() * 0.001);
            mouse_delta.SetY(DeltaTime * mouse_delta.GetY() * -0.001);

            camera->Rotate(mouse_delta.GetX(), mouse_delta.GetY());
        }

        if (FxControlManager::IsKeyDown(FxKey::FX_KEY_W)) {
            camera->Move(FxVec3f(0.0f, 0.0f, DeltaTime * 0.01f));
        }
        if (FxControlManager::IsKeyDown(FxKey::FX_KEY_S)) {
            camera->Move(FxVec3f(0.0f, 0.0f, DeltaTime * -0.01f));
        }
        if (FxControlManager::IsKeyDown(FxKey::FX_KEY_A)) {
            camera->Move(FxVec3f(DeltaTime * 0.01f, 0.0f, 0.0f));
        }
        if (FxControlManager::IsKeyDown(FxKey::FX_KEY_D)) {
            camera->Move(FxVec3f(DeltaTime * -0.01f, 0.0f, 0.0f));
        }

        if (FxControlManager::IsKeyPressed(FX_KEY_EQUALS)) {
            gRenderer->DeferredRenderer->ToggleWireframe(true);
        }
        if (FxControlManager::IsKeyPressed(FX_KEY_MINUS)) {
            gRenderer->DeferredRenderer->ToggleWireframe(false);
        }

        if (FxControlManager::IsKeyPressed(FX_KEY_O)) {
            // Print out memory pool statistics
            FxLogInfo("=== Memory Pool Stats ====");

            FxMemPool& global_pool = FxMemPool::GetGlobalPool();

            const double total_used = static_cast<double>(global_pool.GetTotalUsed()) / FxUnitMebibyte;
            const double total_capacity = static_cast<double>(global_pool.GetTotalCapacity()) / FxUnitMebibyte;

            float usage_percent = (total_used / total_capacity) * 100.0f;
            FxLogInfo("Usage: {:.02f} MiB / {:.02f} MiB ({:.02f}% In Use)", total_used, total_capacity, usage_percent);

            const FxMemPoolStatistics& stats = global_pool.GetStatistics();

            FxLogInfo("Peak Used: {:.02f} MiB", (static_cast<double>(stats.BytesAllocatedPeak) / FxUnitMebibyte));
            FxLogInfo("");
            FxLogInfo("Total Allocs: {}", stats.TotalAllocs);
            FxLogInfo("Total Frees:  {}", stats.TotalFrees);


            FxLogInfo("");
        }

        if (FxControlManager::IsKeyPressed(FxKey::FX_KEY_L)) {
            light->MoveTo(camera->Position);

            // camera.Position.Print();
            // light.mPosition.Print();
        }

        if (FxControlManager::IsKeyPressed(FxKey::FX_KEY_F)) {
            light2->bEnabled = !light2->bEnabled;
        }

        // if (FxControlManager::IsKeyPressed(FxKey::FX_KEY_Y)) {
        //     // FxAssetManager::LoadAsset(helmet_model, "../models/FireplaceRoom.glb");

        //     // helmet_object.Attach(helmet_model);
        //     // helmet_object.Attach(cheese_material);
        //     FxMemPool::GetGlobalPool().PrintAllocations();
        // }

        if (FxControlManager::IsKeyDown(FxKey::FX_KEY_R)) {
            // second_light_on = !second_light_on;

            // cube_object.MoveBy(FxVec3f(DeltaTime * 0.001f, 0, 0));
        }
        if (FxControlManager::IsKeyDown(FxKey::FX_KEY_E)) {
            // second_light_on = !second_light_on;

            // cube_object.MoveBy(FxVec3f(DeltaTime * -0.001f, 0, 0));
        }

        if (FxControlManager::IsKeyPressed(FxKey::FX_KEY_0)) {
            // cube_object.SetPhysicsEnabled(false);
            // cube_object.MoveTo(FxVec3f(5, 10, 0));
        }

        if (FxControlManager::IsKeyPressed(FxKey::FX_KEY_P)) {
            helmet_object->SetPhysicsEnabled(!helmet_object->GetPhysicsEnabled());
            // gRenderer->DeferredRenderer->RebuildLightingPipeline();
            // gPhysics->bPhysicsPaused = !gPhysics->bPhysicsPaused;
            // cube_object.SetPhysicsEnabled(!cube_object.GetPhysicsEnabled());
            // FxLogInfo("Fireplace dimensions: {}", fireplace_object->Dimensions);
        }

        CheckGeneralControls();

        camera->Update();

        gPhysics->Update();

        if (gRenderer->BeginFrame(*deferred_renderer) != RxFrameResult::Success) {
            continue;
        }

        // deferred_renderer->SkyboxRenderer.Render(gRenderer->GetFrame()->CommandBuffer, *camera);

        if (light2->bEnabled) {
            light2->MoveTo(camera->Position + (camera->Direction * 1.0));
        }

        main_scene.Render();

        gRenderer->DoComposition(*camera);

        LastTick = CurrentTick;
    }

    gRenderer->GetDevice()->WaitForIdle();

    material_manager.Destroy();
    asset_manager.Shutdown();

    skybox_mesh->IsReference = false;
    skybox_mesh->Destroy();

    gRenderer->DeferredRenderer->SkyboxRenderer.Destroy();

    deferred_renderer->Destroy();

    // composition_pipeline.Destroy();
    //
    generated_cube->Destroy();
    generated_sphere->Destroy();

    return 0;
}
