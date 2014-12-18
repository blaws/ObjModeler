// Benjamin Laws
// CSE 40166 - Computer Graphics
// December 17, 2014
// Final Project

#include <unistd.h>
#include <cassert>
#include <vector>
#include <string>
#include "Angel.h"
#include "trackball.h"
#include "tiny_obj_loader.h"
#include "SOIL.h"

/////////////////////////////////////////////////////////////////

// window
int windowW( 750 );
int windowH( 750 );

// framerate
const float fps( 30.0 );
const float spf( 1.0 / fps );
const float mspf( 1000.0 * spf );

// filenames
std::string objFilePath;

// original file data
std::vector<tinyobj::material_t> materials;
std::vector<tinyobj::shape_t> shapes;

// data
std::vector<vec3> vert;
std::vector<vec3> prevVert;
std::vector<vec2> texcoord;
std::vector<vec3> normal;
std::vector<GLuint> indices;

// vertex connections
std::vector<std::vector<float> > tightConnections;
std::vector<std::vector<float> > midConnections;
std::vector<std::vector<float> > looseConnections;
std::vector<std::vector<float> > *connections( &looseConnections );
std::vector<bool> deletedVertices;
std::vector<vec3> pinnedVertices;

// textures
GLuint defaultTexture;
std::vector<GLuint> objTexture;

// floor
const vec3 floorSize( 10.0, 10.0, 0.0 );
const int numFloorVert( 6 );
tinyobj::material_t defaultMaterial;

// view
float curQuaternion[4];
float radius( 20.0 );
vec3 lookAt( 0.0, 0.0, 1.0 );
vec3 eye( 0.0, -radius, lookAt.z );
const vec3 up( 0.0, 0.0, 1.0 );

// matrices
mat4 projMatrix;
mat4 rotMatrix;
mat4 forceRotMatrix;
mat4 mvMatrix;

// interaction
int mouseX;
int mouseY;
const float maxClickDistance( 0.1 );
bool allVertexMode( false );

// object modifiers
float objSize( 1.0 );
vec3 objTrans( 0.0, 0.0, 0.0 );
vec3 objRot( 0.0, 0.0, 0.0 );

// options
vec3 grav( 0.0, 0.0, -4.0 );
const int maxGrav( 6 );
float clickForce( 25.0 );
int numConstraintReps( 1 );
bool dispWireframe( false );
bool connectTightly( connections == &tightConnections );
bool frictionOn( true );
bool deletionMode( false );
bool pinMode( false );
vec3 windForce;
int windSpeed( 0 );
const int maxWindSpeed( 5 );

// shader handles
GLuint sProgram;
GLint mvMatrixLoc;
GLint vertLoc;
GLint texcoordLoc;
GLint normalLoc;
GLint textureLoc;

// shader handles (material)
GLint ambientLoc;
GLint diffuseLoc;
GLint specularLoc;
GLint shininessLoc;

// buffers
GLuint sceneVAO;
GLuint objectBuffer;
GLuint indexBuffer;

/////////////////////////////////////////////////////////////////

void setView()
{
    eye.y = -radius;
    eye.z = lookAt.z;
    
    projMatrix = Perspective( 45.0, 1.0, 0.1, std::fmax( 100.0, 2.0 * radius ) );
    mvMatrix = LookAt( eye, lookAt, up ) * rotMatrix;
}

void init()
{
    // settings
    srand( time( NULL ) );
    glClearColor( 1.0, 1.0, 1.0, 1.0 );
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glEnable( GL_DEPTH_TEST );
    glPointSize( 3 );

    // initial data
    trackball( curQuaternion, 0, 0, 0, 0 );
    setView();
    
    // buffer objects
    glGenVertexArraysAPPLE( 1, &sceneVAO );
    glBindVertexArrayAPPLE( sceneVAO );
    glGenBuffers( 1, &objectBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, objectBuffer );
    glGenBuffers( 1, &indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );

    // shader program handles
    sProgram = InitShader( "vert.glsl", "frag.glsl" );
    mvMatrixLoc = glGetUniformLocation( sProgram, "u_mv_Matrix" );
    vertLoc = glGetAttribLocation( sProgram, "a_vPosition" );
    texcoordLoc = glGetAttribLocation( sProgram, "a_vTexCoord" );
    normalLoc = glGetAttribLocation( sProgram, "a_vNormal" );
    textureLoc = glGetUniformLocation( sProgram, "u_sTexture" );
    ambientLoc = glGetUniformLocation( sProgram, "u_kAmbient" );
    diffuseLoc = glGetUniformLocation( sProgram, "u_kDiffuse" );
    specularLoc = glGetUniformLocation( sProgram, "u_kSpecular" );
    shininessLoc = glGetUniformLocation( sProgram, "u_fShininess" );

    // enable vertex attributes
    glEnableVertexAttribArray( vertLoc );
    glEnableVertexAttribArray( texcoordLoc );
    glEnableVertexAttribArray( normalLoc );
    
    // texture
    glActiveTexture( GL_TEXTURE0 );
    glUniform1i( textureLoc, 0 );
}

inline float distance( const vec3 x, const vec3 y )
{
    vec3 d = x - y;
    return sqrt( d.x * d.x + d.y * d.y + d.z * d.z );
}

void connectVertices()
{
    // reset
    tightConnections.clear();
    tightConnections.resize( vert.size(), std::vector<float>( vert.size(), -1 ) );
    looseConnections.clear();
    looseConnections.resize( vert.size(), std::vector<float>() );
    midConnections.clear();
    midConnections.resize( vert.size(), std::vector<float>() );
    
    int ni = 0;
    int nj;
    
    // tightly connected
    for( std::vector<vec3>::iterator vi = vert.begin(); vi != vert.end(); ++vi, ++ni )
    {
        nj = ni;
        for( std::vector<vec3>::iterator vj = vi; vj != vert.end(); ++vj, ++nj )
        {
            tightConnections[ni][nj] = tightConnections[nj][ni] = distance( *vi, *vj );
        }
    }

    // loosely connected
    // duplicate vertices
    ni = 0;
    for( std::vector<vec3>::iterator vi = vert.begin(); vi != vert.end(); ++vi, ++ni )
    {
        nj = ni;
        for( std::vector<vec3>::iterator vj = vi; vj != vert.end(); ++vj, ++nj )
        {
            if( ni != nj )
            {
                float d = distance( *vi, *vj );
                if( d < 0.1 )
                {
                    looseConnections[ni].push_back( nj );
                    looseConnections[ni].push_back( d );
                    looseConnections[nj].push_back( ni );
                    looseConnections[nj].push_back( d );
                }
            }
        }
    }
    // connected vertices
    for( std::vector<GLuint>::iterator i = indices.begin(); i != indices.end(); ++i )
    {
        GLuint a = *i;
        GLuint b = *(++i);
        GLuint c = *(++i);
        
        float d1 = tightConnections[a][b];
        float d2 = tightConnections[b][c];
        float d3 = tightConnections[a][c];
        
        looseConnections[a].push_back( b );
        looseConnections[a].push_back( d1 );
        looseConnections[b].push_back( a );
        looseConnections[b].push_back( d1 );

        looseConnections[b].push_back( c );
        looseConnections[b].push_back( d2 );
        looseConnections[c].push_back( b );
        looseConnections[c].push_back( d2 );

        looseConnections[a].push_back( c );
        looseConnections[a].push_back( d3 );
        looseConnections[c].push_back( a );
        looseConnections[c].push_back( d3 );
    }
    
    // medium connected: each vertex is connected to the vertices connected to its loose connections, also
    midConnections = looseConnections;
    ni = 0;
    for( std::vector<std::vector<float> >::iterator i = looseConnections.begin(); i != looseConnections.end(); ++i, ++ni )
    {
        for( std::vector<float>::iterator j = i->begin(); j != i->end(); j += 2 )
        {
            int curConn = *j;
            for( std::vector<float>::iterator k = looseConnections[curConn].begin(); k != looseConnections[curConn].end(); k += 2 )
            {
                midConnections[ni].push_back( curConn );
                midConnections[ni].push_back( tightConnections[ni][curConn] );
            }
        }
    }
}

void pinVertex( const int v )
{
    pinnedVertices[v] = vert[v];
    std::cout << "Vertex " << v << " pinned" << std::endl;
}

void deleteVertex( const int v )
{
    // don't delete v from vert, because that would change indices;
    // instead, remove every triangle involving v from indices
    
    // indices
    assert( indices.size() % 3 == 0 );
    for( std::vector<GLuint>::iterator i = indices.begin(); i != indices.end(); )
    {
        if( *i == v || *(i+1) == v || *(i+2) == v )
        {
            indices.erase( i, i+3 );
        }
        else
        {
            i += 3;
        }
    }
    
    // keep track for Verlet constraints
    deletedVertices[v] = true;
    vert[v] = floorSize;  // so that the dot is less obvious in wireframe mode
    
    std::cout << "Vertex " << v << " deleted." << std::endl;
}

void calculateMeshNormals()
{
    int ni = 0;
    for( std::vector<std::vector<float> >::iterator vi = looseConnections.begin(); vi != looseConnections.end(); ++vi, ++ni )
    {
        int nj = 0;
        
        // get connecting edges at each point
        std::vector<vec3> diffs;
        for( std::vector<float>::iterator vj = vi->begin(); vj != vi->end(); ++vj, ++nj )
        {
            if( *vj > 0.0 )
            {
                diffs.push_back( vert[nj] - vert[ni] );
            }
        }
        
        // average cross products of pairwise diff vectors
        vec3 avg;
        for( int a = 0; a < diffs.size(); ++a )
        {
            for( int b = a + 1; b < diffs.size(); ++b )
            {
                avg += cross( diffs[a], diffs[b] );
            }
        }
        avg /= diffs.size() * ( diffs.size() - 1 ) * 0.5;
        
        // set normal
        normal[ni] = avg;
    }
}

inline void bindMaterial( const int idx )
{
    tinyobj::material_t* mat;

    if( idx < 0 )
    {
        mat = &defaultMaterial;
        glBindTexture( GL_TEXTURE_2D, defaultTexture );
    }
    else
    {
        mat = &materials[idx];
        glBindTexture( GL_TEXTURE_2D, objTexture[idx] );
    }
    
    glUniform3fv( ambientLoc, 1, mat->ambient );
    glUniform3fv( diffuseLoc, 1, mat->diffuse );
    glUniform3fv( specularLoc, 1, mat->specular );
    glUniform1f( shininessLoc, mat->shininess );
}

inline void moveInsideVolume( vec3& p, vec3& pPrev )
{
    // the random variance is to prevent objects from becoming
    // "stuck" with all their vertices in the exact same plane
    
    if( p.z < floorSize.z )
    {
        p.z = floorSize.z + rand()%2 * 0.01;
        if( frictionOn )
        {
            pPrev = p;
        }
    }
    if( p.x > floorSize.x )
    {
        p.x = floorSize.x - rand()%2 * 0.01;
        if( frictionOn )
        {
            pPrev = p;
        }
    }
    else if( p.x < -floorSize.x )
    {
        p.x = -floorSize.x + rand()%2 * 0.01;
        if( frictionOn )
        {
            pPrev = p;
        }
    }
    if( p.y > floorSize.y )
    {
        p.y = floorSize.y - rand()%2 * 0.01;
        if( frictionOn )
        {
            pPrev = p;
        }
    }
    else if( p.y < -floorSize.y )
    {
        p.y = -floorSize.y + rand()%2 * 0.01;
        if( frictionOn )
        {
            pPrev = p;
        }
    }
}

void printObjInfo()
{
    std::cout << "# of shapes    : " << shapes.size() << std::endl;
    std::cout << "# of materials : " << materials.size() << std::endl;
    
    for (size_t i = 0; i < shapes.size(); i++) {
        printf("shape[%ld].name = %s\n", i, shapes[i].name.c_str());
        printf("Size of shape[%ld].indices: %ld\n", i, shapes[i].mesh.indices.size());
        printf("Size of shape[%ld].material_ids: %ld\n", i, shapes[i].mesh.material_ids.size());
        printf("shape[%ld].vertices: %ld\n", i, shapes[i].mesh.positions.size() / 3);
        assert((shapes[i].mesh.positions.size() % 3) == 0);
        for (size_t v = 0; v < shapes[i].mesh.positions.size(); v+=3) {
            printf("  v[%ld] = (%f, %f, %f)\n", v,
                   shapes[i].mesh.positions[3*v+0],
                   shapes[i].mesh.positions[3*v+1],
                   shapes[i].mesh.positions[3*v+2]);
        }
    }
    
    for (size_t i = 0; i < shapes.size(); i++) {
        assert((shapes[i].mesh.indices.size() % 3) == 0);
        for (size_t f = 0; f < shapes[i].mesh.indices.size(); f+=3) {
            printf("  idx[%ld] = %d, %d, %d. mat_id = %d\n", f, shapes[i].mesh.indices[f], shapes[i].mesh.indices[f+1], shapes[i].mesh.indices[f+2], shapes[i].mesh.material_ids[f]);
        }
    }
    
    for (size_t i = 0; i < shapes.size(); i++) {
        printf("shape[%ld].texcoords: %ld\n", i, shapes[i].mesh.texcoords.size() / 3);
        assert((shapes[i].mesh.texcoords.size() % 2) == 0);
        for (size_t v = 0; v < shapes[i].mesh.texcoords.size(); v+=2) {
            printf("  v[%ld] = (%f, %f)\n", v,
                   shapes[i].mesh.texcoords[3*v+0],
                   shapes[i].mesh.texcoords[3*v+1]);
        }
    }
    
    for (size_t i = 0; i < shapes.size(); i++) {
        printf("shape[%ld].normals: %ld\n", i, shapes[i].mesh.normals.size() / 3);
        assert((shapes[i].mesh.normals.size() % 3) == 0);
        for (size_t v = 0; v < shapes[i].mesh.normals.size(); v+=3) {
            printf("  v[%ld] = (%f, %f, %f)\n", v,
                   shapes[i].mesh.normals[3*v+0],
                   shapes[i].mesh.normals[3*v+1],
                   shapes[i].mesh.normals[3*v+2]);
        }
    }
    
    for (size_t i = 0; i < materials.size(); i++) {
        printf("material[%ld].name = %s\n", i, materials[i].name.c_str());
        printf("  material.Ka = (%f, %f ,%f)\n", materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
        printf("  material.Kd = (%f, %f ,%f)\n", materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
        printf("  material.Ks = (%f, %f ,%f)\n", materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
        printf("  material.Tr = (%f, %f ,%f)\n", materials[i].transmittance[0], materials[i].transmittance[1], materials[i].transmittance[2]);
        printf("  material.Ke = (%f, %f ,%f)\n", materials[i].emission[0], materials[i].emission[1], materials[i].emission[2]);
        printf("  material.Ns = %f\n", materials[i].shininess);
        printf("  material.Ni = %f\n", materials[i].ior);
        printf("  material.dissolve = %f\n", materials[i].dissolve);
        printf("  material.illum = %d\n", materials[i].illum);
        printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
        printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
        printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
        printf("  material.map_Ns = %s\n", materials[i].normal_texname.c_str());
        std::map<std::string, std::string>::const_iterator it(materials[i].unknown_parameter.begin());
        std::map<std::string, std::string>::const_iterator itEnd(materials[i].unknown_parameter.end());
        for (; it != itEnd; it++) {
            printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
        }
        printf("\n");
    }
}

void resetObj()
{
    // clear structures
    vert.clear();
    texcoord.clear();
    normal.clear();
    indices.clear();
    
    // vertices
    mat4 mRotate = RotateZ( objRot.z ) * RotateY( objRot.y ) * RotateX( objRot.x );
    for (size_t i = 0; i < shapes.size(); i++) {
        assert((shapes[i].mesh.positions.size() % 3) == 0);
        for (size_t v = 0; v < shapes[i].mesh.positions.size(); v+=3) {
            vec4 p = mRotate * vec4( shapes[i].mesh.positions[v] * objSize,
                                     shapes[i].mesh.positions[v+1] * objSize,
                                     shapes[i].mesh.positions[v+2] * objSize, 1.0 );
            vert.push_back( vec3( p.x, p.y, p.z ) + objTrans );
        }
    }
    
    // indices
    for (size_t i = 0; i < shapes.size(); i++) {
        assert((shapes[i].mesh.indices.size() % 3) == 0);
        for (size_t f = 0; f < shapes[i].mesh.indices.size(); f+=3) {
            int a = shapes[i].mesh.indices[f];
            int b = shapes[i].mesh.indices[f+1];
            int c = shapes[i].mesh.indices[f+2];
            
            indices.push_back( a );
            indices.push_back( b );
            indices.push_back( c );
        }
    }
    
    // texture coordinates
    for (size_t i = 0; i < shapes.size(); i++) {
        assert((shapes[i].mesh.texcoords.size() % 2) == 0);
        for (size_t v = 0; v < shapes[i].mesh.texcoords.size(); v+=2) {
            texcoord.push_back( vec2( shapes[i].mesh.texcoords[v],
                                      shapes[i].mesh.texcoords[v+1] ) );
        }
    }
    
    // normals
    for (size_t i = 0; i < shapes.size(); i++) {
        assert((shapes[i].mesh.normals.size() % 3) == 0);
        for (size_t v = 0; v < shapes[i].mesh.normals.size(); v+=3) {
            normal.push_back( vec3( shapes[i].mesh.normals[v],
                                    shapes[i].mesh.normals[v+1],
                                    shapes[i].mesh.normals[v+2] ) );
        }
    }

    // re-initialize supporting structures
    deletedVertices.clear();
    deletedVertices.resize( vert.size(), false );
    pinnedVertices.clear();
    pinnedVertices.resize( vert.size(), vec3() );
    prevVert = vert;
}

void loadObjFile( const char* path )
{
    std::string errorStr = tinyobj::LoadObj( shapes, materials, path );
    
    if( !errorStr.empty() )
    {
        std::cerr << errorStr << std::endl;
        exit( EXIT_FAILURE );
    }
    
    resetObj();
    connectVertices();
}

void setupFloor()
{
    // data
    for( int j = -1; j < 2; j += 2 )
    {
        for( int i = -1; i < 2; i += 2 )
        {
            vert.push_back( vec3( i * floorSize.x, j * floorSize.y, floorSize.z ) );
            normal.push_back( vec3( 1.0, 1.0, 1.0 ) );
            indices.push_back( vert.size() - 1 );
        }
    }
    indices.push_back( vert.size() - 2 );
    indices.push_back( vert.size() - 3 );
    
    // material
    for( int i = 0; i < 3; ++i )
    {
        defaultMaterial.ambient[i] = 0.3;
        defaultMaterial.diffuse[i] = 0.7;
        defaultMaterial.specular[i] = 0.5;
    }
    defaultMaterial.shininess = 10.0;
}

// texture coordinates and normals don't change each frame,
// so only vertices and indices need to be resent every time
void bufferNewData()
{
    // index buffer
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof( GLuint ), indices.data(), GL_DYNAMIC_DRAW );

    // vertices
    glBufferSubData( GL_ARRAY_BUFFER, 0, vert.size() * sizeof( vec3 ), vert.data() );
    glEnableVertexAttribArray( vertLoc );
    glVertexAttribPointer( vertLoc, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( 0 ) );
}

void bufferData()
{
    texcoord.resize( vert.size() );
    normal.resize( vert.size() );
    
    // vertex array object
    glBindVertexArrayAPPLE( sceneVAO );
    
    // object buffer
    glBindBuffer( GL_ARRAY_BUFFER, objectBuffer );
    glBufferData( GL_ARRAY_BUFFER, ( vert.size() + normal.size() ) * sizeof( vec3 ) + texcoord.size() * sizeof( vec2 ), NULL, GL_DYNAMIC_DRAW );
    
    // indices and vertices
    bufferNewData();
    
    // texture coordinates
    glBufferSubData( GL_ARRAY_BUFFER, vert.size() * sizeof( vec3 ), texcoord.size() * sizeof( vec2 ), texcoord.data() );
    glEnableVertexAttribArray( texcoordLoc );
    glVertexAttribPointer( texcoordLoc, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( vert.size() * sizeof( vec3 ) ) );
    
    // normals
    glBufferSubData( GL_ARRAY_BUFFER, vert.size() * sizeof( vec3 ) + texcoord.size() * sizeof( vec2 ), normal.size() * sizeof( vec3 ), normal.data() );
    glEnableVertexAttribArray( normalLoc );
    glVertexAttribPointer( normalLoc, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( vert.size() * sizeof( vec3 ) + texcoord.size() * sizeof( vec2 ) ) );
}

/////////////////////////////////////////////////////////////////

std::vector<int> getPointsFromClick( int x, int y )
{
    // this attempts to use the current transformation matrices
    // to tell where each vertex is located on the screen at the
    // moment and find the one that is nearest to the given click location
    
    mat4 projMvMatrix = projMatrix * mvMatrix;
    vec3 pClick( x / (float)windowW * 2.0 - 1.0, ( windowH - y ) / (float)windowH * 2.0 - 1.0, 0.0 );
    float minD( maxClickDistance );
    std::vector<int> minI;
    
    for( int i = 0; i < vert.size() - numFloorVert; ++i )
    {
        vec4 p( projMvMatrix * vec4( vert[i], 1.0 ) );
        float d( distance( pClick, vec3( p.x / p.w, p.y / p.w, 0.0 ) ) );

        if( !deletedVertices[i] && d <= minD )
        {
            if( d < minD )
            {
                minI.clear();
            }
            minD = d;
            minI.push_back( i );
        }
    }
    return minI;
}

void mouse( int button, int state, int x, int y )
{
    if( state == GLUT_DOWN )
    {
        switch( button )
        {
            case GLUT_LEFT_BUTTON:
                mouseX = x;
                mouseY = y;
                break;
            case GLUT_RIGHT_BUTTON:
                std::vector<int> vClick = getPointsFromClick( x, y );
                if( !vClick.empty() )
                {
                    if( pinMode )
                    {
                        for( int i = 0; i < vClick.size(); ++i )
                        {
                            pinVertex( vClick[i] );
                        }
                        pinMode = false;
                        std::cout << "pin mode OFF" << std::endl;
                    }
                    else if( deletionMode )
                    {
                        for( int i = 0; i < vClick.size(); ++i )
                        {
                            deleteVertex( vClick[i] );
                        }
                        deletionMode = false;
                        std::cout << "Deletion mode OFF" << std::endl;
                    }
                    else
                    {
                        if( allVertexMode )
                        {
                            vec4 mMove = forceRotMatrix * vec4( 0.0, -clickForce / vert.size(), 0.0, 1.0 );
                            for( int i = 0; i < vert.size(); ++i )
                            {
                                prevVert[i] += vec3( mMove.x, mMove.y, mMove.z );
                            }
                        }
                        else
                        {
                            vec4 mMove = forceRotMatrix * vec4( 0.0, -clickForce, 0.0, 1.0 );
                            for( int i = 0; i < vClick.size(); ++i )
                            {
                                prevVert[vClick[i]] += vec3( mMove.x, mMove.y, mMove.z );
                            }
                        }
                    }
                }
                break;
        }
    }
}

void mouseMove( int x, int y )
{
    if ( mouseX != x || mouseY != y )
    {
        float lastQuaternion[4];
        
        // find rotation from last position
        trackball( lastQuaternion,
                   ( windowW - 2.0 * mouseX ) / windowW,
                   ( 2.0 * mouseY - windowH ) / windowH,
                   ( windowW - 2.0 * x ) / windowW,
                   ( 2.0 * y - windowH ) / windowH );
        
        // add to current quaternion
        add_quats( lastQuaternion, curQuaternion, curQuaternion );
        
        // get rotation matrices (one for camera, the other for user forces)
        float revQuaternion[] = { curQuaternion[0], curQuaternion[1], curQuaternion[2], -curQuaternion[3] };
        float m[4][4], m2[4][4];
        build_rotmatrix( m, curQuaternion );
        build_rotmatrix( m2, revQuaternion );
        for( int i = 0; i < 4; ++i )
        {
            for(int j = 0; j < 4; ++j )
            {
                rotMatrix[i][j] = m[j][i];      // column-major / row-major swap
                forceRotMatrix[i][j] = m2[j][i];
            }
        }
        
        // update state
        mouseX = x;
        mouseY = y;
        setView();
        glutPostRedisplay();
    }
}

void animate( int s );

void keyboard( unsigned char key, int x, int y )
{
    vec4 tmp;
    switch( key )
    {
        // zoom
        case 'o':
        case 'O':
            radius *= 2.0;
            break;
        case 'i':
        case 'I':
            radius *= 0.5;
            break;
            
        // click interaction
        case '=':
        case '+':
            clickForce *= 2.0;
            std::cout << "Click force: " << clickForce << std::endl;
            break;
        case '-':
        case '_':
            clickForce *= 0.5;
            std::cout << "Click force: " << clickForce << std::endl;
            break;
            
        // constraint strength
        case ',':
        case '<':
            numConstraintReps = std::max( numConstraintReps - 1, 0 );
            std::cout << "Verlet repetitions: " << numConstraintReps << std::endl;
            break;
        case '.':
        case '>':
            ++numConstraintReps;
            std::cout << "Verlet repetitions: " << numConstraintReps << std::endl;
            break;
            
        // connection amount
        case '1':
            connectTightly = false;
            connections = &looseConnections;
            std::cout << "Connection strength 1" << std::endl;
            break;
        case '2':
            connectTightly = false;
            connections = &midConnections;
            std::cout << "Connection strength 2" << std::endl;
            break;
        case '3':
            connectTightly = true;
            connections = &tightConnections;
            std::cout << "Connection strength 3" << std::endl;
            break;
        case '0':
            connectTightly = false;
            connections = NULL;
            std::cout << "Connection strength 0" << std::endl;
            break;
            
        // friction
        case 'f':
        case 'F':
            frictionOn = !frictionOn;
            std::cout << "Friction " << ( frictionOn ? "ON" : "OFF" ) << std::endl;
            break;
            
        // wind
        case 'w':
        case 'W':
            windSpeed = ( windSpeed + 1 ) % ( maxWindSpeed + 1 );
            tmp = forceRotMatrix * vec4( 0.0, windSpeed, 0.0, 1.0 );
            windForce = vec3( tmp.x / tmp.w, tmp.y / tmp.w, tmp.z / tmp.w );
            std::cout << "Windspeed " << windSpeed << std::endl;
            break;
            
        // gravity
        case 'g':
        case 'G':
            grav.z = -( ( -(int)grav.z + 1 ) % ( maxGrav + 1 ) );
            std::cout << "Gravity strength " << ( grav.z ? -grav.z : grav.z ) << std::endl;
            break;
            
        // click force mode
        case 'c':
        case 'C':
            allVertexMode = !allVertexMode;
            std::cout << "Click forces applied at " << ( allVertexMode ? "ALL VERTICES" : "NEAREST VERTEX" ) << std::endl;
            break;
            
        // "pin" mode
        case 'p':
        case 'P':
            pinMode = !pinMode;
            if( pinMode && deletionMode )
            {
                deletionMode = false;
                std::cout << "Deletion mode " << ( deletionMode ? "ON" : "OFF" ) << std::endl;
            }
            std::cout << "Pin mode " << ( pinMode ? "ON" : "OFF" ) << std::endl;
            break;
        case 'u':
        case 'U':
            pinnedVertices.clear();
            pinnedVertices.resize( vert.size(), vec3() );
            std::cout << "All vertices unpinned" << std::endl;
            break;
            
        // deletion mode
        case 'd':
        case 'D':
            deletionMode = !deletionMode;
            if( pinMode && deletionMode )
            {
                pinMode = false;
                std::cout << "Pin mode " << ( pinMode ? "ON" : "OFF" ) << std::endl;
            }
            std::cout << "Deletion mode " << ( deletionMode ? "ON" : "OFF" ) << std::endl;
            break;
            
        // reset
        case 'r':
        case 'R':
            resetObj();
            setupFloor();
            break;
            
        // display mode
        case 13:  // ENTER
            dispWireframe = !dispWireframe;
            glPolygonMode( GL_FRONT_AND_BACK, dispWireframe ? GL_LINE : GL_FILL );
            std::cout << "Wireframe " << ( dispWireframe ? "ON" : "OFF" ) << std::endl;
            break;
    }
    
    setView();
    glutPostRedisplay();
}

void keyboardSpecials( int key, int x, int y )
{
    switch( key )
    {
        case GLUT_KEY_UP:
            lookAt.z += 0.5;
            break;
        case GLUT_KEY_DOWN:
            lookAt.z -= 0.5;
            break;
    }
    
    setView();
    glutPostRedisplay();
}

void reshape( int w, int h )
{
    windowW = w;
    windowH = h;
    
    glViewport( 0, 0, w, h );
}

void display()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    // object
    glUniformMatrix4fv( mvMatrixLoc, 1, GL_TRUE, projMatrix * mvMatrix );
    glBindVertexArrayAPPLE( sceneVAO );
    if( !materials.empty() )
    {
        bindMaterial( 0 );
    }
    else
    {
        bindMaterial( -1 );
    }
    glDrawElements( GL_TRIANGLES, indices.size() - numFloorVert, GL_UNSIGNED_INT, BUFFER_OFFSET( 0 ) );
    if( dispWireframe )
    {
        glDrawArrays( GL_POINTS, 0, vert.size() - numFloorVert );
    }
    
    // floor
    bindMaterial( -1 );
    glDrawElements( GL_TRIANGLES, numFloorVert, GL_UNSIGNED_INT, BUFFER_OFFSET( ( indices.size() - numFloorVert ) * sizeof( GLuint ) ) );
    
    glutSwapBuffers();
}

inline void stickConstraint( vec3& x1, vec3& x2, float d )
{
    vec3 delta = x2 - x1;
    float deltaLength = sqrt( dot( delta, delta ) );
    vec3 diff = deltaLength ? ( deltaLength - d ) / deltaLength : vec3( 0.0 );
    x1 += delta * 0.5 * diff;
    x2 -= delta * 0.5 * diff;
}

void animate( int s )
{
    // Verlet integration: velocity and acceleration (gravity)
    vec3 accel = ( grav + windForce ) * spf * spf;
    for( std::vector<vec3>::iterator i = vert.begin(), j = prevVert.begin(); i != vert.end() && j != prevVert.end(); ++i, ++j )
    {
        vec3 tmpPos( *i );
        *i = 1.99 * (*i) - 0.99 * (*j) + accel;
        *j = tmpPos;
    }
    
    // satisfy constraints: repeating this section multiple times makes the constraints more rigid
    for( int rep = 0; rep < numConstraintReps; ++rep )
    {
        // constraint 1: hold mesh together
        if( connections )
        {
            if( connectTightly )
            {
                int ni = 0;
                for( std::vector<std::vector<float> >::iterator vi = connections->begin(); vi != connections->end(); ++vi, ++ni )
                {
                    int nj = 0;
                    for( std::vector<float>::iterator vj = vi->begin(); vj != vi->end(); ++vj, ++nj )
                    {
                        if( ni != nj && *vj >= 0.0 && !deletedVertices[ni] && !deletedVertices[nj] )
                        {
                            stickConstraint( vert[ni], vert[nj], *vj );
                        }
                    }
                }
            }
            else
            {
                int ni = 0;
                for( std::vector<std::vector<float> >::iterator vi = connections->begin(); vi != connections->end(); ++vi, ++ni )
                {
                    int nj = 0;
                    assert( vi->size() % 2 == 0 );
                    for( std::vector<float>::iterator vj = vi->begin(); vj != vi->end(); vj += 2 )
                    {
                        nj = (int)*vj;
                        if( !deletedVertices[ni] && !deletedVertices[nj] )
                        {
                            stickConstraint( vert[ni], vert[nj], *(vj+1) );
                        }
                    }
                }
            }
        }
        
        // constraint 2: keep model on/above floor and add friction
        for( std::vector<vec3>::iterator i = vert.begin(), j = prevVert.begin(); i != vert.end() && j != prevVert.end(); ++i, ++j )
        {
            moveInsideVolume( *i, *j );
        }
        
        // constraint 3: pinned vertices
        for( std::vector<vec3>::iterator gi = pinnedVertices.begin(), vi = vert.begin(); gi != pinnedVertices.end() && vi != vert.end(); ++gi, ++vi )
        {
            if( gi->x || gi->y || gi->z )
            {
                *vi = *gi;
            }
        }
    }
    
    // calculate new normals
    //calculateMeshNormals();

    // display and repeat
    bufferNewData();
    glutPostRedisplay();
    glutTimerFunc( mspf, animate, 0 );
}

void createTextures()
{
    // default/floor texture
    unsigned char white[] = { 255, 255, 255 };
    defaultTexture = SOIL_create_OGL_texture( white, 1, 1, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0 );
    if( !defaultTexture )
    {
        std::cerr << "Unable to create default texture: " << SOIL_last_result() << std::endl;
    }
    else
    {
        glBindTexture( GL_TEXTURE_2D, defaultTexture );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    }
    
    // object textures
    for( std::vector<tinyobj::material_t>::iterator i = materials.begin(); i != materials.end(); ++i )
    {
        if( !i->diffuse_texname.empty() )
        {
            objTexture.push_back( SOIL_load_OGL_texture( i->diffuse_texname.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y ) );
            if( !objTexture.back() )
            {
                std::cerr << "Texture loading error (" << i->diffuse_texname << "): " << SOIL_last_result() << std::endl;
                objTexture.back() = defaultTexture;
            }
            else
            {
                glBindTexture( GL_TEXTURE_2D, objTexture.back() );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            }
        }
        else
        {
            objTexture.push_back( defaultTexture );
        }
    }
}

inline void printUsageAndQuit( const char* prog )
{
    std::cerr << "Usage: " << prog << " -o <OBJ file> [-s <scale>] [-n <# Verlet repetitions>]\n\t[-x <translationX>] [-y <translationY>] [-z <translationZ>]\n\t[-X <rotationX>] [-Y <rotationY>] [-Z <rotationZ>]\n\t[-c <connection level (0-3)>] [-g <gravity level (0-6)>]" << std::endl;
    exit( EXIT_FAILURE );
}

void parseInput( int argc, char** argv )
{
    int result;
    while( ( result = getopt( argc, argv, "o:s:x:y:z:X:Y:Z:c:n:g:" ) ) != -1 )
    {
        switch( result )
        {
            case 'o':
                objFilePath = optarg;
                break;
            case 's':
                objSize = atof( optarg );
                break;
            case 'x':
                objTrans.x = atof( optarg );
                break;
            case 'y':
                objTrans.y = atof( optarg );
                break;
            case 'z':
                objTrans.z = atof( optarg );
                break;
            case 'X':
                objRot.x = atof( optarg );
                break;
            case 'Y':
                objRot.y = atof( optarg );
                break;
            case 'Z':
                objRot.z = atof( optarg );
                break;
            case 'c':
                if( *optarg == '0' )
                {
                    connections = NULL;
                    connectTightly = false;
                }
                else if( *optarg == '1' )
                {
                    connections = &looseConnections;
                    connectTightly = false;
                }
                else if( *optarg == '2' )
                {
                    connections = &midConnections;
                    connectTightly = false;
                }
                else if( *optarg == '3' )
                {
                    connections = &tightConnections;
                    connectTightly = true;
                }
                else
                {
                    std::cerr << "Unrecognized connection level. Default is 1." << std::endl;
                }
                break;
            case 'n':
                numConstraintReps = atoi( optarg );
                break;
            case 'g':
                grav.z = -1 * std::min( atoi( optarg ), maxGrav );
                break;
            case '?':
            default:
                printUsageAndQuit( argv[0] );
        }
    }
    
    if( objFilePath.empty() )
    {
        printUsageAndQuit( argv[0] );
    }
}

/////////////////////////////////////////////////////////////////

int main( int argc, char** argv )
{
    // check input
    parseInput( argc, argv );
    
    // setup
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowSize( windowW, windowH );
    glutInitWindowPosition( 0, 0 );
    glutCreateWindow( "CSE 40166 - Final Project" );
    
    // settings
    init();
    
    // data
    loadObjFile( objFilePath.c_str() );
    createTextures();
    setupFloor();
    bufferData();
    
    // callbacks
    glutDisplayFunc( display );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyboard );
    glutSpecialFunc( keyboardSpecials );
    glutMouseFunc( mouse );
    glutMotionFunc( mouseMove );
    glutTimerFunc( mspf, animate, 0 );
    
    // run
    glutMainLoop();
    exit( EXIT_SUCCESS );
}
