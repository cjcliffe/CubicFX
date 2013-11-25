#version 150
out vec4 outputF;


uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float vuData[128];
uniform vec3 randColor;
uniform float blendAlpha;



//parameters
const int iterations=20;
const float scale=1.3;
const vec2 fold=vec2(.5);
const vec2 translate=vec2(1.5);
const float zoom=.25;
const float brightness=7.;
const float saturation=.65;
const float texturescale=4.0;
const float rotspeed=.1;
const float colspeed=.05;
const float antialias=2.;

vec3 planeVUMatrix(vec2 uv) {
	float upos = mod(uv.x,1.0)*8.0;
	float vpos = mod(uv.y,1.0)*8.0;
	int idx = int(upos)+int(vpos)*8;
    
	float color = vuData[abs(idx-64)%64]*2.0;
	
	color *= sin(3.14159*(upos-floor(upos)));
	color *= sin(3.14159*(vpos-floor(vpos)));
	
	return randColor*color;
}

vec2 rotate(vec2 p, float angle) {
return vec2(p.x*cos(angle)-p.y*sin(angle),
		   p.y*cos(angle)+p.x*sin(angle));
}

void main(void)
{
	vec3 aacolor=vec3(0.);
	vec2 pos=gl_FragCoord.xy / resolution.xy-.5;
	float aspect=resolution.y/resolution.x;
	pos.y*=aspect;
	pos/=zoom; 
	vec2 pixsize=max(1./zoom,100.-time*50.)/resolution.xy;
	pixsize.y*=aspect;
	for (float aa=0.; aa<25.; aa++) {
		if (aa+1.>antialias*antialias) break;
		vec2 aacoord=floor(vec2(aa/antialias,mod(aa,antialias)));
		vec2 p=pos+aacoord*pixsize/antialias;
		p+=fold;
		float expsmooth=0.;
		vec2 average=vec2(0.);
		float l=length(p);
		for (int i=0; i<iterations; i++) {
			p=abs(p-fold)+fold;
			p=p*scale-translate;
			if (length(p)>20.) break;
			p=rotate(p,time*rotspeed);
			average+=p;
		}
		average/=float(iterations);
		vec2 coord=average+vec2(time*colspeed);
		vec3 color=planeVUMatrix(coord*texturescale).rgb*1.5; // texture2D(iChannel0,coord*texturescale).xyz;
		color*=min(1.1,length(average)*brightness);
		color=mix(vec3(length(color)),color,saturation);
		aacolor+=color;
	}
	outputF = vec4(aacolor/(antialias*antialias),blendAlpha);
}