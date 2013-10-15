#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

bool near(float i, float t, float e){
	return i > t-e && i < t+e;
}
float nearf(float i, float t, float ea){
	
	if(i < t+ea && i > t){
		return 1.0 - (i-t)/(ea);
	}else if(i > t-ea && i <= t){
		return (i-(t-ea))/(ea);
		
		return 0.0;
	}
	return 0.0;
}


void main( void ) {
	float f = 0.0;
	vec2 position = ( gl_FragCoord.xy );
	vec2 mousepos = mouse*resolution;
	
	float line_thick = 1.0;
	float half_line_thick = 0.5;
	float amp = resolution.y * 0.15;
//	time = 1.0;
	float o1 = (
		sin( position.x *-0.005 + time*1.0) * 0.5 
		+ cos(position.x * -0.0033 + time*2.0) * 0.5
	) * amp;
	
	float o1_1 = o1;
	
	float o2 = (
		sin( position.x *-0.0065 + time*3.3) * 0.5 
		+ cos(position.x * -0.005 + time * 2.7) * 0.5
	) * amp*0.8;

	float o3 = (
		cos( position.x *-0.004 + time*1.3) * 0.33 
		+ cos(position.x * -0.005 + time * 2.5) * 0.33
		+ sin( position.x *-0.0065 + time*1.3) * 0.33
	) * amp*0.6;
	float aoe = 200.0;
	if(mousepos.x > position.x && aoe - (mousepos.x-position.x) > 0.0){
		 f = (aoe -(mousepos.x-position.x)) / aoe;
		f = smoothstep(0.0,1.0,f);
		o1_1 = o1 + sin(position.x *-0.08 + time*23.0) * f * amp * mouse.y;	
	}else if(mousepos.x < position.x && aoe - (position.x-mousepos.x) > 0.0){
		 f = (aoe - (position.x-mousepos.x))  / aoe;
		f = smoothstep(0.0,1.0,f);
		o1_1 = o1 + sin(position.x *-0.08 + time*23.0) * f * amp * mouse.y;	
	}
	if(mousepos.x > position.x && aoe - (mousepos.x-position.x) > 0.0){
		float f = (aoe -(mousepos.x-position.x)) / aoe;
		f = smoothstep(0.0,1.0,f);
		o2 += sin(position.x *-0.075 + time*30.0) * f * amp * mouse.y;	
	}else if(mousepos.x < position.x && aoe - (position.x-mousepos.x) > 0.0){
		float f = (aoe - (position.x-mousepos.x))  / aoe;
		f = smoothstep(0.0,1.0,f);
		o2 += sin(position.x *-0.075 + time*30.0) * f * amp * mouse.y;	
	}
	if(mousepos.x > position.x && aoe - (mousepos.x-position.x) > 0.0){
		float f = (aoe -(mousepos.x-position.x)) / aoe;
		f = smoothstep(0.0,1.0,f);
		o3 += sin(position.x *-0.09 + time*25.0) * f * amp * mouse.y;	
	}else if(mousepos.x < position.x && aoe - (position.x-mousepos.x) > 0.0){
		float f = (aoe - (position.x-mousepos.x))  / aoe;
		f = smoothstep(0.0,1.0,f);
		o3 += sin(position.x *-0.09 + time*25.0) * f * amp * mouse.y;	
	}
	/*
	if(resolution.x*0.75 > position.x && aoe - (resolution.x*0.75-position.x) > 0.0){
		float f = (aoe -(resolution.x*0.75-position.x)) / aoe;
		f = smoothstep(0.0,1.0,f);
		o3 += sin(position.x *0.09 + time*25.0) * f * amp * mouse.y;	
	}else if(resolution.x*0.75 < position.x && aoe - (position.x-resolution.x*0.75) > 0.0){
		float f = (aoe - (position.x-resolution.x*0.75))  / aoe;
		f = smoothstep(0.0,1.0,f);
		o3 += sin(position.x *0.09 + time*25.0) * f * amp * mouse.y;	
	} */
	
	vec4 c1 = vec4(1.0,0.4,0.0,1.0);
	vec4 c2 = vec4(0.0,0.4,1.0,1.0);
	vec4 c3 = vec4(0.0,1.0,0.0,1.0);
	vec4 o = vec4(0.0,0.0,0.0,0.0);
	for(float j = 0.0; j < 1.0; j++){
		float glow = 80.0;//(pow(20.0 * sin(mouse.y / 0.32), 1.2));
		float modulator = (mouse.y ) * 80.2;
		float n = nearf(position.y,o1 + resolution.y*0.5,(glow + f * modulator))*0.5;
		o += c1 * n; vec4( n,0.0,0.0, 0.5);
		if(near(position.y,o1_1 + resolution.y*0.5 ,3.0)){ o += c1; }
		//*	
		n = nearf(position.y,o2 + resolution.y*0.5,glow)*0.5;
		o += c2*n;
		if(near(position.y,o2 + resolution.y*0.5 ,3.0)){ o += c2; }
		
		
		o += c3*nearf(position.y,o3 + resolution.y*0.5,glow)*0.5;		
		if(near(position.y,o3 + resolution.y*0.5 ,3.0)){ o = c3; }
		//*/
		
		gl_FragColor = o;
	
	}
}
