// Benjamin Laws
// CSE 40166 - Computer Graphics
// November 20, 2014
// Assignment 7: Custom 3D Scene

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "Angel.h"
#include "objParser.h"

bool loadOBJ( const char* path, std::vector<vec3>& out_verts, std::vector<vec2>& out_texcoords, std::vector<vec3>& out_normals, int& objSize, vec3 vertDisplacement /* = vec3( 0.0, 0.0, 0.0 ) */, vec3 vertScale /* = vec3( 1.0, 1.0, 1.0 ) */, vec3 vertRotate /* = vec3( 1.0, 1.0, 1.0 ) */ )
{
    // create temporary reference storage
    std::vector<unsigned int> vertexIndices, coordIndices, normalIndices;
    std::vector<vec3> temp_verts;
    std::vector<vec2> temp_texcoords;
    std::vector<vec3> temp_normals;
    
    // ignore elements already in the buffers
    int vertOffset = out_verts.size();
    
    // open input file
    FILE* file = fopen( path, "r" );
    if( !file )
    {
        std::cerr << "Cannot open " << path << ": " << strerror( errno ) << std::endl;
        return false;
    }

    // set up vertex transformation
    mat4 displacementMat = Translate( vertDisplacement ) * Scale( vertScale.x, vertScale.y, vertScale.z ) * RotateZ( vertRotate.z ) * RotateY( vertRotate.y ) * RotateX( vertRotate.x );
    
    // read file
    while( 1 )
    {
        char lineHeader[128];
        
        // get line type
        int res = fscanf( file, "%s", lineHeader );
        if( res == EOF )
        {
            break;
        }
        
        // vertex
        if( strcmp( lineHeader, "v" ) == 0 )
        {
            vec3 vertex;
            fscanf( file, "%f%*[ \t]%f%*[ \t]%f\n", &vertex.x, &vertex.y, &vertex.z );
            vec4 dispVertex = displacementMat * vec4( vertex, 1.0 );
            temp_verts.push_back( vec3( dispVertex.x, dispVertex.y, dispVertex.z ) );
        }
        
        // texture coordinate
        else if( strcmp( lineHeader, "vt" ) == 0 )
        {
            vec2 coord;
            fscanf( file, "%f%*[ \t]%f\n", &coord.x, &coord.y );
            temp_texcoords.push_back( coord );
        }
        
        // normal
        else if( strcmp( lineHeader, "vn" ) == 0 )
        {
            vec3 normal;
            fscanf( file, "%f%*[ \t]%f%*[ \t]%f\n", &normal.x, &normal.y, &normal.z );
            temp_normals.push_back( normal );
        }
        
        // face
        else if( strcmp( lineHeader, "f" ) == 0 )
        {
            char line[100];
            unsigned int vertexIndex[4] = {0};
            unsigned int coordIndex[4] = {0};
            unsigned int normalIndex[4] = {0};
            int matches = 0;
            
            fgets( line, 100, file );
            
            // square faces
            matches = sscanf( line, "%d/%d/%d%*[ \t]%d/%d/%d%*[ \t]%d/%d/%d%*[ \t]%d/%d/%d",
                              &vertexIndex[0], &coordIndex[0], &normalIndex[0],
                              &vertexIndex[1], &coordIndex[1], &normalIndex[1],
                              &vertexIndex[2], &coordIndex[2], &normalIndex[2],
                              &vertexIndex[3], &coordIndex[3], &normalIndex[3] );
            if( matches != 12 )
            {
                vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = vertexIndex[3] = 0;
                coordIndex[0] = coordIndex[1] = coordIndex[2] = coordIndex[3] = 0;
                normalIndex[0] = normalIndex[1] = normalIndex[2] = normalIndex[3] = 0;
                
                matches = sscanf( line, "%d//%d%*[ \t]%d//%d%*[ \t]%d//%d%*[ \t]%d//%d",
                                  &vertexIndex[0], &normalIndex[0],
                                  &vertexIndex[1], &normalIndex[1],
                                  &vertexIndex[2], &normalIndex[2],
                                  &vertexIndex[3], &normalIndex[3] );
                if( matches != 8 )
                {
                    vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = vertexIndex[3] = 0;
                    coordIndex[0] = coordIndex[1] = coordIndex[2] = coordIndex[3] = 0;
                    normalIndex[0] = normalIndex[1] = normalIndex[2] = normalIndex[3] = 0;
                    
                    matches = sscanf( line, "%d/%d%*[ \t]%d/%d%*[ \t]%d/%d%*[ \t]%d/%d",
                                      &vertexIndex[0], &coordIndex[0],
                                      &vertexIndex[1], &coordIndex[1],
                                      &vertexIndex[2], &coordIndex[2],
                                      &vertexIndex[3], &coordIndex[3] );
                    if( matches != 8 )
                    {
                        vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = vertexIndex[3] = 0;
                        coordIndex[0] = coordIndex[1] = coordIndex[2] = coordIndex[3] = 0;
                        normalIndex[0] = normalIndex[1] = normalIndex[2] = normalIndex[3] = 0;
                        
                        matches = sscanf( line, "%d%*[ \t]%d%*[ \t]%d%*[ \t]%d",
                                          &vertexIndex[0], &vertexIndex[1], &vertexIndex[2], &vertexIndex[3] );
            
            // triangular faces
                        if( matches != 4 )
                        {
                            vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = vertexIndex[3] = 0;
                            coordIndex[0] = coordIndex[1] = coordIndex[2] = coordIndex[3] = 0;
                            normalIndex[0] = normalIndex[1] = normalIndex[2] = normalIndex[3] = 0;
                            
                            matches = sscanf( line, "%d/%d/%d%*[ \t]%d/%d/%d%*[ \t]%d/%d/%d",
                                              &vertexIndex[0], &coordIndex[0], &normalIndex[0],
                                              &vertexIndex[1], &coordIndex[1], &normalIndex[1],
                                              &vertexIndex[2], &coordIndex[2], &normalIndex[2] );
                            if( matches != 9 )
                            {
                                vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = vertexIndex[3] = 0;
                                coordIndex[0] = coordIndex[1] = coordIndex[2] = coordIndex[3] = 0;
                                normalIndex[0] = normalIndex[1] = normalIndex[2] = normalIndex[3] = 0;
                                
                                matches = sscanf( line, "%d//%d%*[ \t]%d//%d%*[ \t]%d//%d",
                                                  &vertexIndex[0], &normalIndex[0],
                                                  &vertexIndex[1], &normalIndex[1],
                                                  &vertexIndex[2], &normalIndex[2] );
                                if( matches != 6 )
                                {
                                    vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = vertexIndex[3] = 0;
                                    coordIndex[0] = coordIndex[1] = coordIndex[2] = coordIndex[3] = 0;
                                    normalIndex[0] = normalIndex[1] = normalIndex[2] = normalIndex[3] = 0;
                                    
                                    matches = sscanf( line, "%d/%d%*[ \t]%d/%d%*[ \t]%d/%d",
                                                      &vertexIndex[0], &coordIndex[0],
                                                      &vertexIndex[1], &coordIndex[1],
                                                      &vertexIndex[2], &coordIndex[2] );
                                    if( matches != 6 )
                                    {
                                        vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = vertexIndex[3] = 0;
                                        coordIndex[0] = coordIndex[1] = coordIndex[2] = coordIndex[3] = 0;
                                        normalIndex[0] = normalIndex[1] = normalIndex[2] = normalIndex[3] = 0;
                                        
                                        matches = sscanf( line, "%d%*[ \t]%d%*[ \t]%d",
                                                          &vertexIndex[0], &vertexIndex[1], &vertexIndex[2] );

                                        if( matches != 3 )
                                        {
                                            std::cerr << "Unable to parse " << path << " into faces correctly." << std::endl;
                                            return false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        
            // add triangle to lists
            if( vertexIndex[0] && vertexIndex[1] && vertexIndex[2] )
            {
                vertexIndices.push_back( vertexIndex[0] );
                vertexIndices.push_back( vertexIndex[1] );
                vertexIndices.push_back( vertexIndex[2] );
                if( coordIndex[0] && coordIndex[1] && coordIndex[2] )
                {
                    coordIndices.push_back( coordIndex[0] );
                    coordIndices.push_back( coordIndex[1] );
                    coordIndices.push_back( coordIndex[2] );
                }
                if( normalIndex[0] && normalIndex[1] && normalIndex[2] )
                {
                    normalIndices.push_back( normalIndex[0] );
                    normalIndices.push_back( normalIndex[1] );
                    normalIndices.push_back( normalIndex[2] );
                }
            }
            
            // square faces only: add second triangle
            if( vertexIndex[3] && vertexIndex[2] && vertexIndex[0] )
            {
                vertexIndices.push_back( vertexIndex[2] );
                vertexIndices.push_back( vertexIndex[3] );
                vertexIndices.push_back( vertexIndex[0] );
                if( coordIndex[3] && coordIndex[2] && coordIndex[0] )
                {
                    coordIndices.push_back( coordIndex[2] );
                    coordIndices.push_back( coordIndex[3] );
                    coordIndices.push_back( coordIndex[0] );
                }
                if( normalIndex[3] && normalIndex[2] && normalIndex[0] )
                {
                    normalIndices.push_back( normalIndex[2] );
                    normalIndices.push_back( normalIndex[3] );
                    normalIndices.push_back( normalIndex[0] );
                }
            }
        }
    }
    
    // process data into output lists
    for( unsigned int i = 0; i < vertexIndices.size(); ++i )
    {
        out_verts.push_back( temp_verts[ vertexIndices[i] - 1 ] );
    }
    objSize = out_verts.size() - vertOffset;
    for( unsigned int i = 0; i < coordIndices.size(); ++i )
    {
        out_texcoords.push_back( temp_texcoords[ coordIndices[i] - 1 ] );
    }
    for( unsigned int i = 0; i < normalIndices.size(); ++i )
    {
        out_normals.push_back( temp_normals[ normalIndices[i] - 1 ] );
    }
    
    // return
    return true;
}

bool loadMTL( const char* path, vec3& k_ambient, vec3& k_diffuse, vec3& k_specular, float& shininess, GLuint& texture )
{
    // open source file
    std::ifstream inFile( path );
    if( !inFile )
    {
        std::cerr << "Cannot open " << path << ": " << strerror( errno ) << std::endl;
        return false;
    }
    
    // read data
    while( !inFile.eof() )
    {
        char line[100];
        inFile >> line;

        // ambient
        if( strcmp( line, "Ka" ) == 0 )
        {
            inFile >> line;
            k_ambient.x = atof( line );
            inFile >> line;
            k_ambient.y = atof( line );
            inFile >> line;
            k_ambient.z = atof( line );
        }

        // diffuse
        else if( strcmp( line, "Kd" ) == 0 )
        {
            inFile >> line;
            k_diffuse.x = atof( line );
            inFile >> line;
            k_diffuse.y = atof( line );
            inFile >> line;
            k_diffuse.z = atof( line );
        }

        // specular
        else if( strcmp( line, "Ks" ) == 0 )
        {
            inFile >> line;
            k_specular.x = atof( line );
            inFile >> line;
            k_specular.y = atof( line );
            inFile >> line;
            k_specular.z = atof( line );
        }
        
        // shininess
        else if( strcmp( line, "Ns" ) == 0 )
        {
            inFile >> line;
            shininess = atof( line );
        }

        // texture (must be in current directory)
        else if( strcmp( line, "map_Kd" ) == 0 )
        {
            inFile >> line;
            createTextureFromBMP( line, texture );
        }
    }
    
    if( !texture )
    {
        createDefaultTexture( texture );
    }
    
    return true;
}

bool createTextureFromBMP( const char* path, GLuint& texture, GLuint texSlot /* = 0 */ )
{
    // open texture file
    std::ifstream inFile( path );
    if( !inFile )
    {
        std::cerr << "Cannot open " << path << ": " << strerror( errno ) << std::endl;
        return false;
    }
    
    // get size of image and discard header
    int width, height;
    inFile.ignore( 18 );
    inFile.read( (char*)&width, 4 );
    inFile.read( (char*)&height, 4 );
    inFile.ignore( 25 );
    
    // flip data if necessary
    int yStart = 0;
    int yInc = 1;
    if( height < 0 )
    {
        height *= -1;
        yStart = height - 1;
        yInc = -1;
    }
    
    // read data (BMP stores pixels in BGR format)
    GLubyte* image = new GLubyte[ width * height * 3 ];
    GLubyte* ptr = image + 2;
    for( int j = yStart; j > -1 && j < height; j += yInc )
    {
        int i;
        for( i = 0; i < width; ++i )
        {
            for( int k = 0; k < 3; ++k )
            {
                inFile.read( (char*)ptr, 1 );
                --ptr;
            }
            ptr += 6;
        }
        
        // remove padding bytes
        if( i % 4 )
        {
            inFile.ignore( 4 - ( i % 4 ) );
        }
    }
    
    // create texture
    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );
    glActiveTexture( GL_TEXTURE0 + texSlot );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image );

    return true;
}

void createDefaultTexture( GLuint& texture, GLuint texSlot /* = 0 */ )
{
    GLubyte image[] = { 255, 255, 255 };

    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );
    glActiveTexture( GL_TEXTURE0 );
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, image );
}
