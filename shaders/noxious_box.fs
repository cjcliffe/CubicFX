#version 150
out vec4 outputF;


uniform float time;
uniform vec2 resolution;
uniform float vuData[128];
uniform vec3 randColor;
uniform float blendAlpha;
uniform float vuLow;

vec3 planeVUMatrix(vec2 uv) {
	float upos = mod(uv.x,1.0)*8.0;
	float vpos = mod(uv.y,1.0)*8.0;
	int idx = int(upos)+int(vpos)*8;
    
	float color = vuData[abs(idx-64)%64]*2.0;
	
	color *= sin(3.14159*(upos-floor(upos)));
	color *= sin(3.14159*(vpos-floor(vpos)));
	
	return randColor*color;
}


// noxiousBox by eiffie 
// If the globe was just lights and mirrors we could do Global Illumination in one sample.
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

const vec3 emColor=vec3(1.35,1.3,1.25);
const int RaySteps=78, maxBounces=6;
const float HitDistance=0.002,lBloom=50.0,maxDepth=15.0,fov=2.5;
const vec2 ve=vec2(0.02,0.0);
vec3 sunDir;
int obj=0;
bool bColoring=false;
vec4 orbitTrap;
float mld;

struct material {vec3 color;float refrRefl,difExp,spec,specExp;};

vec3 litPos;
float ABSCL;
const float mr=0.25, mxr=1.0, SCALE = -2.25;
const vec4 scale=vec4(SCALE,SCALE,SCALE,2.25);
float DE(in vec3 z0){//amazing box by tglad (2 iterations)
	vec4 z = vec4(z0,1.0),p0=z;
	z.xyz=clamp(z.xyz, -1.0, 1.0) *2.0-z.xyz;
	z*=scale/clamp(dot(z.xyz,z.xyz),mr,mxr);
	z+=p0;
	z.xyz=clamp(z.xyz, -1.0, 1.0) *2.0-z.xyz;
	z*=scale/clamp(dot(z.xyz,z.xyz),mr,mxr);
	z+=p0;
	float dL=((length(z.xyz+litPos)-1.0)/z.w)-(vuLow*0.15);
	mld=min(mld,dL);
	float dS=min(dL,(length(z.xyz)-ABSCL)/z.w);
	if(bColoring){
		if(dS==dL)obj=1;
		else {obj=0;orbitTrap=z;}
	}
	return dS;
}

material getMaterial( in vec3 p, in vec3 nor )
{//return material properties
	if(obj==1)return material(vec3(1.0),0.01,2048.0,1.0,1024.0);
	return material(vec3(0.5)+sin(orbitTrap.rgb)*cos(length(orbitTrap.xyz))*0.5,0.01,pow(2.0,16.0),1.0,pow(2.,16.0));
}

vec3 getBackground( in vec3 rd ){
	return planeVUMatrix(rd.xy*2.0).rgb;
}

//the code below can be left as is so if you don't understand it that makes two of us :)

vec3 scene(vec3 ro, vec3 rd) 
{// find color and depth of scene
	vec3 fcol=vec3(1.333),tcol=vec3(0.0);//color frequency mask, emissive color
	float d,t=0.0;//dist to obj, total ray len
	int iHitCount=0;mld=1000.0;
	for(int i=0; i<RaySteps; i++ ){// march loop
		if(t>=maxDepth)continue;
		t+=d=DE(ro+t*rd);//march
		if(abs(d)<HitDistance*t){//hit
			orbitTrap=vec4(1000.0);
			bColoring=true;//turn on material mapping
			t+=d=DE(ro+t*rd);//move close to the hit point without fudging
			bColoring=false;
			//if(obj==1){tcol+=fcol*emColor;break;}
			tcol+=fcol*emColor/max(lBloom*mld,1.0);
			if(obj==0){
				ro+=rd*t;// advance ray position to hit point
				d=DE(ro);
				float d1=DE(ro-ve.xyy),d2=DE(ro+ve.xyy);
				float d3=DE(ro-ve.yxy),d4=DE(ro+ve.yxy);
				float d5=DE(ro-ve.yyx),d6=DE(ro+ve.yyx);
				vec3 nor=normalize(vec3(-d1+d2,-d3+d4,-d5+d6));
				material m=getMaterial( ro, nor );//and material
				rd=reflect(rd,nor);//setting up for a new ray direction and defaulting to a reflection
				fcol*=0.75*m.color;///min(1.0,float(i)*0.2);
				d=abs(d-0.5*(d2+d1))+abs(d-0.5*(d4+d3))+abs(d-0.5*(d6+d5));//edge finder
				fcol=max(vec3(0.0),fcol-vec3(d*60.0));
				t=max(d*5.0,HitDistance);//hopefully pushs away from the surface
				if(iHitCount++>maxBounces || dot(fcol,fcol)<0.01)t=maxDepth;
				mld=1000.0;
			}else{t=maxDepth;}
		}
	}
	tcol+=fcol*emColor/max(lBloom*mld,1.0);//this could run twice??
	return tcol+fcol*getBackground(rd);//light the scene with emissive and background
}	
mat3 lookat(vec3 fw,vec3 up){
	fw=normalize(fw);
	vec3 rt=normalize(cross(fw,normalize(up)));up=cross(rt,fw);
	return mat3(rt,up,fw);
}
void main() {
	float time=time*0.1;
	sunDir=normalize(vec3(0.7,1.0,-0.7));
	ABSCL=SCALE*SCALE*(0.8+sin(time*4.0)*0.2);
	litPos=vec3(0.0,0.0,0.0);
	vec3 ro=vec3(cos(time)*vec2(sin(time*6.4),cos(time*6.4)),sin(time))*(8.0+3.0*sin(time*3.0));
	vec3 dir=vec3((-resolution.xy+2.0*(gl_FragCoord.xy))/resolution.y,fov);
	vec3 rd=normalize(lookat(-ro,vec3(0.0,1.0+cos(time*25.0)*0.25,0.25*sin(time*25.0)))*dir);
	outputF = vec4(scene(ro,rd),blendAlpha);
}