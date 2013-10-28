#version 150
out vec4 outputF;


uniform float time;
uniform vec2 resolution;
uniform float timerKick;
uniform vec3 randColor;

float map(vec3 p)
{
    const int MAX_ITER = 10;
    const float BAILOUT=4.0;
    float Power=8.0;

    vec3 v = p;
    vec3 c = v;

    float r=0.0;
    float d=1.0;
    for(int n=0; n<=MAX_ITER; ++n)
    {
        r = length(v);
        if(r>BAILOUT) break;

        float theta = acos(v.z/r);
        float phi = atan(v.y*(0.3+abs(cos(time*0.9))), v.x*(0.3+abs(sin(time))));
        d = pow(r,Power-1.0)*Power*d+1.0;

        float zr = pow(r,Power);
        theta = theta*Power+(time*5.0+timerKick*4.0);
        phi = phi*Power+(time*5.0+timerKick*6.0);
        v = (vec3(sin(theta)*cos(phi), sin(phi)*sin(theta), cos(theta))*zr)+c;
    }
    return 0.5*log(r)*r/d;
}


void main( void )
{
    vec2 pos = (gl_FragCoord.xy*2.1 - resolution.xy) / resolution.y;
    vec3 camPos = vec3(2.0*cos((time+timerKick)*0.45), 2.0*sin((time+timerKick)*0.34), -2.0*sin((time)*0.23));
    vec3 camTarget = vec3(0.0, 0.0, 0.0);

    vec3 camDir = normalize(camTarget-camPos);
    vec3 camUp  = normalize(vec3(0.0, 1.0, 0.0));
    vec3 camSide = cross(camDir, camUp);
    float focus = 1.8;

    vec3 rayDir = normalize(camSide*pos.x + camUp*pos.y + camDir*focus);
    vec3 ray = camPos;
    float m = 0.0;
    float d = 0.0, total_d = 0.0;
    const int MAX_MARCH = 50;
    const float MAX_DISTANCE = 1000.0;
    for(int i=0; i<MAX_MARCH; ++i) {
        d = map(ray);
        total_d += d;
        ray += rayDir * d;
        m += 1.0;
        if(d<0.001) { break; }
        if(total_d>MAX_DISTANCE) { total_d=MAX_DISTANCE; break; }
    }

    float c = (total_d)*0.0001;
    vec4 result = vec4( randColor*(1.0-vec3(c, c, c) - vec3(0.025, 0.025, 0.025)*m*0.8), 1.0 );
    outputF = result;
}
