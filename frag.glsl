// Benjamin Laws
// CSE 40166 - Computer Graphics
// December 17, 2014
// Final Project

uniform sampler2D u_sTexture;

uniform vec3 u_kAmbient;
uniform vec3 u_kDiffuse;
uniform vec3 u_kSpecular;
uniform float u_fShininess;

varying vec3 v_vPointLightDir;
varying vec3 v_vSpotLightDir;
varying vec3 v_vSpotLightUpDir;

varying vec3 v_vNormal;
varying vec2 v_vTexCoord;
varying vec3 v_vEyeDir;

//==========================================================

void main()
{
    // lighting vectors
    vec3 pointL = normalize( v_vPointLightDir );
    vec3 spotL = normalize( v_vSpotLightDir );
    vec3 N = gl_FrontFacing ? normalize( v_vNormal ) : normalize( -v_vNormal );
    vec3 V = normalize( v_vEyeDir );
    vec3 pointR = reflect( pointL, N );
    vec3 spotR = reflect( spotL, N );
    vec3 U = normalize( v_vSpotLightUpDir );

    // dot products
    float pointNdotL = max( 0.0, dot( pointL, N ) );
    float pointVdotR = max( 0.0, dot( V, pointR ) );
    float spotNdotL = max( 0.0, dot( spotL, N ) );
    float spotVdotR = max( 0.0, dot( V, spotR ) );
    
    // constrain spotlight
    spotNdotL *= pow( dot( spotL, U ), 1.0 );
    spotVdotR *= pow( dot( spotL, U ), 1.0 );
    
    // intensities
    vec3 baseColor = texture2D( u_sTexture, v_vTexCoord ).xyz;
    vec3 diffuse = u_kDiffuse * ( pointNdotL + spotNdotL );
    vec3 specular = u_kSpecular * ( pow( pointVdotR, u_fShininess )
                                    + pow( spotVdotR, u_fShininess ) );

    // fragment color
    gl_FragColor = vec4( vec3( u_kAmbient + diffuse + specular ) * baseColor, 1.0 );
    gl_FragColor = min( vec4( 0.9 ), gl_FragColor );
}
