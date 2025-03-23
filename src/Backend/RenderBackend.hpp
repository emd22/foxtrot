#pragma once

class RenderBackend {
public:

    virtual void Init() = 0;

    virtual ~RenderBackend() = default;
};
