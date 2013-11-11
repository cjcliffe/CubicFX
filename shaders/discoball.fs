#version 150
out vec4 outputF;


uniform vec2 resolution;
uniform float time;
uniform float vuData[128];
uniform vec3 randColor, randColor2;
uniform float timerKick;

vec4 s(vec2 px,float z)
{
    float l=3.1415;
    float k=5.0*time*sign(z);
    float x = px.x*320.0*.0065*z;
    float y = px.y*240.0*.0060*z;
    float c=sqrt(x*x+y*y);
    if(c>1.0)
    {
        return vec4(0.0);
    }
    else
    {
        float u=-.2*sign(z)+0.5*sin(k*.05);
        float v=sqrt(1.0-x*x-y*y);
        float q=y*sin(u)-v*cos(u);
        y=y*cos(u)+v*sin(u);
        v=acos(y);
        u=acos(x/sin(v))/(2.0*l)*120.0*sign(q)-k;
        v=v*60.0/l;
        q=cos(floor(v/l));
        c = vuData[int(mod(floor(u/5.0)*5.0+floor(v/5.0),64.0))]*2.0;//+mod(v/10.0,1.0);//pow(abs(cos(u)*sin(v)),.2)*.1/(q+sin(float(int((u+l/2.0)/l))+k*.6+cos(q*25.0)))*pow(1.0-c,.9);
		if (c>1.0) c = -c+1.0;
		
        vec4 res;
        if(c<0.0)
           res = vec4(-c * randColor2,1.0); //vec4(-c/2.0,0.0,-c*2.0,1.0);
        else
           res = vec4(c * randColor,1.0); //vec4(c,c*2.0,c*2.0,1.0);
        return res;
    }
}


void main(void)
{
    vec2 p = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
    vec4 c = vec4(0.0);
    for(int i=80;i>0;i--)
        c+=s(p,1.0-float(i)/80.0)*(.008-float(i)*.00005);
    vec4 d=s(p,1.0);
    outputF = (d.a==0.0?s(p,-.2)*.02:d)+sqrt(c);
}

