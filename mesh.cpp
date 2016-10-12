#include <stdexcept>
#include <string>
#include <fstream>

#include "mesh.h"

/* Load 3D-mesh .OBJ file. See: https://en.wikipedia.org/wiki/Wavefront_.obj_file */
void Mesh::load(const char* file) {
    /* Clear buffers */
    vertices.clear();
    triangles.clear();

    /* Open file */
    std::ifstream fp(file);
    if( ! fp.is_open() ) throw std::runtime_error(std::string("Cannot open file: ") + file);

    /* Read data line by line */
    std::string line;
    while( std::getline(fp, line) ) {
        const char* buf = line.c_str();

        switch( buf[0] ) {
            /* Skip comments and empty lines */
            case '#': case '\r': case '\n': 
                break;

            /* Found vertex data, read it */
            case 'v': {
                vertex_t v;
                if( sscanf(buf, "v %f %f %f", &v.x, &v.y, &v.z) != 3 )
                    throw std::runtime_error(std::string("Error reading vertex from line: ") + buf);
                vertices.push_back(v);
            }
            break;

            /* Found triangle data. Note: in .OBJ file format indexes start at 1, so we take that
               in consideration */
            case 'f': {
                triangle_t t;
                if( sscanf(buf, "f %hu %hu %hu", &t.a, &t.b, &t.c) != 3 )
                    throw std::runtime_error(std::string("Error reading face from line: ") + buf);
                t.a -= 1; t.b -= 1; t.c -= 1;
                triangles.push_back(t);
            }
            break;
        }
    }

    printf("Loaded %zu vertices, %zu triangles\n", vertices.size(), triangles.size());
}

/* Load vertex data into EGL buffer */
GLuint Mesh::genVertexBuffer() {
    /* Create buffer and bind it */
    GLuint buf_id;
    glGenBuffers(1, &buf_id);
    glBindBuffer(GL_ARRAY_BUFFER, buf_id);

    /* Load vertex data into EGL buffer */
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_t), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    return buf_id;
}

/* Load index (triangle) data into EGL buffer */
GLuint Mesh::genTrianglesBuffer() {
    /* Create buffer */
    GLuint buf_id;
    glGenBuffers(1, &buf_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_id);

    /* Load indexes (triangles) into buffer */
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(triangle_t), &triangles[0], GL_STATIC_DRAW);
    return buf_id;
}

GLsizei Mesh::getVertexNum() {
    return vertices.size();
}

GLsizei Mesh::getTrianglesNum() {
    return triangles.size();
}


