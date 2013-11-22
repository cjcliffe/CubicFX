
#version 150
out vec4 outputF;


uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float vuLow, vuMid, vuHigh;
uniform float timerKick;
uniform vec3 randColor;

// a raymarching experiment by kabuto
//fork by tigrou ind (2013.01.22)


const int MAXITER = 42;

vec3 field(vec3 p) {
	p *= .1;
	float f = .1;
	for (int i = 0; i < 4; i++) {
		p = sin((time+timerKick)/70.0)+p.yzx*mat3(.8,.6,0,-.6,.8,0,0,0,1);
		p += vec3(.123,.456,.789)*float(0.1);
		p = abs(fract(p)-.5);
		p *= 2.0;
		f *= 2.0;
	}
	p *= p;
	return sqrt(p+p.yzx)/f-.035;
}


uniform float blendAlpha;
void main( void ) {
	//float jit = 0.01;
	//if (mod(time, 1.1) < 0.5) jit = 0.001;
	float jit = 0.01*(vuLow+vuHigh);
	vec3 dir = normalize(vec3((gl_FragCoord.xy-resolution*.5)/resolution.x,1.));
	float a = sin(time)*0.03;
	float mouseOffsetX = mouse.x  / 4.0 ;
	vec3 pos = vec3(50.0*sin(time/50.0),50.0*cos(time/60.0),(time+timerKick*6.0) );
	dir *= mat3(1,0,0,0,cos(a),-sin(a),0,sin(a),cos(a));
	dir *= mat3(cos(a),0,-sin(a)*6.0,0,1,0,sin(a),0,cos(a));
	vec3 color = vec3(0);
	for (int i = 0; i < MAXITER; i++) {
		vec3 f2 = field(pos);
		float f = min(min(f2.x,f2.y),f2.z);
		
		pos += dir*f;
		color += float(MAXITER-i)/(f2+jit);
	}
	vec3 color3 = vec3(1.-1./(1.+color*(.09/float(MAXITER*MAXITER))));
	color3 *= color3;
	outputF = vec4(randColor*vec3(color3.r+color3.g+color3.b),1.);
	outputF.a = blendAlpha;
}
