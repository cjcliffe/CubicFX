#version 150
out vec4 outputF;


uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float sampleData[256+50];
uniform float sampleRange;
uniform vec3 randColor;

uniform float vuData[128];

//Sparkling sinusoid by  curiouschettai

void main( void ) {
	float sinVal = resolution.y * sampleData[50+int((gl_FragCoord.x/resolution.x)*256)] / (sampleRange*20.0);
	
	
	float funcArgShift = -10.0;//adjusts width of the white center of the glow.
	float funcVal = abs(gl_FragCoord.y-(resolution.y/2.0+sinVal)) - funcArgShift;
	float intensity = 500.0/(funcVal * funcVal);
	
	float scaline = mix(abs(sin(gl_FragCoord.y/2.0+time/6.0)), abs(sin(gl_FragCoord.x/2.0+time/6.0)), 0.5);
	
	vec3 color = (intensity/2.0)*randColor;

	int vuxPos = int(floor((gl_FragCoord.y/resolution.y)*8.0)*8.0+floor((gl_FragCoord.x/resolution.x)*8.0));	
	float vuxData = vuData[abs(32-vuxPos)];
	float posY = (gl_FragCoord.y/resolution.y);
	//if (posY<=vuxData || (1.0-posY)<=vuxData) {
		color += randColor*vuData[vuxPos];
	//}
		
	color /= (scaline);
	
	
	outputF = vec4(color, 1.0);
}