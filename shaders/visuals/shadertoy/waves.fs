#version 150
out vec4 outputF;


uniform float time,timerKick;
uniform vec2 resolution;
uniform vec3 randColor,randColor2;
uniform float blendAlpha;

uniform sampler2D samplerTex;
uniform float sampleRange;


// Based on this: https://www.shadertoy.com/view/4dsGzH
// By bonniem.

const float BLOCK_WIDTH = 0.03;

void main(void)
{
	vec2 uv = gl_FragCoord.xy / resolution.xy;

	vec3 COLOR1 = randColor * 0.4;
	vec3 COLOR2 = randColor2 * 0.4;

	// To create the BG pattern
	vec3 final_color = vec3(1.0);
	vec3 bg_color = vec3(0.0);
	vec3 wave_color = vec3(0.0);

	vec2 guv = gl_FragCoord.xy / max(resolution.x, resolution.y);
	float c1 = mod(guv.x, 2.0 * BLOCK_WIDTH);
	c1 = step(BLOCK_WIDTH, c1);

	float c2 = mod(guv.y, 2.0 * BLOCK_WIDTH);
	c2 = step(BLOCK_WIDTH, c2);

	bg_color = mix(uv.x * COLOR1, uv.y * COLOR2, c1 * c2);


	// To create the waves
	float snd = texture(samplerTex, vec2((gl_FragCoord.x / resolution.x) * 0.25, 0.5)).r;
	float soundOffset = clamp(snd / (max(sampleRange, 0.02) * 20.0), -0.3, 0.3);

	vec2 wuv = -1.0 + 2.0 * uv;
	wuv.x *= resolution.x / resolution.y;
	wuv.y += 0.1 + soundOffset;
	for(float i = 0.0; i < 10.0; i++)
	{
		wuv.y += 0.07 * sin(wuv.x + i / 7.0 + time);
		float wave_width = abs(1.0 / (150.0 * wuv.y));
		wave_color += vec3(wave_width * 1.9, wave_width, wave_width * 1.5);
	}

	final_color = bg_color + wave_color;


	outputF = vec4(final_color, blendAlpha);
}