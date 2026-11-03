#include "gfx.hpp"

const Vertex kVertices[3] = {
    {{0.0f, 0.5f}, {1.0f, 0.0f, 0.0f}},
    {{-0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}},
};

const Vertex kQuad[6] = {
    {{0.0f, 0.0f}},
    {{1.0f, 0.0f}},
    {{0.0f, 1.0f}},
    {{0.0f, 1.0f}},
    {{1.0f, 0.0f}},
    {{1.0f, 1.0f}},
};

const char *kShader = R"(
struct VSIn {
    @location(0) pos   : vec2f,
    @location(1) color : vec3f,
};

struct VSOut {
    @builtin(position) pos : vec4f,
    @location(0)       color : vec3f,
};

@vertex
fn vs_main(in : VSIn) -> VSOut {
    var out : VSOut;
    out.pos = vec4f(in.pos, 0.0, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in : VSOut) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);
}
)";

const char *RectShader = R"(
struct VSIn {
    @location(0) local : vec2f,

    @location(1) rect  : vec4f,
    @location(2) color : vec4f,
};

struct VSOut {
    @builtin(position) pos : vec4f,
    @location(0) color : vec4f,
};

@vertex
fn vs_main(in : VSIn) -> VSOut {
    var out : VSOut;

    let p = in.rect.xy + in.local * in.rect.zw;

    // Convert normalized top-left coordinates to clip space.
    out.pos = vec4f(
        p.x * 2.0 - 1.0,
        1.0 - p.y * 2.0,
        0.0,
        1.0
    );

    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in : VSOut) -> @location(0) vec4f {
    return in.color;
}
)";
