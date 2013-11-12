#version 150
out vec4 outputF;


uniform vec2 resolution;
uniform float time;
uniform float vuData[128];
uniform vec3 randColor;

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

    float an = time*.25;

    float x = p.x*cos(an)-p.y*sin(an);
    float y = p.x*sin(an)+p.y*cos(an);
     
    uv.x = 1.25*x/abs(y);
    uv.y = 1.20*time + 1.25/abs(y);

    outputF = vec4(planeVUMatrix(uv) * y * y * 8.0, 1.0);
}
