#version 150
out vec4 outputF;

uniform float time;
uniform vec2 resolution;
uniform int beatCounterQuarter,beatCounter;
uniform float timerKick;
uniform vec3 randColor;

#define PI 3.14159265359
#define GEAR_PHASE 0.0958

vec2 cmul(vec2 v1, vec2 v2) {
	return vec2(v1.x * v2.x - v1.y * v2.y, v1.y * v2.x + v1.x * v2.y);
}

vec2 cdiv(vec2 v1, vec2 v2) {
	return vec2(v1.x * v2.x + v1.y * v2.y, v1.y * v2.x - v1.x * v2.y) / dot(v2, v2);
}

vec4 gear(vec2 uv, float dir, float phase) {
	vec2 p = uv - 0.5;
	
	float r = length(p);
	float t = fract(2.0*timerKick); //smoothstep(0.0, 1.0, fract(beatCounterQuarter/4.0));
	t *= 2.0 * PI / 6.0;
	float a = atan(p.y, p.x) + (phase + t) * dir;
	float e = 0.20 + clamp(sin(a * 6.0) * 0.13, 0.0, 0.1);
	
	if (r < e) {
		return vec4(randColor - r, 1.0);
	}
	else {
		return vec4(0.0);
	}
}

vec4 gears(vec2 uv) {
	vec4 c1 = gear(uv, 1.0, 0.0);
	vec4 c2 = gear(vec2(fract(uv.x + 0.5), uv.y), -1.0, GEAR_PHASE);
	vec4 c3 = gear(vec2(uv.x, fract(uv.y + 0.5)), -1.0, GEAR_PHASE);
	
	return c1 * c1.a + c2 * c2.a + c3 * c3.a;
}

void main(void)
{
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	
	uv -= 0.5;
	uv.x /= resolution.y / resolution.x;
	
	float t = sin((timerKick+time*0.86) * 0.2) * 6.0;
	float t2 = cos((timerKick+time*0.95) * 0.2) * 6.0;
	vec2 a = vec2(3.0, 0.0);
	vec2 b = vec2(0.0, 0.0);
	vec2 c = vec2(t2, t);
	vec2 d = vec2(0.0, 1.0);
	vec2 uv2 = cdiv(cmul(uv, a) + b, cmul(uv, c) + d);
	
	vec4 col = gears(fract(uv2));
	
	outputF = vec4(randColor,1.0) * col ; //+ vec4(abs(dot(uv, normalize(uv2))) * 0.25) * (1.0 - col.a);
}