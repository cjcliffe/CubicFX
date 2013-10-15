#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

// GLSL Shader tutorial by @hintz at OpenTechSchool Berlin 

void main( void ) 
{

	vec2 position = ( (gl_FragCoord.xy - mouse.xy*resolution)/ (resolution.xx + 0.5*resolution.xy));
	vec2 position2 = ( (gl_FragCoord.xy - 0.5*resolution)/ (resolution.xx + 0.5*resolution.xy)) ;
	
	float b = sin(100.0*sqrt(position.x*position.x+position.y*position.y));
	 b += sin(100.0*sqrt(position2.x*position2.x+position2.y*position2.y));
	float color = 0.0;
	gl_FragColor = gl_FragColor + vec4( vec3(b), 2.0 );
	
	
}