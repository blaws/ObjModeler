// Benjamin Laws
// CSE 40166 - Computer Graphics
// November 5, 2014
// Assignment 5: Object Loader and Renderer

#ifndef OBJPARSER_H
#define OBJPARSER_H

#include <vector>

bool loadOBJ( const char* path, std::vector<vec3>& out_verts, std::vector<vec2>& out_texcoords, std::vector<vec3>& out_normals, int& objSize, vec3 vertDisplacement = vec3( 0.0, 0.0, 0.0 ), vec3 vertScale = vec3( 1.0, 1.0, 1.0 ), vec3 vertRotate = vec3( 1.0, 1.0, 1.0 ) );
bool loadMTL( const char* path, vec3& k_ambient, vec3& k_diffuse, vec3& k_specular, float& shininess, GLuint& texture );
bool createTextureFromBMP( const char* path, GLuint& texture, GLuint texSlot = 0 );
void createDefaultTexture( GLuint& texture, GLuint texSlot = 0 );

#endif
