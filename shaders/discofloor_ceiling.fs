#version 150
out vec4 outputF;

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
//tigrou 2013.02.28 (for gamedevdtack exchange)

uniform float vuData[128];
uniform vec3 randColor;
uniform float timerKick;

float get2DTexture(float x, float y)
{
	
   return sin(x)+sin(y);
}

vec2 rotate(vec2 v, float angle)
{
    float x = v.x * cos(angle) - v.y * sin(angle);
    float y = v.x * sin(angle) + v.y * cos(angle);
    return vec2(x, y);
}

void main( void ) {
  
      vec2 position = ( gl_FragCoord.xy / resolution.xy ) - vec2(0.5,0.5);
      
     float x = position.x;	
     float y = position.y;
	
     float horizon = 0.1; //adjust if needed
     float fov = 0.5; 
	
     float px = x;
     float py = y - horizon - fov; 
     float pz = y - horizon;  	
	
     //projection 
     float sx = px / pz;
     float sy = py / pz; 
	
     float scaling = 10.0; //adjust if needed, depends of texture size

     //rotate	  
     vec2 r = rotate(vec2(sx, sy), time/10.0);	
     sx = r.x;
     sy = r.y;
   
     //move	
     sx += (time) / 1.0 * sign(pz);
	
	 int idx = int(mod(sx,1.0)*8.0)+int(mod(sy,1.0)*8.0)*8;
     float color = vuData[abs(idx-64)%64]/6.0; //get2DTexture(sx * scaling, sy * scaling);  

     //shading
     color *= pow(pz,1.5)*280.0;	
	
     outputF = vec4( randColor*color,  1.0);

}