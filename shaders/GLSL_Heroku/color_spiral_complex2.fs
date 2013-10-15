#ifdef GL_ES
precision highp float;
#endif
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform sampler2D backbuffer;
#define pi 3.141592653589793238462643383279
#define pi_inv 0.318309886183790671537767526745
#define pi2_inv 0.159154943091895335768883763372

/// bipolar complex by @Flexi23
/// "logarithmic zoom with a spiral twist and a division by zero in the complex number plane." (from https://www.shadertoy.com/view/4ss3DB)

vec2 complex_mul(vec2 factorA, vec2 factorB){
  return vec2( factorA.x*factorB.x - factorA.y*factorB.y, factorA.x*factorB.y + factorA.y*factorB.x);
}

vec2 complex_div(vec2 numerator, vec2 denominator){
   return vec2( numerator.x*denominator.x + numerator.y*denominator.y,
                numerator.y*denominator.x - numerator.x*denominator.y)/
          vec2(denominator.x*denominator.x + denominator.y*denominator.y);
}

vec2 wrap_flip(vec2 uv){
	return vec2(1.)-abs(fract(uv*.5)*2.-1.);
}
 
float border(vec2 domain, float thickness){
   vec2 uv = fract(domain-vec2(0.5));
   uv = min(uv,1.-uv)*2.;
   return clamp(max(uv.x,uv.y)-1.+thickness,0.,1.)/(thickness);
}

float circle(vec2 uv, vec2 aspect, float scale){
	return clamp( 1. - length((uv-0.5)*aspect*scale), 0., 1.);
}

float sigmoid(float x) {
	return 2./(1. + exp2(-x)) - 1.;
}

float smoothcircle(vec2 uv, vec2 center, vec2 aspect, float radius, float sharpness){
	return 0.5 - sigmoid( ( length( (uv - center) * aspect) - radius) * sharpness) * 0.5;
}

float lum(vec3 color){
	return dot(vec3(0.30, 0.59, 0.11), color);
}

vec2 spiralzoom(vec2 domain, vec2 center, float n, float spiral_factor, float zoom_factor, vec2 pos){
	vec2 uv = domain - center;
	float angle = atan(uv.y, uv.x);
	float d = length(uv);
	return vec2( angle*n*pi2_inv + log(d)*spiral_factor*0., -log(d)*zoom_factor + 0.*n*angle*pi2_inv) + pos;
}

vec2 mobius(vec2 domain, vec2 zero_pos, vec2 asymptote_pos){
	return complex_div( domain - zero_pos, domain - asymptote_pos);
}

const float Pi = 3.14159;
const int zoom = 80;
const float speed = 0.05;
float fScale = 2.0;

void main(void)
{
		// domain map
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	
	// aspect-ratio correction
	vec2 aspect = vec2(1.,resolution.y/resolution.x);
	vec2 uv_correct = 0.5 + (uv -0.5)/ aspect.yx;
	vec2 mouse_correct = 0.5 + ( mouse.xy / resolution.xy - 0.5) / aspect.yx;
		
	float phase = time*0. + pi*1.;
	float dist = 1.;
	vec2 uv_bipolar = mobius(uv_correct, vec2(0.5 - dist*0.5, 0.5), vec2(0.5 + dist*0.5, 0.5));
	uv_bipolar = spiralzoom(uv_bipolar, vec2(0.), 8., 0., 0.9, mouse.yx*vec2(-1.,2.)*4. );
	uv_bipolar = vec2(-uv_bipolar.y,uv_bipolar.x); // 90° rotation 
	
	
	vec2 p= 0.5 + (wrap_flip(uv_bipolar) - 0.5)*2.;
	float t = -time*0.5;
	
	float ct = t * speed;
	
	for(int i=1;i<zoom;i++) {
		vec2 newp=p;
		newp.x+=0.25/float(i)*cos(float(i)*p.y+t*cos(ct)*0.3/40.0+0.03*float(i))*fScale+0.0;		
		newp.y+=0.25/float(i)*cos(float(i)*p.x+t*ct*0.3/50.0+0.03*float(i+10))*fScale+0.0;
		p=newp;
	}
	//p = uv;
	
	vec3 col=vec3((10.0*sin(9.0*p.x)),10.0*sin(5.0*p.y),10.0*sin(p.x+p.y)*cos(p.x+p.y));
	gl_FragColor=vec4(col, 1.0);
}