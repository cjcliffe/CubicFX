#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

//Sparkling sinusoid by  curiouschettai

void main( void ) {
	float sinVal = 50.0*sin(gl_FragCoord.x/30.0+time);
	float funcArgShift = -10.0;//adjusts width of the white center of the glow.
	float funcVal = abs(gl_FragCoord.y-(resolution.y/2.0+sinVal)) - funcArgShift;
	float intensity = 500.0/(funcVal * funcVal);
	
	float scaline = mix(abs(sin(gl_FragCoord.y/2.0+time/6.0)), abs(sin(gl_FragCoord.x/2.0+time/6.0)), 0.5);
	
	
	vec3 color = vec3(intensity/8.0, intensity/8.0, intensity/2.0);
	color /= (scaline);
	
	gl_FragColor = vec4(color, 1.0);
}