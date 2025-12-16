#version 440

// --- Fragment Input ---
layout(location = 0) in vec2 vTexCoord;  // Interpolated texture coordinates from vertex shader

// --- Texture Sampler ---
layout(binding = 1) uniform sampler2D inputTexture;  // RGBA image texture

// --- Fragment Output ---
layout(location = 0) out vec4 fragColor;  // Final pixel color

void main()
{
    // Sample color from texture at interpolated texture coordinate
    vec4 sampledColor = texture(inputTexture, vTexCoord);
    
    // Output the sampled color directly
    // Note: Ready for future modifications (brightness, contrast, effects, etc.)
    fragColor = sampledColor;
}