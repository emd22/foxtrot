#pragma once

#include <Math/Vec2.hpp>

class RenderBackend {
public:
    virtual void Init(Vec2i window_size) = 0;

    virtual ~RenderBackend() = default;
};
