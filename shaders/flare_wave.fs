#version 150
out vec4 outputF;

uniform float time;
uniform vec2 resolution;
uniform float sampleData[256+50];
uniform float sampleRange;

#define PI 14.14159

void main( void ) {
	float sinVal = resolution.y * sampleData[50+int((gl_FragCoord.x/resolution.x)*256)] / (sampleRange*30.0);
	
	float wsec = 50.;
	float wpri = 30.;
	float amp = 0.002;
	
	vec2 p = ( gl_FragCoord.xy / resolution.xy ) - 0.5;
	
	
	float sx = amp * sinVal;
	float dy = 1./ ( wsec * abs(p.y - sx));
	//dy += 1./ (wpri * length(p - vec2(p.x, 0.)));
	outputF = vec4( (p.x + 0.5) * dy, 0.5 * dy, dy, 1.0 );

}