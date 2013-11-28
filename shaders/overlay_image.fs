#version 150
out vec4 outputF;


uniform vec2 resolution;
uniform float time, blendAlpha;
uniform float vuLow, vuMid;

uniform sampler2D overlayImage;


vec3 glitch(vec2 uv) {

	float 	centerBuffer 		= 0.01,
			vignetteStrength 	= 0.6,
			aberrationStrength 	= 5.0,
			glitchSize 			= 0.05;
		
	float 	chrDist,
			vigDist;
	
	float wave = vuLow;
	float wave2 = vuMid - 0.5;
	
	uv.x += sin( uv.y / 5.0 * wave2 )/5.0;
	
	vec2 uvG = vec2( 0.5 , sin( wave + wave2 * 2.0 ) );
	glitchSize *= wave + wave2 * 10.0;
	
	if( uv.y > uvG.y && uv.y < uvG.y + glitchSize )
	{
		uv.x = 0.8-uv.x;
		uv.y = sin(time*10.0) - uv.y;
	}

	if( 1.0-uv.y > 1.0-uvG.y && 1.0-uv.y < 1.0-uvG.y + glitchSize )
	{
		uv.x = uv.x;
		uv.y = uv.y;
	}
	
	uvG = vec2( 0.5 , sin( wave + wave2 * 2.5 ) );
	glitchSize *= 1.3;
	
	if( uv.y > uvG.y && uv.y < uvG.y + glitchSize )
	{
		uv.x = 0.3-uv.x;
		uv.y = sin(time*10.0) - uv.y;
	}

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

void main(void){
  vec2 uv=gl_FragCoord.xy/resolution.xy;

//  outputF=texture(overlayImage,uv);
   outputF.rgb = glitch(uv);

  //float m = sin(time);
  float m = blendAlpha;
  
  if (m>0.0) {
	outputF.a = (m)*(1.0-outputF.r);
  } else {
	outputF.a = (abs(m))*(outputF.r);
  }
  
//  outputF.a *= blendAlpha;
}

