#version 150
out vec4 outputF;


uniform float time,timerKick;
uniform vec2 resolution;
uniform float vuData[128];
uniform vec3 randColor;
uniform float blendAlpha;
uniform float vuLow;


float sdHexPrism( vec2 p, vec2 h )
{
    vec2 q = abs(p);
    return max(q.x-h.y,max(q.x+q.y*0.57735,q.y*1.1547)-h.x);
}

vec3 nrand3( vec2 co )
{
	vec3 a = fract( cos( co.x*8.3e-3 + co.y )*vec3(1.3e5, 4.7e5, 2.9e5) );
	vec3 b = fract( sin( co.x*0.3e-3 + co.y )*vec3(8.1e5, 1.0e5, 0.1e5) );
	vec3 c = mix(a, b, 0.5);
	return c;
}

float map(vec3 p)
{
    float h = 1.0;
    float rh = 0.5;
    vec2 grid = vec2(1.2, 0.8);
    vec2 grid_half = grid*0.5;
    float radius = 0.35;
    vec3 orig = p;
	
    p.y = -abs(p.y);

    vec2 g1 = vec2(ceil(orig.xz/grid));
    vec2 g2 = vec2(ceil((orig.xz+grid_half)/grid));
    vec3 rxz =  nrand3(g1);
    vec3 ryz =  nrand3(g2);

	
    float d1 = p.y + h + rxz.x*rh;
    float d2 = p.y + h + ryz.y*rh;
	
	float v1 = vuData[int(mod(g1.x*9.0+g1.y*8.0,64.0))];
	float v2 = vuData[int(mod(g2.x*7.0+g2.y*6.0,64.0))];
	d1 = d1+0.9-v1*0.75;
	d2 = d2+0.9-v2*0.75;

    vec2 p1 = mod(p.xz, grid) - grid_half;
    float c1 = sdHexPrism(vec2(p1.x,p1.y), vec2(radius));
	    
    vec2 p2 = mod(p.xz+grid_half, grid) - vec2(grid_half);
    float c2 = sdHexPrism(vec2(p2.x,p2.y), vec2(radius));
	
    float dz = (grid.y*g1.y - p.z + 0.1)*0.5;
    float dz1 = -(abs(p.y)-h)+0.1;
	
    return min(min(max(c1,d1), max(c2,d2)), max(dz,dz1));
}



vec3 genNormal(vec3 p)
{
    const float d = 0.01;
    return normalize( vec3(
        map(p+vec3(  d,0.0,0.0))-map(p+vec3( -d,0.0,0.0)),
        map(p+vec3(0.0,  d,0.0))-map(p+vec3(0.0, -d,0.0)),
        map(p+vec3(0.0,0.0,  d))-map(p+vec3(0.0,0.0, -d)) ));
}

vec3 getCamPos(float t) {
	return vec3((cos(t) - sin(t / 4.0)) * 50.0, 0.0, (cos(t / 1.5) - sin(t * 2.0)) * 50.0);
}

void main()
{
    vec2 pos = (gl_FragCoord.xy*2.0 - resolution.xy) / resolution.y;
    vec3 camPos = getCamPos(time/15.0); //vec3(-0.5,0.0,3.0);
    //vec3 camDir = normalize(vec3(0.8, 0.0, -1.0));
	vec3 camDir = normalize(getCamPos(time/15.0+1.0)-camPos);
    camPos -=  vec3(time*0.2,0.0,time*1.0);
    vec3 camUp  = normalize(vec3(0.00, 1.0, 0.0));
    vec3 camSide = cross(camDir, camUp);
    float focus = 1.8;

    vec3 rayDir = normalize(camSide*pos.x + camUp*pos.y + camDir*focus);	    
    vec3 ray = camPos;
    int march = 0;
    float d = 0.0;

    float prev_d = 0.0;
    float total_d = 0.0;
    const int MAX_MARCH = 82;
    for(int mi=0; mi<MAX_MARCH; ++mi) {
        d = map(ray);
	march=mi;
        total_d += d;
        ray += rayDir * d;
        if(d<0.001) {break; }
	prev_d = d;
    }

    float glow = 0.0;
    
    float sn = 0.0;
    {
        const float s = 0.001;
        vec3 p = ray;
        vec3 n1 = genNormal(ray);
        vec3 n2 = genNormal(ray+vec3(s, 0.0, s));
        vec3 n3 = genNormal(ray+vec3(0.0, s, 0.0));
        glow = max(1.0-abs(dot(camDir, n1)-0.5), 0.0)*0.5;
        if(dot(n1, n2)<0.999 || dot(n1, n3)<0.999) {
            sn += 1.0;
        }
    }
    {
	vec3 p = ray;
        float grid1 = max(0.0, max((mod((p.x+p.y+p.z*2.0)-time*3.0, 5.0)-4.0)*1.5, 0.0) );
        float grid2 = max(0.0, max((mod((p.x+p.y*2.0+p.z)-time*2.0, 7.0)-6.0)*1.2, 0.0) );
        sn = sn*0.2 + sn*(grid1+grid2)*1.0;
    }
    glow += sn;

    float fog = min(1.0, (1.0 / float(MAX_MARCH)) * float(march))*1.0;
    vec3  fog2 = 0.005 * vec3(1, 1, 1) * total_d;
    glow *= min(1.0, 4.0-(4.0 / float(MAX_MARCH-1)) * float(march));
    //float scanline = mod(gl_FragCoord.y, 4.0) < 2.0 ? 0.7 : 1.0;
    outputF = vec4(randColor*1.5*vec3(0.15+glow*0.75, 0.15+glow*0.75, 0.15+glow*0.75) * fog + fog2, blendAlpha);// * scanline;
}

	