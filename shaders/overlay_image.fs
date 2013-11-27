#version 150
out vec4 outputF;


uniform vec2 resolution;
uniform float time;
uniform float vuLow;

uniform sampler2D overlayImage;


void main(void){
  vec2 vPos=gl_FragCoord.xy/resolution.xy;

  outputF=texture(overlayImage,vPos);

  float m = sin(time);
  
  if (m>0.0) {
	outputF.a = (m)*(1.0-outputF.r);
  } else {
	outputF.a = (abs(m))*(outputF.r);
  }
}

