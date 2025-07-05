#!/bin/bash
glslc -fshader-stage=vertex lighting.vert.glsl -o lighting.vert.spv
glslc -fshader-stage=fragment lighting.frag.glsl -o lighting.frag.spv

glslc -fshader-stage=vertex gbuffer.vert.glsl -o gbuffer.vert.spv
glslc -fshader-stage=fragment gbuffer.frag.glsl -o gbuffer.frag.spv
