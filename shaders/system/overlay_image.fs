#version 150
out vec4 outputF;


uniform vec2 resolution;
uniform float time, blendAlpha;
uniform float vuLow, vuMid, vuHigh;

uniform sampler2D overlayImage;


vec3 glitch1(vec2 uv) {

	float 	centerBuffer 		= 0.001,
			vignetteStrength 	= 0.6,
			aberrationStrength 	= 5.0,
			glitchSize 			= 0.25;
		
	float 	chrDist,
			vigDist;
	
	float wave = (vuLow+vuHigh)/3.0;
	float wave2 = vuLow - 0.5;
	
	uv.x += sin( uv.y / 5.0 * wave2 )/5.0;
	
	vec2 uvG = vec2( 0.5 , sin( wave + wave2 * 2.0 ) );
	glitchSize *= wave + wave2 * 10.0;
	
	/*
	if( uv.y > uvG.y && uv.y < uvG.y + glitchSize )
	{
		uv.x = 0.8-uv.x;
		uv.y = sin(time*10.0) - uv.y;
	}*/

	if( 1.0-uv.y > 1.0-uvG.y && 1.0-uv.y < 1.0-uvG.y + glitchSize )
	{
		uv.x = uv.x;
		uv.y = uv.y;
	}
	
	uvG = vec2( 0.5 , sin( wave + wave2 * 2.5 ) );
	glitchSize *= 1.3;
	
	/*if( uv.y > uvG.y && uv.y < uvG.y + glitchSize )
	{
		uv.x = 0.3-uv.x;
		uv.y = sin(time*10.0) - uv.y;
	}*/

	if( 1.0-uv.y > 1.0-uvG.y && 1.0-uv.y < 1.0-uvG.y + glitchSize )
	{
		uv.x = uv.x;
		uv.y = uv.y;
	}
	
	
	vec2 vecDist = uv - ( 0.5 , 0.5 );
	chrDist = vigDist = length( vecDist );
	
	chrDist	-= centerBuffer;
	if( chrDist < 0.0 ) chrDist = 0.0;

	vec2 uvR = uv * ( 1.0 + chrDist * 0.02 * aberrationStrength * wave ),
		 uvB = uv * ( 1.0 - chrDist * 0.02 * aberrationStrength * wave );
	 
	vec4 c;
	
	c.x = texture( overlayImage , uvR ).x; 
	c.y = texture( overlayImage , uv ).y; 
	c.z = texture( overlayImage , uvB ).z;
	
	c *= 1.0 - vigDist* vignetteStrength * (wave*2.0);
	
	float scanline = sin( uv.y * 800.0 * wave2 )/30.0; 
	c *= 1.0 + scanline; 
	
	return c.rgb;
}




// Noise generation functions borrowed from: 
// https://github.com/ashima/webgl-noise/blob/master/src/noise2D.glsl

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

float snoise(vec2 v)
  {
  const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                      0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                     -0.577350269189626,  // -1.0 + 2.0 * C.x
                      0.024390243902439); // 1.0 / 41.0
// First corner
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);

// Other corners
  vec2 i1;
  //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
  //i1.y = 1.0 - i1.x;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  // x0 = x0 - 0.0 + 0.0 * C.xx ;
  // x1 = x0 - i1 + 1.0 * C.xx ;
  // x2 = x0 - 1.0 + 2.0 * C.xx ;
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

// Permutations
  i = mod289(i); // Avoid truncation effects in permutation
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
		+ i.x + vec3(0.0, i1.x, 1.0 ));

  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;

// Gradients: 41 points uniformly over a line, mapped onto a diamond.
// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

// Normalise gradients implicitly by scaling m
// Approximation of: m *= inversesqrt( a0*a0 + h*h );
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

// Compute final noise value at P
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}


vec3 glitch2(vec2 uv)
{
	float imp = (vuLow+vuHigh)/2.0;
	float jerkOffset = (1.0-step(snoise(vec2(time*1.3,5.0)),0.8))*0.05;
	
	float wiggleOffset = snoise(vec2(time*15.0,uv.y*80.0))*0.003;
	float largeWiggleOffset = snoise(vec2(time*1.0,uv.y*25.0))*0.004;
	
	float xOffset = (wiggleOffset + largeWiggleOffset + jerkOffset)*imp;
	
	float red 	=   texture(	overlayImage, 	vec2(uv.x + xOffset -0.01*imp,uv.y)).r;
	float green = 	texture(	overlayImage, 	vec2(uv.x + xOffset,	  uv.y)).g;
	float blue 	=	texture(	overlayImage, 	vec2(uv.x + xOffset +0.01*imp,uv.y)).b;
	
	vec3 color = vec3(red,green,blue);
	float scanline = sin(uv.y*800.0)*imp*0.04;
	color -= scanline;
	
	return color;
}


void main(void){
  vec2 uv=gl_FragCoord.xy/resolution.xy;

//  outputF=texture(overlayImage,uv);
	
	if (mod(time,10.0)<5.0) {
		outputF.rgb = glitch1(uv);
	} else {
		outputF.rgb = glitch2(uv);
	}
	
  //float m = sin(time);
  float m = blendAlpha;
  
  //float q = outputF.r;
  float q = (outputF.r+outputF.g+outputF.b)/3.0;
  
  if (m>0.0) {
	outputF.a = (m)*(1.0-q);
  } else {
	outputF.a = (abs(m))*(q);
  }
  
  //outputF.a *= blendAlpha;
}

