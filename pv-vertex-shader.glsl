#version 410 core                                                  
                                                                   
layout (location = 0) in vec4 position;                            
layout (location = 1) in vec4 normal;
layout (location = 2) in vec4 pos_color; 
																	
layout(std140) uniform constants									
{																	
	mat4 mv_matrix;													
	mat4 view_matrix;												
	mat4 proj_matrix;												
	vec4 ball_color;
};				

uniform int settings[4];

// Light and material properties
uniform vec3 light_pos = vec3(20.0, 20.0, 0.0);
                                                                   
out VS_OUT                                                         
{                                                                  
    vec3 color;                                                    
} vs_out;                                                          
                                                                   
                                                                   
void main(void)                                                    
{                                                                  
    // Calculate view-space coordinate								
    vec4 P = mv_matrix * position;									

	//---------------Phong lighting taken from super bible code
    // Calculate normal in view space
    //vec3 N = mat3(mv_matrix) * normal.xyz;
	vec3 nn;
	if (settings[3] == 1) {
		nn = position.xyz;
	} else {
		nn = normal.xyz;
	}
	if (settings[0] == 1) {
		nn = -nn;
	}

	vec3 N = mat3(mv_matrix) * nn;

    // Calculate view-space light vector
    vec3 L = (light_pos - P.xyz) * mat3(mv_matrix);
    // Calculate view vector (simply the negative of the view-space position)
    vec3 V = -P.xyz;

    // Normalize all three vectors
    N = normalize(N);
    L = normalize(L);
    V = normalize(V);
    // Calculate R by reflecting -L around the plane defined by N
    vec3 R = reflect(-L, N);

    // Calculate the diffuse and specular contributions
    vec3 diffuse = max(dot(N, L), 0.0) * vec3(0.5, 0.5, 0.5);
    vec3 specular = pow(max(dot(R, V), 0.0), 256.0) * vec3(0.9);

	// Send output color to fragment shader
	if (settings[1] == 1) {
		if (settings[2] == 1) {
			vs_out.color = (vec3(0.2, 0.2, 0.2) + diffuse + specular) * -position.rgb;
		} else {
			vs_out.color = (vec3(0.2, 0.2, 0.2) + diffuse + specular) * ball_color.rgb;
		}
	} else {
		vs_out.color = (vec3(0.2, 0.2, 0.2) + diffuse + specular) * pos_color.rgb;
	}
		
    gl_Position = proj_matrix * P;								    
}                                             