// from jspride invitation ! ==> http://www.pouet.net/prod.php?which=61280

precision highp float;

uniform float time;
uniform vec2 resolution;

const float a=1e4;
const vec3 b=vec3(.7,.9,1);
const float c=.004;
const vec3 d=vec3(.5,1,.9);

float n(vec3 e){
	return length(max(abs(e)-vec3(3),0.));
}

float o(vec3 e)
{
	float f,g,h;
	f=sin(time*0.5)*.004;
	g=cos(e.z*f);
	h=sin(e.z*f);
	mat2 i=mat2(g,-h,h,g);
	e.xy=i*e.xy;

	return (n(mod(e-vec3(-5.,0,180.-time*1e2),50.)-vec3(25))-2.5);
}

vec3 p(vec3 e){
	float f=.01;
	vec3 g;
	g.x=o(vec3(e.x+f,e.y,e.z))-o(vec3(e.x-f,e.y,e.z));
	g.y=o(vec3(e.x,e.y+f,e.z))-o(vec3(e.x,e.y-f,e.z));
	g.z=o(vec3(e.x,e.y,e.z+f))-o(vec3(e.x,e.y,e.z-f));
	
	return normalize(g);
}

float q(vec3 e,vec3 f,float g)
{
	float h=g;
	e+=f*g;
	for(int i=0;i<1;i++){
		e+=f;
		h=min(h,max(o(e),0.));
	}
	return h/g;
}

vec3 r(vec3 e,vec3 f){
	float ep=1.;
	for(int g=0;g<100;g++){
		float h=o(e);
		e+=ep*h*f;
		ep*=1.00005;
	}
	return e;
}

void main(){
	vec3 e,f,g,h,i,j;
	e=vec3(cos(time*.7)*5.,sin(time*.4)*320.+12.,0);
	f=normalize(vec3((gl_FragCoord.x-resolution.x*.5)/resolution.y,gl_FragCoord.y/resolution.y-.15,1));
	f.x*=0.73;
	h=r(e,f);
	i=p(h);
	j=vec3(2);
	float k,l,m;
	k=1.-2.5/exp(h.z*c);
	l=q(h,i,05.);
	m=max(dot(i,vec3(5.0, 6.707,0)),0.);
	g=mix(j*m*l,vec3(.8),k)*vec3(0.00159*gl_FragCoord.x,0.05,1);
	g=(g-normalize(g)*0.1)*1.;
	gl_FragColor=vec4(g,1);
}