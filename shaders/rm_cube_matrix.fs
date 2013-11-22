#version 150
out vec4 outputF;

uniform vec2 resolution;
uniform float time;

vec3 trans(vec3 p)
{
  return mod(p, 2.0)-1.0;
}
 
float lengthN(vec3 v, float n)
{
  vec3 tmp = pow(abs(v), vec3(n));
  return pow(tmp.x+tmp.y+tmp.z, 1.0/n);
}
 
float distanceFunction(vec3 pos)
{
  return lengthN(trans(pos), 8.0) - 0.3;
}
vec3 getNormal(vec3 p)
{
  const float d = 0.0001;
  return
    normalize
    (
      vec3
      (
        distanceFunction(p+vec3(d,0.0,0.0))-distanceFunction(p+vec3(-d,0.0,0.0)),
        distanceFunction(p+vec3(0.0,d,0.0))-distanceFunction(p+vec3(0.0,-d,0.0)),
        distanceFunction(p+vec3(0.0,0.0,d))-distanceFunction(p+vec3(0.0,0.0,-d))
      )
    );
}
 
uniform float blendAlpha;

void main() {
	vec2 pos = (gl_FragCoord.xy*2.0 -resolution) / resolution.y;
	
	vec3 camPos = vec3(-0.5,0.0,0.0);
	vec3 camDir = normalize(vec3(0.3*cos(time*0.1), 0.0, -1.0));
	camPos -=  vec3(0.0,0.0,time*3.0);
	vec3 camUp  = normalize(vec3(0.5, 1.0, 0.6));
	vec3 camSide = cross(camDir, camUp);
	float focus = 1.8;
	
	vec3 rayDir = normalize(camSide*pos.x + camUp*pos.y + camDir*focus);
	
	float t = 0.0, d;
	vec3 posOnRay = camPos;
	
	for(int i=0; i<54; ++i){
		d = distanceFunction(posOnRay);
		t += d;
		posOnRay = camPos + t*rayDir;
	}
	vec3 normal = getNormal(posOnRay);
	
	
	if(abs(d) < 0.001)
	{
		vec3 color = vec3(normal.x*0.3+normal.y*0.2+normal.z*0.2,
				  normal.x*0.2+normal.y*0.3+normal.z*0.2,
				  normal.x*0.2+normal.y*0.2+normal.z*0.3);
		outputF = vec4(vec3(color),1.0);
	}else{
		outputF = vec4(0.0);
	}
	outputF.a = blendAlpha;
}