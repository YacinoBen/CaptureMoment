#version 440

// --- Vertex Input ---
layout(location = 0) in vec2 position;   // Quad corner position (x, y)
layout(location = 1) in vec2 texCoord;   // Texture coordinates (u, v)

// --- Uniform Buffer Object ---
layout(std140, binding = 0) uniform buf {
    mat4 mvp;        // Model-View-Projection matrix
};

// --- Vertex Output ---
layout(location = 0) out vec2 vTexCoord;  // Interpolated texture coordinates to fragment shader

void main()
{
    // Transform vertex position using MVP matrix
    // z=0.0, w=1.0 for 2D rendering
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    
    // Pass texture coordinates to fragment shader
    vTexCoord = texCoord;
}