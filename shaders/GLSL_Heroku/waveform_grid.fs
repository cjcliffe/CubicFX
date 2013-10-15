#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
varying vec2 surfacePosition;

// shabby - visualising various OSC with filter simulation

#define PI 3.1415926765
vec2 gPixSize;
vec2 gPos;

vec3 Plot(float f1,float f2,float width,vec3 col)
{
vec2 p1=vec2(gPos.x,f1);
vec2 p2=vec2(gPos.x+gPixSize.x,f2);
float res=distance(gPos,p2);
res=smoothstep(0.0,distance(p1,p2)*width,res);
return col*(1.0-res);
}

float sinc(float x)  {	return sin(x)/(x);}
float sincn(float x) {	return sin(PI*x)/(PI*x);}


float PDOsc(float x)
{
x*=0.25;	
x=mod(x,1.);
float CutOff=abs(cos(time)*2.);
float res=cos(2.*PI*(min(x*pow(CutOff,03.75),(1.0-x)*pow(CutOff,.8)*.275)+x));
// Morph from 0.00001=Square 1=SawTooth 2=Super Sine 3-7=Crazy peak shape 10+ = Pulse
float type=(cos(time*0.5)); 
res=pow(res,2.+type)/res;
return res;
}

void main( void ) 

{

	gPos = ( gl_FragCoord.xy / resolution.xy)-0.5;
	gPos=surfacePosition;
	gPixSize=2./resolution.xy;
	gPos=(gPos*vec2(14.0,10));
	vec3 col = vec3(0.0);
	
	float f1=PDOsc(gPos.x);
	float f2=PDOsc(gPos.x+gPixSize.x);

	col+=Plot(f1,f2,8.,vec3(1.0,0.7,0.0));
	
	// shabby grid affair
	float xx=mod(gPos.x,0.5)-0.25;
	col+=smoothstep(0.5,0.7,(xx*xx*10.)*vec3(0.9,.9,0.92));
	
	
	xx=mod(gPos.y,0.5)-0.25;
	col+=smoothstep(0.5,0.7,(xx*xx*10.)*vec3(0.9,0.9,0.9));
	xx=0.005/(0.5-gPos.y);
	col+=smoothstep(0.5,0.7,(xx*xx*1.)*vec3(0.9,0.9,0.9));
	xx=0.005/(0.5-gPos.x);
	col+=smoothstep(0.5,0.7,(xx*xx*1.)*vec3(0.9,0.9,0.9));
	
	//col+=(mod(.5-gPos.x,0.25*1.4))*vec3(1,1,1);
	//col+=(mod(.5-gPos.y,0.25))*vec3(1,1,1);
	
	
	gl_FragColor = vec4( col, 1.0 );

}