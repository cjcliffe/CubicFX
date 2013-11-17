#version 150
out vec4 outputF;

#define FlareScale 5.0
#define FlareMix 1.0
#define TimeScale 2.0
#define TunnelSegments 128.0
#define TunnelDist 128
#define TunnelSpeed 5.0
#define CosmosMix 1.0
#define CosmosRot 0.0
#define CosmosSpeed 2.0

/*
 * Base adapted from http://glsl.heroku.com/e#8463.1
 * Flare adapted from MrOMGWTF
 * Cosmos adapted from Kali's "Cosmos" shader: https://www.shadertoy.com/view/MssGD8
 */

uniform float time;
uniform float timerKick;
uniform vec2 resolution;
uniform float vuHigh;
uniform float vuLow;
uniform vec3 randColor;

vec4 colour(float c, float d)
{
	float f = d*255.0;
	c*=12.0;
	vec3 res = vec3(0.0,0.0,0.0);
	res += smoothstep(1.0,2.0,c) * vec3(0.0,3.0,310.0)/255.0;
	res += smoothstep(2.0,3.0,c) * vec3(f*0.5,f*1.5,f)/255.0;
	res += smoothstep(3.0,4.0,c) * vec3(0.0,-1.0,25.0)/255.0;
	res += smoothstep(4.0,5.0,c) * vec3(0.0,0.0,32.0)/255.0;
	res += smoothstep(5.0,6.0,c) * vec3(0.0,1.0,23.0)/255.0;
	res += smoothstep(6.0,7.0,c) * vec3(0.0,0.0,-30.0)/255.0;
	res += smoothstep(7.0,8.0,c) * vec3(0.0,0.0,-57.0)/255.0;
	res += smoothstep(8.0,9.0,c) * vec3(0.0,70.0,-15.0)/255.0;
	res += smoothstep(9.0,10.0,c) * vec3(0.0,100.0,50.0)/255.0;
	res += smoothstep(10.0,11.0,c) * vec3(0.0,71.0,58.0)/255.0;
	res += smoothstep(11.0,12.0,c) * vec3(0.0,10.0,64.0)/255.0;
	res += smoothstep(12.0,13.0,c) * vec3(0.0,1.0,33.0)/255.0;
	return vec4(res,1.0) * sqrt(d);
}

float periodic(float x,float period,float dutycycle)
{
	x/=period;
	x=abs(x-floor(x)-0.5)-dutycycle*0.5;
	return x*period;
}

float pcount(float x,float period)
{
	return floor(x/period);
}

float distfunc(vec3 pos, float t)
{
	vec3 gridpos=pos-floor(pos)-0.5;
	float r=length(pos.xy);
	float a=atan(pos.y,pos.x);
	a+=t*0.3*sin(pcount(r,3.0)+1.0)*sin(pcount(pos.z,1.0)*13.73);
	return min(max(max(
		periodic(r,3.0,0.2),
		periodic(pos.z,1.0,0.7+0.3*cos(t/3.0))),
		periodic(a*r,3.141592*2.0/6.0*r,0.7+0.3*(cos(t/3.0)-0.2))),0.25);
}

vec4 cosmos(float t) 
{
	float tim = (t+20.34) * 30.0;	
	float s = 0.0, v= 0.0;
	vec2 uv = (gl_FragCoord.xy / resolution.xy) * 2.0 - 1.0;
	
	float p = tim * 0.005 * CosmosRot;
	
	float si = sin(p);
	float co = cos(p);
	mat2 rot = mat2(co, si, -si, co);
	uv *= rot;
	
	for (int r = 0; r < 100; r++) 
	{
		vec3 p= vec3(0.3, 0.2, floor(tim) * 0.0008 * CosmosSpeed) + s * vec3(uv, 0.2);
		p.z = fract(p.z);
		for (int i=0; i < 11; i++)
		{
			p=abs(p)/dot(p,p) * 2.0 - 1.0;
		}
		v += length(p*p)*max(0.3 - s, 0.0) * .012;
		s += .003;
	}
	return v * vec4(v * 0.35, 0.7, s * 4.2, 1.0);	
}

vec3 flare(vec2 spos, vec2 fpos, vec3 clr, float e)
{
	vec3 color;
	float d = distance(spos, fpos);
	vec2 dd;
	dd.x = spos.x - fpos.x;
	dd.y = spos.y - fpos.y;
	dd = abs(dd);
	
	color = clr * max(0.0, (0.025 * e * e * FlareScale) / dd.y) * max(0.0, 1.5 - (dd.x - 1.0 * (e * 4.0 - 2.0)));
	color += clr * max(0.0, 0.5 / d);
	color += clr * max(0.0, 0.5 / distance(spos, -fpos)) * 0.15 ;
	color += clr * max(0.0, 0.13 - distance(spos, -fpos * 1.5)) * 1.5 ;
	color += clr * max(0.0, 0.07 - distance(spos, -fpos * 0.4)) * 2.0 ;
		
	return color;
}

float noise(vec2 pos)
{
	return fract(1111. * sin(111. * dot(pos, vec2(2222., 22.))));	
}

vec4 flareColor(float e) 
{
	vec2 position = ( gl_FragCoord.xy / resolution.xy * 2.0 ) - 1.0;
	position.x *= resolution.x / resolution.y;
	vec3 color = flare(position, vec2(0.0) , randColor, e);
	return vec4( color * (0.95 + noise(position*0.001 + 0.00001) * 0.05), 1.0 );
}


void main()
{
	float t = time * TimeScale;
	
	float d = 0.2+vuLow*0.7; //(sin((t - 5.0) / 3.0)*0.5+0.5);
	float mx = 0.5;
	float my = 0.5;
	
	vec2 coords=(2.0*gl_FragCoord.xy-resolution)/max(resolution.x,resolution.y);
	vec3 ray_dir=normalize(vec3(coords,1.0+0.0*sqrt(coords.x*coords.x+coords.y*coords.y)));
	vec3 ray_pos=vec3(32.0*pow(0.5-mx, 1.0),32.0*(0.5-my),t*TunnelSpeed);
	float i = TunnelSegments;
	for(int j=0;j<TunnelDist;j++)
	{
		float dist=distfunc(ray_pos,(timerKick*5.0));
		ray_pos+=dist*ray_dir;

		if(abs(dist)<0.001) { i=float(j); break; }
	}

	float c = i / TunnelSegments;
	vec3 distc = (colour(c, d) * 1.0).rgb;
	
	outputF = vec4(randColor*(distc.r+distc.g+distc.b),1.0);
	outputF += flareColor(d) * d * FlareMix;
//	outputF += cosmos(t) * d * d * CosmosMix;
	outputF *= d;
	outputF *= mod(gl_FragCoord.y, 2.0);
}
