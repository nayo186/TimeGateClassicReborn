$input v_texcoord0

#include "bgfx_shader.sh"
#include "palette.sh"

// Time Gate's textures: palette indexes, 256 texels wide, 0 transparent;
// without a texture (1 x 1 of 0) the face has the colour of v_texcoord0.w.
// The shade level of each point (v_texcoord0.z, 0-63) picks the row of the
// shade table (64 x 256) that gives the colour shown (row 0: as it is).
USAMPLER2D(s_texels, 0);
USAMPLER2D(s_shades, 2);

void main()
{
    ivec2 size = textureSize(s_texels, 0);
    uint index;
    if (size.x > 1)
    {
        ivec2 texel = ivec2(int(floor(v_texcoord0.x)) & 255, clamp(int(floor(v_texcoord0.y)), 0, size.y - 1));
        index = texelFetch(s_texels, texel, 0).r;
        if (index == 0u)
            discard;
    }
    else
    {
        index = uint(v_texcoord0.w + 0.5);
    }
    if (textureSize(s_shades, 0).y >= 64)
    {
        int level = clamp(int(floor(v_texcoord0.z)), 0, 63);
        index = texelFetch(s_shades, ivec2(int(index), level), 0).r;
    }
    gl_FragColor = getColorFromRawOffset(index);
}
