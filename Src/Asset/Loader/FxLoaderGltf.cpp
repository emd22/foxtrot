#include "FxLoaderGltf.hpp"
#include "Asset/FxAssetBase.hpp"
#include "Renderer/FxPrimitiveMesh.hpp"

#include <ThirdParty/cgltf.h>

#include <Asset/FxAssetModel.hpp>
#include <FxObject.hpp>

#include <Core/FxRef.hpp>

FxRef<FxAssetImage> LoadTexture(const FxRef<FxMaterial>& material, const cgltf_texture_view& texture_view);

void UnpackMeshAttributes(FxRef<FxPrimitiveMesh<>>& mesh, cgltf_primitive* primitive)
{
    FxSizedArray<float32> positions;
    FxSizedArray<float32> normals;
    FxSizedArray<float32> uvs;

    for (int i = 0; i < primitive->attributes_count; i++) {
        auto *attribute = &primitive->attributes[i];


        if (attribute->type == cgltf_attribute_type_position) {
            cgltf_size data_size = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

            // Create our vertex position buffer
            positions.InitSize(data_size);
            cgltf_accessor_unpack_floats(attribute->data, positions.Data, data_size);
        }
        else if (attribute->type == cgltf_attribute_type_normal) {
            cgltf_size data_size = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

            // Create our vertex normal buffer
            normals.InitSize(data_size);
            cgltf_accessor_unpack_floats(attribute->data, normals.Data, data_size);
        }
        else if (attribute->type == cgltf_attribute_type_texcoord) {
            cgltf_size data_size = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

            // Create our vertex normal buffer
            uvs.InitSize(data_size);
            cgltf_accessor_unpack_floats(attribute->data, uvs.Data, data_size);
        }
    }

    auto combined_vertices = FxPrimitiveMesh<>::MakeCombinedVertexBuffer(positions, normals, uvs);

    // Set the combined vertices to the mesh
    mesh->UploadVertices(combined_vertices);
    mesh->IsReady = true;
}

//void CreateMaterialFromGltfMesh(FxRef<FxObject>& object, cgltf_material* gltf_material)
//{
////    cgltf_material* gltf_material = primitive->material;
//    if (!gltf_material) {
//        return;
//    }
//
//    FxRef<FxMaterial> material = FxRef<FxMaterial>::New();
//
//    if (gltf_material->has_pbr_metallic_roughness) {
//        material->DiffuseTexture.Texture = LoadTexture(material, gltf_material->pbr_metallic_roughness.base_color_texture);
//    }
//
//    object->Material = material;
//}

//void FxLoaderGltf::LoadAttachedMaterials()
//{
//    for (FxGltfMaterialToLoad& to_load : MaterialsToLoad) {
//        CreateMaterialFromGltfMesh(to_load.Object, mGltfData->meshes[to_load.MeshIndex].primitives[to_load.PrimitiveIndex].material);
//    }
//}

void MakeMaterialTextureForPrimitive(FxRef<FxMaterial>& material, FxMaterialComponent& component, cgltf_texture_view& texture_view)
{
    if (!texture_view.texture) {
        return;
    }

    const uint8* image_buffer = cgltf_buffer_view_data(texture_view.texture->image->buffer_view);
    uint32 image_buffer_size = static_cast<uint32>(texture_view.texture->image->buffer_view->size);

    // Stage that shit so we can nuke mGltfData as soon as we can
    uint8* goober_buffer = FxMemPool::Alloc<uint8>(image_buffer_size);
    memcpy(goober_buffer, image_buffer, image_buffer_size);

    component.DataToLoad = FxMakeSlice(const_cast<const uint8*>(goober_buffer), image_buffer_size);
}

void FxLoaderGltf::MakeEmptyMaterialTexture(FxRef<FxMaterial>& material, FxMaterialComponent& component)
{
    FxSizedArray<uint8> image_data = { 255, 255, 255, 255 };

    component.Texture = FxMakeRef<FxAssetImage>();
    component.Texture->Texture.Create(image_data, FxVec2u(1, 1), VK_FORMAT_R8G8B8A8_SRGB, 4);
    // component.Texture->IsFinishedNotifier.SignalDataWritten();
    component.Texture->IsUploadedToGpu = true;
    component.Texture->IsUploadedToGpu.notify_all();

    component.Texture->mIsLoaded.store(true);
}


void FxLoaderGltf::MakeMaterialForPrimitive(FxRef<FxObject>& object, cgltf_primitive* primitive)
{
    cgltf_material* gltf_material = primitive->material;

    if (!gltf_material) {
        return;
    }

    FxRef<FxMaterial> material = FxMaterialManager::New("Fireplace", &Renderer->DeferredRenderer->GPassPipeline);

    // For some reason the peeber metallic roughness holds our diffuse texture
    if (gltf_material->has_pbr_metallic_roughness) {
        MakeMaterialTextureForPrimitive(material, material->DiffuseTexture, gltf_material->pbr_metallic_roughness.base_color_texture);
        material->Properties.BaseColor = FxColorFromRGBA(255, 255, 255, 255);
    }
    // There is no albedo texture on the model, use the base colour.
    else {
        MakeEmptyMaterialTexture(material, material->DiffuseTexture);
        material->Properties.BaseColor = FxColorFromFloats(gltf_material->pbr_metallic_roughness.base_color_factor);
    }

    object->Material = material;
}

void FxLoaderGltf::UploadMeshToGpu(FxRef<FxObject>& object, cgltf_mesh *gltf_mesh, int mesh_index)
{
    const bool has_multiple_primitives = gltf_mesh->primitives_count > 1;

    // Assume at first that there is only one primitive;
    FxRef<FxObject> current_object = object;

    // Similarly to `CreateGpuResource`, we are going to make the `object` into a container
    // if there are multiple primitives.
    if (has_multiple_primitives) {
        current_object = FxMakeRef<FxObject>();
    }

    for (int i = 0; i < gltf_mesh->primitives_count; i++) {
        auto* primitive = &gltf_mesh->primitives[i];

        FxSizedArray<uint32> indices;

        FxRef<FxPrimitiveMesh<>> primitive_mesh = FxMakeRef<FxPrimitiveMesh<>>();

        // if there are indices in the mesh, add them to the FxPrimitiveMesh
        if (primitive->indices != nullptr) {
            indices.InitSize(primitive->indices->count);

            cgltf_accessor_unpack_indices(
                primitive->indices,
                indices.Data,
                sizeof(uint32),
                primitive->indices->count
            );

            // Set the mesh indices
            primitive_mesh->UploadIndices(indices);
        }

        UnpackMeshAttributes(primitive_mesh, primitive);

        MakeMaterialForPrimitive(current_object, primitive);


//        CreateMaterialFromPrimitive(current_object, primitive);
//        FxGltfMaterialToLoad material{
//            .PrimitiveIndex = i,
////            .GltfMaterial = gltf_mesh->primitives[i].material,
//            .Object = current_object,
//            .MeshIndex = mesh_index
//        };



//        MaterialsToLoad.push_back(material);

        current_object->Mesh = primitive_mesh;

        if (has_multiple_primitives) {
            // Attach the current object to the object container (our output)
            object->AttachObject(current_object);

            // Create a new object to load into next
            current_object = FxMakeRef<FxObject>();
        }

        // model->Meshes.Data[i] = primtive_mesh;
    }

//    object = current_object;

    Log::Info("Add primitive:", 0);

}

#include <FxMaterial.hpp>
#include <iostream>

#include <Asset/FxAssetManager.hpp>



//FxRef<FxAssetImage> LoadTexture(const FxRef<FxMaterial>& material, const cgltf_texture_view& texture_view)
//{
//    if (!texture_view.texture) {
//        return FxRef<FxAssetImage>(nullptr);
//    }
//
//    if (texture_view.texture->image->uri != nullptr) {
//        std::cout << "Texture URI: " << texture_view.texture->image->uri << '\n';
//        return FxRef<FxAssetImage>(nullptr);
//    }
//
//    if (texture_view.texture->image != nullptr) {
//        const uint8* data = cgltf_buffer_view_data(texture_view.texture->image->buffer_view);
//
//        uint32 size = texture_view.texture->image->buffer_view->size;
//
//        FxRef<FxAssetImage> texture = FxAssetManager::LoadFromMemory<FxAssetImage>(data, size);
//
//        // Since this is being loaded on another thread anyway, this shouldn't cause too much of an issue.
////        texture->WaitUntilLoaded();
//
//        return texture;
//    }
//    else {
//        std::cout << "no image added\n";
//    }
//
//    return FxRef<FxAssetImage>(nullptr);
//}

FxLoaderGltf::Status FxLoaderGltf::LoadFromFile(FxRef<FxAssetBase> asset, const std::string& path)
{
//    FxRef<FxAssetModel> model(asset);

    cgltf_options options{};

    cgltf_result status = cgltf_parse_file(&options, path.c_str(), &mGltfData);
    if (status != cgltf_result_success) {
        Log::Error("Error parsing GLTF file! (path: %s)", path.c_str());
        return FxLoaderGltf::Status::Error;
    }

    status = cgltf_load_buffers(&options, mGltfData, path.c_str());
    if (status != cgltf_result_success) {
        Log::Error("Error loading buffers from GLTF file! (path: %s)", path.c_str());

        return FxLoaderGltf::Status::Error;
    }


    std::cout << "cgltf textures; " << mGltfData->textures_count << '\n';


    // if (mGltfData->materials_count) {
    //     for (int i = 0; i < mGltfData->materials_count; i++) {
    //         cgltf_material& gltf_material = mGltfData->materials[i];

    //         FxRef<FxMaterial> material = FxRef<FxMaterial>::New();


    //         if (gltf_material.has_pbr_metallic_roughness) {
    //             material->DiffuseTexture = LoadTexture(material, gltf_material.pbr_metallic_roughness.base_color_texture);

    //         }



    //         model->Materials.push_back(material);
    //     }
    // }

    return FxLoaderGltf::Status::Success;
}

FxLoaderGltf::Status FxLoaderGltf::LoadFromMemory(FxRef<FxAssetBase> asset, const uint8* data, uint32 size)
{
    // (void)asset;
//    FxRef<FxAssetModel> model(asset);

    cgltf_options options{};

    cgltf_result status = cgltf_parse(&options, data, size, &mGltfData);
    if (status != cgltf_result_success) {
        Log::Error("Error parsing GLTF file from data");
        return FxLoaderGltf::Status::Error;
    }

    std::cout << "cgltf textures; " << mGltfData->textures_count << '\n';

    // if (mGltfData->materials_count) {
    //     for (int i = 0; i < mGltfData->materials_count; i++) {
    //         cgltf_material& gltf_material = mGltfData->materials[i];

    //         FxRef<FxMaterial> material = FxRef<FxMaterial>::New();


    //         if (gltf_material.has_pbr_metallic_roughness) {
    //             material->DiffuseTexture = LoadTexture(material, gltf_material.pbr_metallic_roughness.base_color_texture);
    //         }

    //         model->Materials.push_back(material);
    //     }
    // }

    return FxLoaderGltf::Status::Success;
}

void FxLoaderGltf::CreateGpuResource(FxRef<FxAssetBase>& asset)
{
    FxRef<FxObject> output_object(asset);

    // If there is only one mesh to load, store the mesh directly in the output object
    FxRef<FxObject> current_object = output_object;

    // If there are multiple gltf meshes, we will need to use the output object as a
    // container for multiple other meshes
    const bool has_multiple_meshes = mGltfData->meshes_count > 1;
    if (has_multiple_meshes) {
        current_object = FxMakeRef<FxObject>();
    }

    for (int mesh_index = 0; mesh_index < mGltfData->meshes_count; mesh_index++) {
        cgltf_mesh* mesh = &mGltfData->meshes[mesh_index];

        printf("Primitives Count: %zu\n", mesh->primitives_count);

        // object->Meshes.InitSize(mesh->primitives_count);

        UploadMeshToGpu(current_object, mesh, mesh_index);

        if (has_multiple_meshes) {
            // Attach the current object to the object container (our output)
            output_object->AttachObject(current_object);

            // Create a new object to load into next
            current_object = FxMakeRef<FxObject>();
        }
    }

//    cgltf_free(mGltfData);
//    mGltfData = nullptr;


    asset = output_object;
//    current_object.mPtr = nullptr;
//    current_object.mRefCnt = nullptr;


    asset->IsUploadedToGpu = true;
    asset->IsUploadedToGpu.notify_all();

}

void FxLoaderGltf::Destroy(FxRef<FxAssetBase>& asset)
{
//    while (!asset->IsUploadedToGpu) {
//        asset->IsUploadedToGpu.wait(true);
//    }

    if (mGltfData) {
       printf("Destroyed mesh\n");
       cgltf_free(mGltfData);
       mGltfData = nullptr;
    }
}
