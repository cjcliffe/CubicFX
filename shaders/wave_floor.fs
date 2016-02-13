#version 150
out vec4 outputF;


uniform float time,timerKick;
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

//Scene Start

//Floor
vec2 obj0(in vec3 p){
  //obj deformation
	p.y =p.y+ sin(p.x/2.0+(time+timerKick)*0.9)+cos(p.z/2.5+(time+timerKick)*0.96)+0.5*sin((p.x+p.z)/5.0+(time+timerKick)*0.8);
	p.y=p.y+sin(sqrt(p.x*p.x+p.z*p.z)-(time+timerKick)*4.0)*0.75;
  //plane
  return vec2(p.y+3.0,0);
}
//Floor Color (checkerboard)
vec3 obj0_c(in vec3 p){
 /*if (fract(p.x*.5)>.5)
   if (fract(p.z*.5)>.5)
     return vec3(0,1,1);
   else
     return vec3(1,1,1);
 else
   if (fract(p.z*.5)>.5)
     return vec3(1,1,1);
   else
     	return vec3(0,1,1);*/
		return planeVUMatrix(p.xz)*2.0;
}

//Scene End

void main(void){
  vec2 vPos=-1.0+2.0*gl_FragCoord.xy/resolution.xy;

  //Camera animation
  vec3 vuv=vec3(0,2,sin(time*0.1));//Change camere up vector here
  vec3 prp=vec3(-sin(time*0.6)*8.0,0,cos(time*0.4)*8.0); //Change camera path position here
  vec3 vrp=vec3(0,-5,0); //Change camere view here


  //Camera setup
  vec3 vpn=normalize(vrp-prp);
  vec3 u=normalize(cross(vuv,vpn));
  vec3 v=cross(vpn,u);
  vec3 vcv=(prp+vpn);
  vec3 scrCoord=vcv+vPos.x*u*resolution.x/resolution.y+vPos.y*v;
  vec3 scp=normalize(scrCoord-prp);

  //Raymarching
  const vec3 e=vec3(0.1,0,0);
  const float maxd=80.0; //Max depth

  vec2 s=vec2(0.1,0.0);
  vec3 c,p,n;

  float f=1.0;
  for(int i=0;i<156;i++){
    if (abs(s.x)<.01||f>maxd) break;
    f+=s.x;
    p=prp+scp*f;
    s=obj0(p);
  }
  
  if (f<maxd){
    if (s.y==0.0)
      c=obj0_c(p);
    n=normalize(
      vec3(s.x-obj0(p-e.xyy).x,
           s.x-obj0(p-e.yxy).x,
           s.x-obj0(p-e.yyx).x));
    float b=dot(n,normalize(prp-p));
    outputF=vec4((b*c+pow(b,8.0))*(1.0-f*.02),blendAlpha);//simple phong LightPosition=CameraPosition
  }
  else outputF=vec4(randColor*(0.5*(resolution.y/gl_FragCoord.y)),blendAlpha); //background color
}
