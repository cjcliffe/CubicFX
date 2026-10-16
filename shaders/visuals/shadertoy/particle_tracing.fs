#version 150
out vec4 outputF;

uniform float time;
uniform vec2 resolution;
uniform float timerKick;
uniform float vuData[128];
uniform vec3 randColor;
uniform float blendAlpha;


// twitter: @eddbiddulph

vec3 cam_origin;
mat3 cam_rotation;
vec2 frag_coord;

vec3 rotateX(float a, vec3 v)
{
	return vec3(v.x, cos(a) * v.y + sin(a) * v.z, cos(a) * v.z - sin(a) * v.y);
}

vec3 rotateY(float a, vec3 v)
{
	return vec3(cos(a) * v.x + sin(a) * v.z, v.y, cos(a) * v.z - sin(a) * v.x);
}

vec3 round(vec3 x)
{
	return floor(x + vec3(0.5));
}

float torusDistance(vec3 p, float inner_radius, float outer_radius)
{
	vec3 ring_p = vec3(normalize(p.xy) * outer_radius, 0.0);
	return distance(p, ring_p) - inner_radius;
}

vec2 orbIntensity(vec3 p)
{
	float idx = p.x+p.y*20.0+p.z*20.0*20.0;
	return vec2(vuData[int(mod(idx,64.0))],0.3);

/*	// return a value to create some nice shapes out of particles
	vec3 ofs = vec3(0.0, 0.0, 0.0);
	float d0 = torusDistance(p - ofs, 0.5, 5.0);
	float d1 = torusDistance(rotateY(3.1415926 * 0.5, p) - ofs, 1.3, 8.0);
	float d2 = torusDistance(rotateX(0.2, rotateY(3.1415926, p)) - ofs, 1.5, 20.0);
	float amb = smoothstep(0.8, 1.0, cos(p.x * 10.0) * sin(p.y * 5.0) * cos(p.z * 7.0)) * 0.02;
	float wave = step(abs(p.y + 10.0 +  cos(p.z * 0.1) * sin(p.x * 0.1 + time) * 4.0), 1.0) * 0.3;
	return vec2(max(max(1.0 - step(4.0, length(p)), step(d0, 0.0)), step(d1, 0.0)) + amb + step(d2, 0.0) * 0.1 + wave,
				step(0.3, wave)); */
}

vec3 project(vec3 p)
{
	// transpose the rotation matrix. unfortunately tranpose() is not available.
	mat3 cam_rotation_t = mat3(vec3(cam_rotation[0].x, cam_rotation[1].x, cam_rotation[2].x),
							   vec3(cam_rotation[0].y, cam_rotation[1].y, cam_rotation[2].y),
							   vec3(cam_rotation[0].z, cam_rotation[1].z, cam_rotation[2].z));
	
	// transform into viewspace
	p = cam_rotation_t * (p - cam_origin);
	
	// project
	return vec3(p.xy / p.z, p.z);
}

vec3 orb(float rad, vec3 coord)
{
	// return the orb sprite
	return 6.0 * (1.0 - smoothstep(0.0, rad, length((coord.xy - frag_coord)))) *
			randColor * clamp(coord.z, 0.0, 1.0);
}

vec3 traverseUniformGrid(vec3 ro, vec3 rd)
{
	vec3 increment = vec3(1.0) / rd;
	vec3 intersection = ((floor(ro) + round(rd * 0.5 + vec3(0.5))) - ro) * increment;

	increment = abs(increment);
	ro += rd * 1e-3;
	
	vec3 orb_accum = vec3(0.0);
	
	// traverse the uniform grid
	for(int i = 0; i < 50; i += 1)
	{
		vec3 rp = floor(ro + rd * min(intersection.x, min(intersection.y, intersection.z)));
		
		vec2 orb_intensity = orbIntensity(rp);

		// get the screenspace position of the cell's centerpoint										   
		vec3 coord = project(rp + vec3(0.5));
		
		float rmask = smoothstep(0.0, 0.1, distance(frag_coord, coord.xy));
		
		// calculate the initial radius
		float rad = 0.5 / coord.z * (1.0 - smoothstep(0.0, 50.0, length(rp)));
		
		// adjust the radius
		rad *= 0.5 + 0.5 * sin(rp.x + time * 5.0) * cos(rp.y + time * 10.0) * cos(rp.z);
		
		orb_accum += orb(rad, coord) * orb_intensity.x * mix(1.0, rmask, orb_intensity.y);
		
		// step to the next ray-cell intersection
		intersection += increment * step(intersection.xyz, intersection.yxy) *
									step(intersection.xyz, intersection.zzx);
	}
	
	return orb_accum;
}


void main(void)
{
	// get the normalised device coordinates
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	frag_coord = uv * 2.0 - vec2(1.0);
	frag_coord.x *= resolution.x / resolution.y;

	// zoom in
	frag_coord *= 1.5;

	cam_origin = vec3(10.0*sin(time*0.14),10.0*cos(time*0.135),10.0*-sin(time*0.141));	//rotateX(time * 0.3, rotateY(time * 0.5, vec3(0.0, 0.0, -10.0 + 5.0 * cos(time * 0.1))));
	vec3 lookAt = vec3(10.0*sin(1000.0+time*0.14),10.0*cos(2000.0+time*0.135),10.0*-sin(3000.0+time*0.141));
	
	// calculate the rotation matrix
	vec3 cam_w = normalize(lookAt - cam_origin);
	vec3 cam_u = normalize(cross(cam_w, vec3(0.0, 1.0, 0.0)));
	vec3 cam_v = normalize(cross(cam_u, cam_w));
	
	cam_rotation = mat3(cam_u, cam_v, cam_w);
	
	vec3 ro = cam_origin,rd = cam_rotation * vec3(frag_coord, 1.0);
	
	// render the particles
	outputF.rgb = traverseUniformGrid(ro, rd);
	outputF.rgb = sqrt(outputF.rgb * 0.8);
	outputF.a = blendAlpha;
}
