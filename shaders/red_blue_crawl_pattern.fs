#version 150
out vec4 outputF;

// modified by @hintz


uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float timerKick;
uniform int beatCounter;
uniform vec3 randColor;
uniform vec3 randColor2;

#define PI 3.14159
#define TWO_PI (PI*2.0)

void main(void) 
{
	float timer = (time+timerKick)*4.0;

	float N = 6.0 + float(int(floor(beatCounter/8)) % 8);
	
	vec2 center = (gl_FragCoord.xy);
	center.x=-200.2*sin(time/100.0);
	center.y=-200.2*cos(time/100.0);

	vec2 v = (gl_FragCoord.xy - resolution/20.0) / min(resolution.y,resolution.x) * 15.0;
	v.x=v.x-10.0+center.x;
	v.y=v.y-200.0+center.y;
	float col = 0.0;

	for(float i = 0.0; i < N; i++) 
	{
	  	float a = i * (TWO_PI/N) * 61.95;
		col += cos(TWO_PI*(v.y * cos(a) + v.x * sin(a) + timer*0.4 ));
	}
	
	col /= 3.0;

	outputF = vec4(randColor * clamp(vec3(col*1.0, col*1.0, col*1.0),0.0,1.0) + randColor2 * clamp(-vec3(col*1.0, col*1.0, col*1.0),0.0,1.0), 1.0);
}