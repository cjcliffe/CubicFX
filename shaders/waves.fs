#version 150
out vec4 outputF;


uniform float time,timerKick;
uniform vec2 resolution;
uniform float vuData[128];
uniform vec3 randColor,randColor2;
uniform float blendAlpha;
uniform float vuLow;

uniform sampler1D samplerTex;
uniform float sampleRange;

vec3 planeVUMatrix(vec2 uv) {
	float upos = mod(uv.x,1.0)*8.0;
	float vpos = mod(uv.y,1.0)*8.0;
	int idx = int(upos)+int(vpos)*8;
    
	float color = vuData[abs(idx-64)%64]*2.0;
	
	color *= sin(3.14159*(upos-floor(upos)));
	color *= sin(3.14159*(vpos-floor(vpos)));
	
	return randColor*color;
}


// Based on this: https://www.shadertoy.com/view/4dsGzH
// By bonniem.

vec3 COLOR1 = randColor; // vec3(0.0, 0.0, 0.3);
vec3 COLOR2 = randColor2; //vec3(0.5, 0.0, 0.0);
float BLOCK_WIDTH = 0.01;

void main(void)
{
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	
	// To create the BG pattern
	vec3 final_color = vec3(1.0);
	vec3 bg_color = vec3(0.0);
	vec3 wave_color = vec3(0.0);
	
	float c1 = mod(uv.x, 2.0 * BLOCK_WIDTH);
	c1 = step(BLOCK_WIDTH, c1);
	
	float c2 = mod(uv.y, 2.0 * BLOCK_WIDTH);
	c2 = step(BLOCK_WIDTH, c2);
	
	bg_color = mix(uv.x * COLOR1, uv.y * COLOR2, c1 * c2);
	
	
	// To create the waves
	float wave_width = 0.01;
	uv  = -1.0 + 2.0 * uv;
	uv.y += 0.1;
	for(float i = 0.0; i < 10.0; i++) 
	{
		float soundOffset = texture(samplerTex,(gl_FragCoord.x/resolution.x)*0.25).r / (sampleRange*120.0); //texture2D(iChannel0, vec2(0.5 + uv.x / 20.0, i / 7.0)).x; 
		uv.y += 0.07 * pow(sin(uv.x + i/7.0 + time),2.0) + soundOffset;//(0.07 * sin(uv.x + i/7.0 + time ));
		wave_width = abs(1.0 / (150.0 * uv.y));
		wave_color += vec3(wave_width * 1.9, wave_width, wave_width * 1.5);
	}
	
	final_color = bg_color + wave_color;
	
	
	outputF = vec4(final_color, blendAlpha);
}