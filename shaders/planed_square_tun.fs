#version 150
out vec4 outputF;


uniform float time;
uniform vec2 resolution;

uniform float vuData[128];
uniform vec3 randColor;
uniform float timerKick;

vec3 planeVUMatrix(vec2 uv) {
	float upos = mod(uv.x,1.0)*8.0;
	float vpos = mod(uv.y,1.0)*8.0;
	int idx = int(upos)+int(vpos)*8;
    
	float color = vuData[abs(idx-64)%64]*2.0;
	
	color *= sin(3.14159*(upos-floor(upos)));
	color *= sin(3.14159*(vpos-floor(vpos)));
	
	return randColor*color;
}

void main(void)
{
    vec2 p = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
    vec2 uv;

    float r = pow( pow(p.x*p.x,16.0) + pow(p.y*p.y,16.0), 1.0/32.0 );
    uv.x = .5*time + 0.5/r;
    uv.y = 1.0*atan(p.y,p.x)/3.1416;

    vec3 col =  planeVUMatrix(uv*6.0);

    outputF = vec4(col*r*r*8.0,1.0);
}
    