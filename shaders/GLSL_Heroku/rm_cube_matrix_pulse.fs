
#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

float sr = sin(radians(45.0));
float cr = cos(radians(45.0));
mat3 rotz= mat3(
            cr, sr, 0,
            sr,-cr, 0,
             0,  0, 1 );
mat3 roty = mat3(
          cr, 0, sr,
           0, 1,  0,
         -sr, 0, cr );

float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdBox( vec2 p, vec2 b )
{
  vec2 d = abs(p) - b;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}


float sdCross( in vec3 p )
{
    float da = sdBox(p.xy,vec2(1.0));
    float db = sdBox(p.yz,vec2(1.0));
    float dc = sdBox(p.zx,vec2(1.0));
    return min(da,min(db,dc));
}



float map(vec3 p)
{
    float d3 = p.z - 0.3;
    p.z += 0.7; //Fix for rep box. // thanks!!
    p = mod(p, vec3(3.0)) - vec3(1.5);
    p = rotz * p;
    p = roty * p;
    float d = sdBox(p,vec3(1.0));

    float s = 1.0;
    for( int m=0; m<3; m++ )
    {
       vec3 a = mod( p*s, 2.0 )-1.0;
       s *= 3.0;
       vec3 r = 3.0*abs(a);

       float c = sdCross(r)/s;
       d = max(d,-c);
    }

    return max(d, d3);
}

vec3 genNormal(vec3 p)
{
    const float d = 0.01;
    return normalize( vec3(
        map(p+vec3(  d,0.0,0.0))-map(p+vec3( -d,0.0,0.0)),
        map(p+vec3(0.0,  d,0.0))-map(p+vec3(0.0, -d,0.0)),
        map(p+vec3(0.0,0.0,  d))-map(p+vec3(0.0,0.0, -d)) ));
}

void main()
{
    vec2 pos = (gl_FragCoord.xy*2.0 - resolution.xy) / resolution.y;
    vec3 camPos = vec3(0.0,0.0,3.0);
    camPos.x +=  -time*0.4;
    camPos.y +=  -time*0.1;
    vec3 camDir = vec3(0.0,0.0,-1.0);
    vec3 camUp  = vec3(0.0, 1.0, 0.0);
    vec3 camSide = cross(camDir, camUp);
    float focus = 1.8;

    float px = pos.x+cos(time*0.2);
    float py = pos.y+sin(time*0.1);
    float floater = time*0.2;
    float sx = sin(floater);
    float cx = cos(floater);
    float ux = px*sx+py*cx;
    float uy = px*cx-py*sx;
	
	
    vec3 rayDir = normalize(camSide*ux + camUp*uy + camDir*focus);

    vec3 ray = camPos;
    int i = 0;
    float d = 0.0, total_d = 0.0;
    const int MAX_MARCH = 32;
    const float MAX_DISTANCE = 750.0;
    for(int mi=0; mi<MAX_MARCH; ++mi) {
	    ++i;
        d = map(ray);
        total_d += d;
        ray += rayDir * d;
        if(d<0.001) { break; }
        if(total_d>MAX_DISTANCE) {
            total_d = MAX_DISTANCE;
            i = MAX_MARCH;
            ray = camPos + rayDir*MAX_DISTANCE;
            break;
        }
    }


    float m = 1.0 / float(MAX_MARCH);
    float glow = 0.0;
    {
        const float s = 0.01;
        vec3 p = ray;
        if(total_d>MAX_DISTANCE) {
        }
        else {
            vec3 n1 = genNormal(ray);
            vec3 n2 = genNormal(ray+vec3(s, 0.0, 0.0));
            vec3 n3 = genNormal(ray+vec3(0.0, s, 0.0));
            if(dot(n1, n2)<0.9 || dot(n1, n3)<0.9) {
                glow = 0.3;
            }
        }
    }
    {
        vec3 p = rotz * ray;
        p = roty * p;
        float grid1 = max(glow, max((mod((p.x+p.y+p.z*2.0)-time*2.5, 5.0)-4.0)*1.5, 0.0) );
        float grid2 = max(glow, max((mod((p.x+p.y*2.0+p.z)-time*2.0, 7.0)-6.0)*1.2, 0.0) );
        vec3 gp1 = abs(mod(p, vec3(0.29)));
        vec3 gp2 = abs(mod(p, vec3(0.36)));
        if(gp1.x<0.28 && gp1.y<0.28) {
            grid1 = 0.0;
        }
        if(gp2.x<0.345 && gp2.y<0.345) {
            grid2 = 0.0;
        }
        glow += grid1+grid2;
    }

    float fog  = m*float(i);
    vec3  col  = vec3(0.2+glow*0.75, 0.2+glow*0.75, 0.3+glow);
    //col        = sqrt(col) + rayDir * 0.01;
    vec3  fog2 = 0.02 * vec3(1, 2, 3) * total_d;
    gl_FragColor = vec4(col * fog + fog2, 1.0);
}

	