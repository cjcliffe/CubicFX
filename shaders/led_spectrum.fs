#version 150
out vec4 outputF;

uniform vec2 resolution;
uniform float time;
uniform float vuData[128];
uniform vec3 randColor;
uniform float blendAlpha;
uniform sampler1D samplerTex;
uniform float sampleRange;
uniform float vuLow, vuMid, vuHigh;

const float PI = 3.14159;

// http://github.prideout.net/barrel-distortion/
vec2 distort(vec2 p, float power)
{
    float a  = atan(p.y, p.x);
    float r = length(p);
    r = pow(r, power);
    return vec2(r * cos(a), r*sin(a));
	//return vec2((a / PI), r*2.0-1.0);	// polar
}

void main(void)
{
    // create pixel coordinates
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	
	// distort
	float bass = vuLow; // texture2D( iChannel0, vec2(0, 0) ).x;		
	uv = distort(uv*2.0-1.0, 1.0+bass*0.5)*0.5+0.5;
	uv.x += 0.05;
		
	// quantize coordinates
	const float bands = 20.0;
	const float segs = 20.0;
	vec2 p;
	p.x = floor(uv.x*bands)/bands;
	p.y = floor(uv.y*segs)/segs;
	
	// read frequency data from first row of texture
	float fft  = vuData[int(64.0*p.x)]; //texture2D( iChannel0, vec2(p.x,0.0) ).x;	

	// led color
	vec3 color = mix(vec3(0.0, 2.0, 0.0), vec3(2.0, 0.0, 0.0), sqrt(uv.y));
	
	// mask for bar graph
	float mask = (p.y < fft) ? 1.0 : 0.0;
	
	// led shape
	vec2 d = fract((uv - p)*vec2(bands, segs)) - 0.5;
	float led = smoothstep(0.5, 0.3, abs(d.x)) *
		        smoothstep(0.5, 0.3, abs(d.y));
	vec3 ledColor = led*color*mask;

    // second texture row is the sound wave
	float wave = (texture(samplerTex,uv.x*0.25).r / (sampleRange*20.0))  +0.5;   //texture2D( iChannel0, vec2(uv.x, 0.75) ).x;
	vec3 waveColor = randColor*0.015 / abs(wave - uv.y);
		
	// output final color
	//gl_FragColor = vec4(vec3(fft),1.0);
    //gl_FragColor = vec4(d, 0.0, 1.0);
	//gl_FragColor = vec4(ledColor, 1.0);
	//gl_FragColor = vec4(waveColor, 1.0);
	outputF = vec4(ledColor + waveColor, blendAlpha);
}
