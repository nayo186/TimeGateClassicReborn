$input v_texcoord0

// A camera's 2D animation (a part of the background that moves: water,
// fire...) over a player's picture (osystem_drawSpriteLayer): its pixels were
// drawn for the original 320x200 picture and, put over the player's one, made
// a flat patch of the old picture (Time Gate's fountain, BUGS.md G59). The
// player's picture stands for the animation's first frame: only what a frame
// changes from that one (the ripples) is added to it, blended between the
// 320x200 pixels so it is not blocky

#include "bgfx_shader.sh"
#include "palette.sh"

USAMPLER2D(s_spriteTexture, 0);      // r = colour, g = 1 where the animation draws
USAMPLER2D(s_originalBackground, 3); // camera picture as loaded (palette indexes)
SAMPLER2D(s_customBackground, 4);
USAMPLER2D(s_referenceTexture, 5);   // the animation's first frame, as s_spriteTexture

uniform vec4 u_customBackground;  // xy: U range of the 4:3 centre of the picture

// The change at one pixel of the 320x200 screen; w = 1 where the animation draws
vec4 changeAt(ivec2 p)
{
    p = clamp(p, ivec2(0, 0), ivec2(319, 199));
    uvec4 texel = texelFetch(s_spriteTexture, p, 0);
    if (texel.g == 0u)
        return vec4(0.0, 0.0, 0.0, 0.0);
    uvec4 first = texelFetch(s_referenceTexture, p, 0);
    uint base = (first.g != 0u) ? first.r : texelFetch(s_originalBackground, p, 0).r;
    vec3 change = getColorFromRawOffset(texel.r).rgb - getColorFromRawOffset(base).rgb;
    return vec4(change, 1.0);
}

void main()
{
    // Between the centres of the four nearest pixels
    vec2 position = v_texcoord0.xy * vec2(320.0, 200.0) - vec2(0.5, 0.5);
    ivec2 base = ivec2(floor(position));
    vec2 f = position - vec2(base);
    vec4 change = mix(mix(changeAt(base), changeAt(base + ivec2(1, 0)), f.x),
                      mix(changeAt(base + ivec2(0, 1)), changeAt(base + ivec2(1, 1)), f.x), f.y);
    if (change.w < 0.01)
        discard;

    vec2 pictureUV = vec2(mix(u_customBackground.x, u_customBackground.y, v_texcoord0.x), v_texcoord0.y);
    vec3 picture = texture2D(s_customBackground, pictureUV).rgb;
    // change.rgb already fades to nothing at the edge of the animation
    gl_FragColor = vec4(clamp(picture + change.rgb, 0.0, 1.0), 1.0);
}
