// Trapped by curiousfly

#version 150
out vec4 outputF;


uniform float time;
uniform vec2 resolution;
uniform float vuLow;
uniform float vuMid;
uniform float vuHigh;
uniform float vuData[128];
uniform vec3 randColor;

const float PI = 3.1416;
const float TWOPI = 2.0*PI; 

void main( void ) {  
	vec2 uPos = ( gl_FragCoord.xy / resolution.y );//normalize wrt y axis
	uPos -= vec2((resolution.x/resolution.y)/2.0, 0.5);//shift origin to center
	
	float multiplier = 0.00001;
	const float step = 0.004;
	const float loop = 100.0;
	const float timeSCale = 0.75;

	
	vec3 redGodColor = vec3(0.0);
	for(float i=1.0;i<loop;i++){
		float t = time*timeSCale-step*i;
		vec2 point = 1.2*vec2(0.5*sin(t*4.0+200.0), 0.75*sin(t+10.0));
		point += 1.2*vec2(0.85*cos(t*2.0), 0.45*sin(t*3.0));
		point /= 2.0;
		float componentColor= (multiplier+0.02*vuLow)/((uPos.x-point.x)*(uPos.x-point.x) + (uPos.y-point.y)*(uPos.y-point.y))/i;
		redGodColor += vec3(componentColor, componentColor/3.0, componentColor/3.0);
	}
	
	vec3 blueGodColor = vec3(0.0);
	for(float i=1.0;i<loop;i++){		
		float t = time*timeSCale-step*i;
		vec2 point = 1.2*vec2(0.75*sin(t), 0.5*sin(t));
		point += 1.2*vec2(0.75*cos(t*4.0), 0.5*sin(t*3.0));
		point /= 2.0;
		float componentColor= (multiplier+0.02*vuMid)/((uPos.x-point.x)*(uPos.x-point.x) + (uPos.y-point.y)*(uPos.y-point.y))/i;
		blueGodColor += vec3(componentColor/3.0, componentColor/3.0, componentColor);
	}
	
	vec3 greenGodColor = vec3(0.0);
	for(float i=1.0;i<loop;i++){
		float t = time*timeSCale-step*i;
		vec2 point = 1.2*vec2(0.75*sin(t*3.0+20.0), 0.45*sin(t*2.0+40.0));
		point += 1.2*vec2(0.35*cos(t*2.0+100.0), 0.5*sin(t*3.0));
		point /= 2.0;
		float componentColor= (multiplier+0.02*vuHigh)/((uPos.x-point.x)*(uPos.x-point.x) + (uPos.y-point.y)*(uPos.y-point.y))/i;
		greenGodColor += vec3(componentColor/3.0, componentColor, componentColor/3.0);
	}
	
	float angle = (atan(uPos.y, uPos.x)+PI) / TWOPI;
	float radius = sqrt(uPos.x*uPos.x + uPos.y*uPos.y);
	
	float wallColor = abs(sin(pow(radius, 0.1)*100.0 - time*10.0) )/2.2;
	
	wallColor *= abs(sin(pow(radius, 0.1)*500.0 + time + 100.0) )/2.2;	
	wallColor *= abs(sin(angle*PI*50.0 - time*5.0))/2.2;
	wallColor *= abs(sin(angle*PI*100.0 + time*5.0))/2.2;
	
	int vuxPos = int(abs(gl_FragCoord.x/resolution.x)*128.0);	
	if ((gl_FragCoord.y/resolution.y)<=vuData[vuxPos]) {
		wallColor += 0.25*vuData[vuxPos];
	}
	
	
	int idx = int(clamp(radius*128.0,0.0,128.0));
		
	vec3 color = radius * wallColor * randColor;
	color += randColor*vuData[idx]*0.05;
	
	color *= blueGodColor+redGodColor+greenGodColor;
	color *= 200.0;
	color += blueGodColor+redGodColor+greenGodColor;


	outputF = vec4(color, 1.0);
}