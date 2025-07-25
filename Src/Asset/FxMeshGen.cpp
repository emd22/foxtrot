#include "FxMeshGen.hpp"

#include <Math/Vec2.hpp>
#include <Math/Vec3.hpp>
#include <unordered_map>

#include <Renderer/FxMesh.hpp>

static const float Z = (1.0f + sqrt(5.0f)) / 2.0f; // Golden ratio
static const FxVec2f UV = FxVec2f(1 / 11.0f, 1 / 3.0f); // The UV coordinates are laid out in a 11x3 grid

static const int IcoVertexCount = 22;
static const int IcoIndexCount = 60;

static const FxVec3f IcoVerts[] = {
	FxVec3f( 0, -1, -Z), FxVec3f(-1, -Z,  0), FxVec3f( Z,  0, -1), FxVec3f( 1, -Z,  0),
	FxVec3f( 1,  Z,  0), FxVec3f(-1, -Z,  0), FxVec3f( Z,  0,  1), FxVec3f( 0, -1,  Z),
	FxVec3f( 1,  Z,  0), FxVec3f(-1, -Z,  0), FxVec3f( 0,  1,  Z), FxVec3f(-Z,  0,  1),
	FxVec3f( 1,  Z,  0), FxVec3f(-1, -Z,  0), FxVec3f(-1,  Z,  0), FxVec3f(-Z,  0, -1),
	FxVec3f( 1,  Z,  0), FxVec3f(-1, -Z,  0), FxVec3f( 0,  1, -Z), FxVec3f( 0, -1, -Z),
	FxVec3f( 1,  Z,  0), FxVec3f( Z,  0, -1)
};

static const FxVec2f IcoUvs[] = {
	UV * FxVec2f( 0, 1),
	UV * FxVec2f( 1, 0),
	UV * FxVec2f( 1, 2),
	UV * FxVec2f( 2, 1),
	UV * FxVec2f( 2, 3),
	UV * FxVec2f( 3, 0),
	UV * FxVec2f( 3, 2),
	UV * FxVec2f( 4, 1),
	UV * FxVec2f( 4, 3),
	UV * FxVec2f( 5, 0),
	UV * FxVec2f( 5, 2),
	UV * FxVec2f( 6, 1),
	UV * FxVec2f( 6, 3),
	UV * FxVec2f( 7, 0),
	UV * FxVec2f( 7, 2),
	UV * FxVec2f( 8, 1),
	UV * FxVec2f( 8, 3),
	UV * FxVec2f( 9, 0),
	UV * FxVec2f( 9, 2),
	UV * FxVec2f(10, 1),
	UV * FxVec2f(10, 3),
	UV * FxVec2f(11, 2)
};

static const int32 IcoIndex[] = {
	 2,  6,  4, // Top
	 6, 10,  8,
	10, 14, 12,
	14, 18, 16,
	18, 21, 20,

	 0,  3,  2, // Middle
	 2,  3,  6,
	 3,  7,  6,
	 6,  7, 10,
	 7, 11, 10,
	10, 11, 14,
	11, 15, 14,
	14, 15, 18,
	15, 19, 18,
	18, 19, 21,

	 0,  1,  3, // Bottom
	 3,  5,  7,
	 7,  9, 11,
	11, 13, 15,
	15, 17, 19
};



FxRef<FxMesh<FxMeshGen::LightVolumeVertex>> FxMeshGen::GeneratedMesh::AsLightVolume()
{
   	FxRef<FxMesh<FxMeshGen::LightVolumeVertex>> mesh = FxMakeRef<FxMesh<FxMeshGen::LightVolumeVertex>>();
    mesh->IsReference = true;

    FxSizedArray<RvkVertex<FxVertexPosition>> points(Positions.Size);

    for (uint32 i = 0; i < Positions.Size; i++) {
        auto* vertex = points.Insert();
        FxVec3f& vec = Positions[i];

        vertex->Position[0] = vec.X;
        vertex->Position[1] = vec.Y;
        vertex->Position[2] = vec.Z;
    }

	// Create the mesh using our bodged point array
	mesh->CreateFromData(points, Indices);

	return mesh;
}

FxRef<FxMesh<>> FxMeshGen::GeneratedMesh::AsMesh()
{
    FxRef<FxMesh<>> mesh = FxMakeRef<FxMesh<>>();
    mesh->IsReference = true;

    auto vertices = FxMesh<>::MakeCombinedVertexBuffer(Positions);
    mesh->CreateFromData(vertices, Indices);

    return mesh;
}

// Implementation is based on code from https://winter.dev/projects/mesh/icosphere
FxRef<FxMeshGen::GeneratedMesh> FxMeshGen::MakeIcoSphere(int resolution) {
	const int rn = (int)pow(4, resolution);

	const int total_index_count = IcoIndexCount * rn;
	const int total_vertex_count = IcoVertexCount + IcoIndexCount * (1 - rn) / (1 - 4);

	FxSizedArray<FxVec3f> positions;
	positions.InitSize(total_vertex_count);

	FxSizedArray<uint32> indices;
	indices.InitSize(total_index_count);

	for (int i = 0; i < IcoVertexCount; i++) {  // Copy in initial mesh
		positions[i] = IcoVerts[i];
	}

	for (int i = 0; i < IcoIndexCount; i++) {
		indices[i] = IcoIndex[i];
	}

	uint32 current_index_count = IcoIndexCount;
	uint32 current_vert_count = IcoVertexCount;

	for (int r = 0; r < resolution; r++) {
		std::unordered_map<uint64_t, int> triangleFromEdge;
		uint32 index_count = current_index_count;

		for (int t = 0; t < index_count; t += 3) {
			int midpoints[3] {};

			for (int e = 0; e < 3; e++) {
				uint32 first = indices[t + e];
				uint32 second = indices[t + (t + e + 1) % 3];

				if (first > second) {
					std::swap(first, second);
				}

				uint64_t hash = (uint64_t)first | (uint64_t)second << (sizeof(uint32_t) * 8);

				auto [triangle, was_new_edge] = triangleFromEdge.insert({ hash, current_vert_count });

				if (was_new_edge) {
					positions[current_vert_count] = (positions[first] + positions[second]) / 2.0f;

					current_vert_count += 1;
				}

				midpoints[e] = triangle->second;
			}

			int mid0 = midpoints[0];
			int mid1 = midpoints[1];
			int mid2 = midpoints[2];

			indices[current_index_count++] = indices[t];
			indices[current_index_count++] = mid0;
			indices[current_index_count++] = mid2;

			indices[current_index_count++] = indices[t + 1];
			indices[current_index_count++] = mid1;
			indices[current_index_count++] = mid0;

			indices[current_index_count++] = indices[t + 2];
			indices[current_index_count++] = mid2;
			indices[current_index_count++] = mid1;

			indices[t]     = mid0; // Overwrite the original triangle with the 4th new triangle
			indices[t + 1] = mid1;
			indices[t + 2] = mid2;
		}
	}

	// Normalize all of the positions
	for (FxVec3f& pos : positions) {
	    pos.NormalizeIP();
	}

	FxRef<FxMeshGen::GeneratedMesh> mesh = FxMakeRef<FxMeshGen::GeneratedMesh>();
	mesh->Positions = std::move(positions);
	mesh->Indices = std::move(indices);

	return mesh;
}
