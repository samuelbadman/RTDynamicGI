#pragma once

#include "Scene/SceneBase.h"
#include "Math/Transform.h"
#include "Renderer/Material.h"
#include "Renderer/ProbeVolume.h"

struct InputEvent;

class DemoScene : public SceneBase
{
public:
	DemoScene();
	void Begin() final;
	void Tick(float deltaTime) final;
	void Draw(UINT perObjectConstantsRootParamIndex) final;
	void DrawImGui() final;

	Renderer::TopLevelAccelerationStructure* GetTlas() const { return tlAccelStructure.get(); }
	glm::vec3& GetProbeVolumePositionWS() { return ProbeVolume.GetVolumePosition(); }
	Renderer::ProbeVolume& GetProbeVolume() { return ProbeVolume; }
	glm::vec3& GetLightDirectionWS() { return LightDirectionWS; }
	float& GetLightIntensity() { return LightIntensity; }
	const Renderer::Material* GetMaterialsPtr() const { return MeshMaterials.data(); }
	size_t GetMaterialCount() const { return MeshMaterials.size(); }
	void SetDrawProbes(const bool draw) { DrawProbes = draw; }
	const auto& GetMeshes() const { return Meshes; }

public:
	static constexpr glm::vec3 SceneForwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
	static constexpr glm::vec3 SceneRightVector = glm::vec3(1.0f, 0.0f, 0.0f);
	static constexpr glm::vec3 SceneUpVector = glm::vec3(0.0f, 1.0f, 0.0f);

private:
	void OnInputEvent(InputEvent&& event);
	void PollInputs(float deltaTime);

private:
	static constexpr size_t SceneMeshTransformCount = 8;
	static constexpr float CameraYawSensitivity = 0.075f;
	static constexpr float CameraPitchSensitivity = 0.075f;
	static constexpr float CameraPitchMin = -90.0f;
	static constexpr float CameraPitchMax = 90.0f;
	static constexpr float CameraFlySpeed = 0.0075f;
	static constexpr glm::vec3 CameraStartPosition = glm::vec3(0.0f, 2.0f, -10.0f);
	static constexpr glm::vec3 ProbeVolumeStartPosition = glm::vec3(0.02f, 1.78f, 0.0f);
	static constexpr glm::vec3 ProbeVolumeExtents = glm::vec3(5.0f);
	static constexpr float ProbeVolumeProbeSpacing = 0.99f;
	static constexpr float ProbeVolumeDebugProbeScale = 0.05f;

	Renderer::ProbeVolume ProbeVolume;

	std::vector<std::unique_ptr<Renderer::Mesh>> Meshes;
	std::vector<std::unique_ptr<Renderer::BottomLevelAccelerationStructure>> blAccelStructures;
	std::unique_ptr<Renderer::TopLevelAccelerationStructure> tlAccelStructure;
	std::vector<Transform> MeshTransforms;
	std::vector<Renderer::Material> MeshMaterials;

	glm::vec3 LightDirectionWS = glm::vec3(-0.5f, -0.3f, 1.0f);
	float LightIntensity = 1.0f;

	bool DrawProbes = true;

	float DoorStartX;
	float DoorTargetX;
	float LerpAccum = 1.0f;
	bool OpenDoor = false;
	static constexpr float DoorOpenSpeed = 0.0001f;
	static constexpr float DoorOpenX = 5.0f;
};