#version 150
out vec4 outputF;

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

const vec3 eyePosition = vec3(0.0, 0.0, -3.0);
const float screenZ = -1.0;
const vec3 backgroundColour = vec3(0.0,0.0,0.1);
const float noIntersectionT = -1.0;
const float epsilon = 0.0001;
const int noBounces = 2;
const float distanceAttenuationPower = 0.2;
const float reflectionRefractionFactorLowerLimit = 0.1;

struct Ray {
	vec3 start;
	vec3 direction;
};
	
struct Material {
	vec3 ambientColour;
	vec3 diffuseColour;
	vec3 specularColour;
	vec3 reflectedColour;
	vec3 refractedColour;
	float reflectiveness;
	float refractiveness;
	float shinyness;
	float refractiveIndex;
};
	
struct Sphere {
	vec3 position;
	float radius;
	Material material;
};
	
struct Plane {
	vec3 position;
	vec3 normal;
	float d;
	Material material1;
	Material material2;
	float squareSize;
};
	
struct Cube {
	vec3 position;
	float size;
	Material material;
};
	
struct Intersection {
	vec3 position;
	float t;
	vec3 normal;
	bool inside;
	Material material;
};
	
struct Light {
	vec3 position;
	vec3 ambientColour;
	vec3 diffuseColour;
	vec3 specularColour;
};
	
Material defaultMaterial = 	Material ( vec3(0.0, 0.0, 0.0), vec3(0.5,0.5,0.5), vec3(1.0,1.0,1.0), vec3(1.0,1.0,1.0), vec3(0.0,0.0,0.0), 0.0, 0.0, 40.0, 1.0 );
Material redMaterial = 		Material ( vec3(0.0, 0.0, 0.0), vec3(1.0,0.0,0.0), vec3(1.0,1.0,1.0), vec3(1.0,1.0,1.0), vec3(0.0,0.0,0.0), 0.5, 0.0, 40.0, 1.0 );
Material blueMaterial = 	Material ( vec3(0.0, 0.0, 0.0), vec3(0.0,0.0,1.0), vec3(1.0,1.0,1.0), vec3(1.0,1.0,1.0), vec3(0.0,0.0,0.0), 0.5, 0.0, 40.0, 1.0 );
Material glassMaterial = 	Material ( vec3(0.0, 0.0, 0.0), vec3(0.0,0.0,0.0), vec3(1.0,1.0,1.0), vec3(0.0,0.0,0.0), vec3(1.0,1.0,1.0), 0.0, 1.0, 40.0, 1.5 );
Material blackMaterial = 	Material ( vec3(0.0, 0.0, 0.0), vec3(0.1,0.1,0.1), vec3(0.1,0.1,0.1), vec3(0.1,0.1,0.1), vec3(0.5,0.5,0.5), 0.1, 0.0, 1.0, 1.0 );
Material whiteMaterial = 	Material ( vec3(0.0, 0.0, 0.0), vec3(0.8,0.8,0.8), vec3(0.1,0.1,0.1), vec3(0.1,0.1,0.1), vec3(0.5,0.5,0.5), 0.1, 0.0, 1.0, 1.0 );
Material mirrorMaterial = 	Material ( vec3(0.0, 0.0, 0.0), vec3(0.2,0.2,0.2), vec3(1.0,1.0,1.0), vec3(1.0,1.0,1.0), vec3(0.0,0.0,0.0), 1.0, 0.0, 40.0, 1.0 );

Light light = Light ( vec3(0.5, 0.5, -1.0), vec3(1.0,1.0,1.0), vec3(1.0,1.0,1.0), vec3(1.0,1.0,1.0) );
	
bool intersectSphere(Ray ray, Sphere sphere, inout Intersection intersection) {
	
	float t0, t1, t;
	
	vec3 l = sphere.position - ray.start;
	float tca = dot(l, ray.direction);
	if ( tca < 0.0 )
		return false;
	float d2 = dot (l, l) - (tca * tca);
	float r2 = sphere.radius*sphere.radius;
	if ( d2 > r2 )
		return false;
	float thc = sqrt(r2 - d2);
	t0 = tca - thc;
	t1 = tca + thc;
		
	if ( t0 < 0.0 )
		t = t1;
	else if ( t1 < 0.0 )
		t = t0;
	else
		t = min(t0,t1);
			
	intersection.position = ray.start + t * ray.direction;
	intersection.t = t;
	intersection.normal = normalize ( intersection.position - sphere.position );
	intersection.material = sphere.material;
	intersection.inside = min(t0,t1) < epsilon && max(t0,t1) >= epsilon;
	//if (intersection.inside) intersection.normal = -intersection.normal;
	return true;
}

bool intersectPlane(Ray ray, Plane plane, inout Intersection intersection) {

	//float t = dot ( plane.normal, plane.position - ray.start ) / dot ( plane.normal, ray.direction );
	float t = - ( dot ( plane.normal, ray.start ) + length(plane.position) ) / ( dot ( plane.normal, ray.direction ) );
	//float t = - ( dot ( plane.normal, ray.start ) - plane.d ) / ( dot ( plane.normal, ray.direction ) );
	
	if ( t < epsilon )
		return false;
	
	intersection.position = ray.start + t * ray.direction;
	intersection.t = t;
	intersection.normal = plane.normal;
	
	/*
	vec3 squarePosition = (intersection.position - plane.position) / (plane.squareSize*2.0);
	float l = 0.5;
	float h = 0.5;
	if ( (fract(squarePosition.x) < l && fract(squarePosition.y) < l && fract(squarePosition.z) < l) || 
	     (fract(squarePosition.x) > h && fract(squarePosition.y) < l && fract(squarePosition.z) > h) ||
	     (fract(squarePosition.x) < l && fract(squarePosition.y) > h && fract(squarePosition.z) > h) ||
	     (fract(squarePosition.x) > h && fract(squarePosition.y) > h && fract(squarePosition.z) < l)
	   )
	    intersection.material = blackMaterial;
	 else
	    intersection.material = whiteMaterial;
	*/
	
	plane.position.z+=time*3.0;	
	
	vec3 p = mod ( intersection.position - plane.position - epsilon, plane.squareSize*2.0 );
	
	float s = plane.squareSize;
	float l = s;
	float h = s;
	if ( ( p.x < l && p.y < l && p.z < l ) ||
	     ( p.x > h && p.y < l && p.z > h ) ||
	     ( p.x < l && p.y > h && p.z > h ) ||
	     ( p.x > h && p.y > h && p.z < l ) )
		intersection.material = plane.material1;
	else
		intersection.material = plane.material2;

	//intersection.material = whiteMaterial;
	
	return true;
	
}

bool intersectCube(Ray ray, Cube cube, inout Intersection intersection) {
	
	vec3 minp = cube.position - cube.size/2.0;
	vec3 maxp = cube.position + cube.size/2.0;
	
	float tmin = (minp.x - ray.start.x) / ray.direction.x;
	float tmax = (maxp.x - ray.start.x) / ray.direction.x;
	if (tmin > tmax) { float s = tmin; tmin = tmax; tmax = s; }
	float tymin = (minp.y - ray.start.y) / ray.direction.y;
	float tymax = (maxp.y - ray.start.y) / ray.direction.y;
	if (tymin > tymax) { float s = tymin; tymin = tymax; tymax = s; }
	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;
	float tzmin = (minp.z - ray.start.z) / ray.direction.z;
	float tzmax = (maxp.z - ray.start.z) / ray.direction.z;
	if (tzmin > tzmax) { float s = tzmin; tzmin = tzmax; tzmax = s; }
	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	//if ((tmin > r.tmax) || (tmax < r.tmin)) return false;
	//if (r.tmin < tmin) r.tmin = tmin;
	//if (r.tmax > tmax) r.tmax = tmax;
	
	float t;
	if ( tmin < epsilon && tmax < epsilon )
		return false;
	if ( tmin < epsilon )
		t = tmax;
	else if ( tmax < epsilon )
		t = tmin;
	else
		t = min(tmin, tmax);
		
	intersection.t = t;
	intersection.position = ray.start + t * ray.direction;
	intersection.material = cube.material;
	if ( tmin < epsilon && tmax > epsilon )
		intersection.inside = true;
	
	if ( abs(intersection.position.x - maxp.x) < epsilon )
		intersection.normal = vec3(1.0, 0.0,0.0);
	else if ( abs(intersection.position.x - minp.x) < epsilon )
		intersection.normal = vec3(-1.0,0.0,0.0);
	else if ( abs(intersection.position.y - maxp.y) < epsilon )
		intersection.normal = vec3(0.0,1.0,0.0);
	else if ( abs(intersection.position.y - minp.y ) < epsilon )
		intersection.normal = vec3(0.0,-1.0,0.0);
	else if ( abs(intersection.position.z - maxp.z ) < epsilon )
		intersection.normal = vec3(0.0,0.0,1.0);
	else
		intersection.normal = vec3(0.0,0.0,-1.0);
	
	return true;
}

Intersection noIntersection () {
	return Intersection(vec3(0.0,0.0,0.0),noIntersectionT,vec3(0.0,0.0,0.0),false,defaultMaterial);
}

bool hasIntersection(Intersection i) {
	return i.t != noIntersectionT;	
}

vec3 lighting(Ray ray, Intersection intersection, float shadowFactor) {
	
	vec3 colour = light.ambientColour * intersection.material.ambientColour;

	vec3 lightDir = normalize(light.position - intersection.position);
	vec3 eyeDir = normalize ( eyePosition - intersection.position );
	colour += shadowFactor * light.diffuseColour * intersection.material.diffuseColour * max(dot(intersection.normal,lightDir), 0.0);
	vec3 reflected =  normalize ( reflect ( -lightDir, intersection.normal ) );
	colour += shadowFactor * light.specularColour * intersection.material.specularColour * pow ( max ( dot(reflected, eyeDir), 0.0) , intersection.material.shinyness );	
	colour *= min ( 1.0/pow(length(intersection.position - ray.start), distanceAttenuationPower), 1.0);

	return colour;
}

void intersection2(Ray ray, inout Intersection minIntersection) {

	Intersection intersection = noIntersection();
	Sphere sphere;
		
	sphere = Sphere(vec3(-0.5,sin(time) + 0.5,0.5),0.4, redMaterial);
	
	if ( intersectSphere ( ray, sphere, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	
	sphere = Sphere(vec3(0.5,cos(time) + 0.5,0.5),0.4, blueMaterial );
	
	if ( intersectSphere ( ray, sphere, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	
	sphere = Sphere(vec3(0.0,0.0,-0.5),0.4, glassMaterial );
	
	if ( intersectSphere ( ray, sphere, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	Plane plane;
	
	plane = Plane(vec3(0.0,-1.0,0.0), vec3(0.0,1.0,0.0), -1.0, blackMaterial, whiteMaterial, 1.0);
	
	if ( intersectPlane ( ray, plane, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	
	Plane plane2 = Plane(vec3(0.0,0.0,10.0), vec3(0.0,0.0,-1.0), -10.0, blackMaterial, whiteMaterial, 1.0);
	
	if ( intersectPlane ( ray, plane2, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	
	Cube cube = Cube(vec3(0.8,-0.6,-0.7), 0.6, mirrorMaterial );
	if ( intersectCube ( ray, cube, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	
}

const int noSpheres = 4;
Sphere spheres[noSpheres];
vec3 velocity[noSpheres];
vec3 position[noSpheres];

void intersection(Ray ray, inout Intersection minIntersection) {

	Intersection intersection = noIntersection();
	float r = 0.25;
	float fi;
	for (int i=0; i<noSpheres; i++) {
		fi = float(i);
		position[i] = vec3(sin(fi*1000.0),sin(fi*10000.0),sin(fi*100.0));
		spheres[i].radius = r;
		spheres[i].material = (mod(fi,2.0)==1.0) ? blueMaterial : redMaterial;
		if ( mod(fi,4.0)==0.0 ) {
			spheres[i].material = glassMaterial;
		}
		velocity[i] = vec3(cos(fi*200.0+fi*400.0+500.0), cos(fi*400.0+fi*2000.0+500.0), cos(fi*600.0)) / 2.0;
	}
	
	vec3 p;

	for (int i=0; i<noSpheres; i++) {
		p = position[i] + ( velocity[i] * time  );
		
		p = mod ( p, 4.0 ) - 2.0;
		if ( p.x > 1.0 )
			p.x = 2.0 - p.x;
		if ( p.y > 1.0 )
			p.y = 2.0 - p.y;
		if ( p.z > 1.0 )
			p.z = 2.0 - p.z;
		if ( p.x < -1.0 )
			p.x = -2.0 - p.x;
		if ( p.y < -1.0 )
			p.y = -2.0 - p.y;
		if ( p.z < -1.0 )
			p.z = -2.0 - p.z;
		spheres[i].position = p;
		
		if ( intersectSphere ( ray, spheres[i], intersection ) ) {
			if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
		}
	}
	Plane plane;
	float s = ( 1.0 + r*2.0 ) / 2.0;
	
	plane = Plane(vec3(0.0,-1.0 - r,0.0), vec3(0.0,1.0,0.0), -0.0, blackMaterial, whiteMaterial, s);
	if ( intersectPlane ( ray, plane, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}

	plane = Plane(vec3(0.0,1.0 + r,0.0), vec3(0.0,-1.0,0.0), -0.0, blackMaterial, whiteMaterial, s);
	if ( intersectPlane ( ray, plane, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}	

	plane = Plane(vec3(-1.0 - r,0.0,0.0), vec3(1.0,0.0,0.0), -0.0, blackMaterial, whiteMaterial, s);
	if ( intersectPlane ( ray, plane, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	
	plane = Plane(vec3(1.0 + r,0.0,0.0), vec3(-1.0,0.0,0.0), -0.0, blackMaterial, whiteMaterial, s);
	if ( intersectPlane ( ray, plane, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}

	plane = Plane(vec3(0.0,0.0,1.0 + r), vec3(0.0,0.0,-1.0), -0.0, blackMaterial, whiteMaterial, s);
	if ( intersectPlane ( ray, plane, intersection ) ) {
		if ( !hasIntersection(minIntersection) || intersection.t < minIntersection.t )
			minIntersection = intersection;
	}
	
}
	
vec3 traceRay(Ray ray, inout Intersection minIntersection) {

	intersection(ray, minIntersection);
	
	if ( hasIntersection(minIntersection) ) {
		Ray shadowRay = Ray ( minIntersection.position + epsilon * minIntersection.normal, normalize(light.position - minIntersection.position) );
		Intersection shadowIntersection = noIntersection();
		intersection ( shadowRay, shadowIntersection );
		float shadowFactor = 1.0;
		
		if ( hasIntersection(shadowIntersection) && shadowIntersection.t < length(light.position - minIntersection.position) ) {
			//shadowFactor = 1.0 * shadowIntersection.material.refractiveness;	
			shadowFactor = 0.8;
		}

		return lighting ( ray, minIntersection, shadowFactor );
	}
	else
		return backgroundColour;
}

Ray myRefract(Ray ray, Intersection intersection) {
	float cosI = dot(ray.direction,intersection.normal);
	float n1,n2;
	Ray refractedRay;
	if ( intersection.inside ) {
		n1 = intersection.material.refractiveIndex;
		n2 = 1.0;
		intersection.normal = -intersection.normal;
		//colour = vec3(1.0,0.,0.);
	}
	else {
		n2 = intersection.material.refractiveIndex;
		n1 = 1.0;
		cosI = -cosI;
		//colour = vec3(0.,0.,1.);
	}
	
	float cosT = 1.0 - pow(n1/n2, 2.0) * (1.0 - pow(cosI, 2.0));
	
	if (cosT < 0.0) {
		refractedRay.direction = normalize ( reflect ( ray.direction, intersection.normal ) );
		refractedRay.start = intersection.position + (epsilon * intersection.normal);
	}
	else {
		cosT = sqrt(cosT);
		refractedRay.direction = normalize ( ray.direction * (n1/n2) + intersection.normal * ((n1/n2) * cosI - cosT) );
		refractedRay.start = intersection.position - (epsilon * intersection.normal);
		//colour = vec3(0.0,1.0,0.0);
	}
	return refractedRay;
}

vec3 recurseRay(Ray ray) {
	
	vec3 colour = vec3(0.0,0.0,0.0);
	vec3 localColour;
	Intersection intersection = noIntersection();
	
	Intersection firstIntersection = noIntersection();
	colour = traceRay ( ray, firstIntersection );
	
	if (hasIntersection(firstIntersection)) {
	
		float reflectionFactor = firstIntersection.material.reflectiveness;
		
		if ( reflectionFactor >= reflectionRefractionFactorLowerLimit ) {
			
			ray.start = firstIntersection.position;
			ray.direction = reflect ( ray.direction, firstIntersection.normal );
			
			for (int i=0; i<noBounces; i++) {
				intersection = noIntersection();
				localColour = traceRay(ray, intersection);
				if ( hasIntersection(intersection) ) {	
					
					colour += reflectionFactor * localColour;
					reflectionFactor *= intersection.material.reflectiveness;
					if ( reflectionFactor < reflectionRefractionFactorLowerLimit )
						break;
					
					ray.start = intersection.position;
					ray.direction =  ( reflect ( ray.direction, intersection.normal ) );
				}
				else {
					colour += reflectionFactor * backgroundColour;
					break;
				}
			}
			
		}
	
		float refractionFactor = firstIntersection.material.refractiveness;
		
		if ( refractionFactor >= reflectionRefractionFactorLowerLimit ) {		
		
			/*
			vec3 refractedDirection = normalize ( refract ( ray.direction, firstIntersection.normal, firstIntersection.material.refractiveIndex ) );
			ray.start = firstIntersection.position + epsilon * refractedDirection;
			ray.direction = refractedDirection;
			*/
			ray = myRefract(ray, firstIntersection);
			
			for (int i=0; i<noBounces; i++) {
				intersection = noIntersection();
				localColour = traceRay(ray, intersection);
				if ( hasIntersection(intersection) ) {	
		
					colour += refractionFactor * localColour;
					refractionFactor *= intersection.material.refractiveness;
					if ( refractionFactor < reflectionRefractionFactorLowerLimit )
						break;
					
					ray = myRefract ( ray, intersection );

				}
				else {
					colour += refractionFactor * backgroundColour;
					break;
				}
			}
		}
	}
	else
		return backgroundColour;
	
	return colour;
}

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}
	
void main( void ) {
	float sx = 0.8;
	float sy = 0.8;
	mat4 mx = rotationMatrix ( vec3(0.0,1.0,0.0), mouse.x * sx - sx/2.0 );
	mat4 my = rotationMatrix ( vec3(1.0,0.0,0.0), mouse.y * sy - sy/2.0 );
	
	float y = (gl_FragCoord.y * 2.0)/resolution.y - 1.0;
	float ratio = resolution.x/resolution.y;
	float x = ((gl_FragCoord.x * 2.0)/resolution.x - 1.0) * ratio;
	vec4 pixelPosition = vec4(x,y,screenZ,1.0) * mx * my;
	vec4 rotatedEyePosition = vec4(eyePosition,1.0) * mx * my;
	
	//Ray ray = Ray(pixelPosition.xyz, normalize(-pixelPosition.xyz));
	Ray ray = Ray(rotatedEyePosition.xyz, normalize(pixelPosition-rotatedEyePosition).xyz);
	gl_FragColor = vec4(recurseRay(ray),1.0);
}