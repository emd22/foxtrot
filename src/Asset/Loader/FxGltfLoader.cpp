#include "FxGltfLoader.hpp"
#include "Asset/FxBaseAsset.hpp"
#include "Renderer/FxMesh.hpp"

#include <ThirdParty/cgltf.h>

#include <Asset/FxModel.hpp>

void UnpackMeshAttributes(FxMesh *mesh, cgltf_primitive *primitive)
{
    FxStaticArray<float32> positions;
    FxStaticArray<float32> normals;

    for (int i = 0; i < primitive->attributes_count; i++) {
        auto *attribute = &primitive->attributes[i];


        if (attribute->type == cgltf_attribute_type_position) {
            printf("Attribute -> Vertex Buffer\n");

            cgltf_size data_size = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

            // Create our vertex position buffer
            positions.InitSize(data_size);
            cgltf_accessor_unpack_floats(attribute->data, positions.Data, data_size);
        }
        else if (attribute->type == cgltf_attribute_type_normal) {
            printf("Attribute -> Normal Buffer\n");

            cgltf_size data_size = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);

            // Create our vertex normal buffer
            normals.InitSize(data_size);
            cgltf_accessor_unpack_floats(attribute->data, normals.Data, data_size);
        }
    }

    auto combined_vertices = FxMesh::MakeCombinedVertexBuffer(positions, normals);

    // Set the combined vertices to the mesh
    mesh->UploadVertices(combined_vertices);
    mesh->IsReady = true;
}

void UploadMeshToGpu(FxModel *model, cgltf_mesh *gltf_mesh, int mesh_index)
{
    for (int i = 0; i < gltf_mesh->primitives_count; i++) {
        auto *primitive = &gltf_mesh->primitives[i];

        FxStaticArray<uint32> indices;

        FxMesh *mesh = new FxMesh;

        // if there are indices in the mesh, add them to the FxMesh
        if (primitive->indices != nullptr) {
            indices.InitSize(primitive->indices->count);

            cgltf_accessor_unpack_indices(
                primitive->indices,
                indices.Data,
                sizeof(uint32),
                primitive->indices->count
            );

            // Set the mesh indices
            mesh->UploadIndices(indices);
        }

        UnpackMeshAttributes(mesh, primitive);

        model->Meshes.Data[i] = mesh;
    }
    Log::Info("Add mesh:", 0);

}

FxGltfLoader::Status FxGltfLoader::LoadFromFile(FxBaseAsset *asset, std::string path)
{
    cgltf_options options{};

    cgltf_result status = cgltf_parse_file(&options, path.c_str(), &mGltfData);
    if (status != cgltf_result_success) {
        Log::Error("Error parsing GLTF file! (path: %s)", path.c_str());
        return FxGltfLoader::Status::Error;
    }

    status = cgltf_load_buffers(&options, mGltfData, path.c_str());
    if (status != cgltf_result_success) {
        Log::Error("Error loading buffers from GLTF file! (path: %s)", path.c_str());

        return FxGltfLoader::Status::Error;
    }

    return FxGltfLoader::Status::Success;
}

void FxGltfLoader::CreateGpuResource(FxBaseAsset *asset)
{
    FxModel *model = static_cast<FxModel *>(asset);
    model->Meshes.InitSize(mGltfData->meshes_count);

    for (int i = 0; i < mGltfData->meshes_count; i++) {
        auto *mesh = &mGltfData->meshes[i];

        UploadMeshToGpu(model, mesh, i);
    }

    cgltf_free(mGltfData);
    mGltfData = nullptr;

    model->IsUploadedToGpu = true;
    model->IsUploadedToGpu.notify_all();
}

void FxGltfLoader::Destroy(FxBaseAsset *asset)
{
    if (mGltfData) {
        cgltf_free(mGltfData);
        mGltfData = nullptr;
    }
}
