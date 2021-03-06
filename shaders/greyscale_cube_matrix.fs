//MG - raymarching
//distance function(s) provided by
//http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
#version 150
out vec4 outputF;

uniform float time;
uniform vec2 resolution;

#define MIN		0.0
#define MAX		50.0
#define DELTA	0.01
#define ITER	1000

float sphere(vec3 p, float r) {
	p = mod(p,2.0)-0.5*2.0;
	return length(p)-r;
}

float sdBox( vec3 p, vec3 b )
{
	p = mod(p,2.0)-0.5*2.0;
	vec3 d = abs(p) - b;
	return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float castRay(vec3 o,vec3 d) {
	float delta = MAX;
	float t = MIN;
	for (int i = 0;i <= ITER;i += 1) {
		vec3 p = o+d*t;
		delta = sdBox(p,vec3(0.5,0.5,0.5));

		t += delta;
		if (t > MAX) {return MAX;}
		if (delta-DELTA <= 0.0) {return float(i);}
	}
	return MAX;
}

uniform float blendAlpha;
void main() {
	vec2 p=(gl_FragCoord.xy/resolution.y)*1.0;
	p.x-=resolution.x/resolution.y*0.5;p.y-=0.5;
	vec3 o = vec3(time,0,time);
	vec3 d = normalize(vec3(p.x,p.y,1.0));
	
	float t = castRay(o,d);
	vec3 rp = o+d*t;
	
	if (t < MAX) {
		t = 1.0-t/float(MAX);
		outputF = vec4(t,t,t,1.0);
	}
	else {
		outputF = vec4(0.0,0.0,0.0,1.0);
	}
	outputF.a = blendAlpha;
}