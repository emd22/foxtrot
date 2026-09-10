/*
 * File:        LightProbe.hpp
 * Author:      emd22
 * Created:     08/09/2026
 * Description: Irradiance probes
 */


#pragma once

#include <Core/String.hpp>
#include <Core/Types.hpp>
#include <Math/Vec3.hpp>
#include <Renderer/Backend/Commands.hpp>
#include <Renderer/Backend/GpuBuffer.hpp>
#include <Renderer/Camera.hpp>
#include <Renderer/Limits.hpp>
#include <Renderer/RenderStage.hpp>

namespace fx {

struct ProbeData
{
	float32 SH[Limits::ProbeSHCoeffCount][4];
};

static_assert(sizeof(ProbeData) == Limits::ProbeSHCoeffCount * 4 * sizeof(float32),
			  "ProbeData must be tightly packed as 9 float4s to mirror the HLSL struct");

/// Volume descriptor uploaded to the GPU for spatial probe lookup.
struct ProbeVolumeData
{
	float32 Min[4];
	float32 InvCellSize[4];
	uint32 DimsAndCount[4]; // Grid dims XYZ + probe count in W
};

static_assert(sizeof(ProbeVolumeData) == 48, "ProbeVolumeData must mirror the HLSL ProbeVolume struct");

/// World-space boxes used for probe placement (bounds fit + push-out).
struct ProbeBoxList
{
	struct Box
	{
		Vec3f Min;
		Vec3f Max;
	};

	static constexpr uint32 scMaxBoxes = 256;
	Box Boxes[scMaxBoxes];
	uint32 Count = 0;
	Vec3f Min;
	Vec3f Max;
	bool Any = false;
};

/// Builds SH coeffs for a constant irradiance colour (matches the old flat ambient).
ProbeData MakeUniformAmbientProbe(float32 r, float32 g, float32 b);

/// Builds SH coeffs for a vertical sky/ground gradient (L00 + L10 terms).
ProbeData MakeSkyGradientProbe(const float32 sky[3], const float32 ground[3]);

class ProbeManager
{
public:
	/// Face resolution for capture bakes
	static constexpr uint32 scCaptureSize = 64;
	static constexpr uint32 scCaptureFaces = 6;

	/// Number of probes baked per frame during a grid bake
	static constexpr uint32 scProbesPerFrame = 4;

public:
	void Create();
	void Destroy();

	ProbeData* GetProbes() { return mProbes; }

	const Vec3f* GetProbePositions() const { return mProbePositions; }
	uint32 GetProbeCount() const { return Limits::MaxIrradianceProbes; }
	uint32 GetCurrentProbeIndex() const { return mCurrentProbe; }
	const ProbeVolumeData& GetVolume() const { return mVolume; }

	/// Fills all probes
	void SetUniformAmbient(float32 r, float32 g, float32 b);
	void SetSkyGradient(const float32 sky[3], const float32 ground[3]);

	void BakeFromSceneLights(const Vec3f& sunDir, const float32 sunRGB[3], const float32 ambRGB[3]);

	///////////////////////////////////
	// Probe Cubemap
	///////////////////////////////////

	void BeginCaptureBake(const Vec3f& position);
	void BeginGridBake();

	void BeginGridBakeAt(const Vec3f& center, const Vec3f& size);

	bool IsCapturePending() const { return mbCapturePending; }
	bool IsCaptureReady() const { return mbCaptureReady; }

	const Vec3f& GetCapturePosition() const { return mProbePositions[mCurrentProbe]; }

	void EnsureCaptureStage();
	renderer::RenderStage& GetCaptureStage() { return mCaptureStage; }

	void SetCaptureCamera(uint32 batch_slot, uint32 face, const PerspectiveCamera& cam)
	{
		mBatchCameras[batch_slot][face] = cam;
	}

	void CopyCaptureFaceToStaging(renderer::CommandBuffer& cmd, uint32 batch_slot, uint32 face);

	/// Starts a batch: records the first probe index and returns how many
	/// probes to capture this frame (up to scProbesPerFrame).
	uint32 BeginBatchCapture();
	void AdvanceBatchCapture() { mCurrentProbe++; }

	/// Called once all faces + copies are recorded for this frame.
	void MarkCaptureReady()
	{
		mbCapturePending = false;
		mbCaptureReady = true;
	}

	bool FinishCaptureBake();

	bool ServiceCaptureBake();

	void UploadToGpu();
	void UploadVolumeToGpu();

	//////////////////////////////////////
	// Probe cache
	/////////////////////////////////////

	String GetProbeFilePath() const;

	bool SaveProbes();
	bool LoadProbes();

private:
	bool ComputeGridPlacement();
	bool GatherPlacementBoxes(ProbeBoxList& out);
	void PlaceGridProbes(const Vec3f& gmin, const Vec3f& size, const ProbeBoxList& boxes);

	/// Projects one probe's staged faces into mProbes[probe_index].
	/// Staging slot selects the batch-local face buffers.
	bool ProjectStagedFaces(uint32 batch_slot, uint32 probe_index);

private:
	ProbeData mProbes[Limits::MaxIrradianceProbes] {};
	ProbeVolumeData mVolume {};
	bool mbInitialized = false;

	StackArray<Vec3f, Limits::MaxIrradianceProbes> mProbePositions {};

	uint32 mNumProbesPending = 0;
	uint32 mCurrentProbe = 0;
	uint32 mBatchStart = 0;

	bool mbCapturePending = false;
	bool mbCaptureReady = false;
	bool mbCaptureBuilt = false;

	renderer::RenderStage mCaptureStage;
	renderer::RawGpuBuffer mCaptureStaging[scProbesPerFrame][scCaptureFaces];
	PerspectiveCamera mBatchCameras[scProbesPerFrame][scCaptureFaces];
};

extern ProbeManager* gProbeManager;

} // namespace fx
