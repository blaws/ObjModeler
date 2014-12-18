// Benjamin Laws
// CSE 40166 - Computer Graphics
// December 17, 2014
// Final Project

uniform mat4 u_mv_Matrix;

attribute vec3 a_vPosition;
attribute vec2 a_vTexCoord;
attribute vec3 a_vNormal;

varying vec3 v_vNormal;
varying vec2 v_vTexCoord;
varying vec3 v_vEyeDir;

varying vec3 v_vPointLightDir;
varying vec3 v_vSpotLightDir;
varying vec3 v_vSpotLightUpDir;

const vec3 pointLightLoc = vec3( 0.0, -5.0, 5.0 );
const vec3 spotLightLoc = vec3( -15, 5.0, 0.0 );
const vec3 spotLightDir = vec3( 1.0, 0.0, 0.0 );

//==========================================================

void main()
{
    gl_Position = u_mv_Matrix * vec4( a_vPosition, 1.0 );
    
    v_vNormal = normalize( a_vNormal );
    v_vTexCoord = a_vTexCoord;
    v_vEyeDir = normalize( -gl_Position.xyz );
    
    v_vPointLightDir = normalize( pointLightLoc - a_vPosition ).xyz;
    v_vSpotLightDir = normalize( spotLightLoc - a_vPosition ).xyz;
    v_vSpotLightUpDir = normalize( spotLightDir );
}
