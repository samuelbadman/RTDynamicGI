#pragma once

#include "Renderer/Renderer.h"

class SceneBase
{
public:
	virtual ~SceneBase() = default;

	virtual void Begin() = 0;
	virtual void Tick(float deltaTime) = 0;
	virtual void Draw(UINT perObjectConstantsRootParamIndex) = 0;
	virtual void DrawImGui() = 0;

	const Renderer::Camera& GetMainCamera() const { return MainCamera; }

protected:
	Renderer::Camera MainCamera = {};
};