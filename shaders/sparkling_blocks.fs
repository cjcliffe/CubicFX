#version 150
out vec4 outputF;


uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler1D samplerTex;
uniform float sampleRange;
uniform vec3 randColor;
uniform float blendAlpha;

uniform float vuData[128];

//Sparkling sinusoid by  curiouschettai

void main( void ) {
	float sinVal = resolution.y * texture(samplerTex,(gl_FragCoord.x/resolution.x)*0.25).r / (sampleRange*20.0);
	
	
	float funcArgShift = -10.0;//adjusts width of the white center of the glow.
	float funcVal = abs(gl_FragCoord.y-(resolution.y/2.0+sinVal)) - funcArgShift;
	float intensity = 500.0/(funcVal * funcVal);
	
	float scaline = mix(abs(sin(gl_FragCoord.y/2.0+time/6.0)), abs(sin(gl_FragCoord.x/2.0+time/6.0)), 0.5);
	
	vec3 color = (intensity/2.0)*randColor;

	
	float upos = (gl_FragCoord.y/resolution.y)*8.0;
	float vpos = (gl_FragCoord.x/resolution.x)*8.0;
	int vuxPos = int(floor(upos)*8.0+floor(vpos));	
	
	float vuxData = vuData[abs(32-vuxPos)];
	float posY = (gl_FragCoord.y/resolution.y);

	color += randColor*vuData[vuxPos]*sin(3.14159*(upos-floor(upos)))*sin(3.14159*(vpos-floor(vpos)));
		
	color /= (scaline);
	
	
	outputF = vec4(color, blendAlpha);
}