#include <math.h>
#include <png.h>
#include <assert.h>
#include <stdlib.h>
#include "pngloader.h"

/* Decoded image */
typedef struct {
    const png_uint_32 width;
    const png_uint_32 height;
    const png_size_t size;
    const GLenum gl_color_format;
    const void* data;
} RawImageData;

typedef struct {
    const png_byte* data;
    const png_size_t size;
} DataHandle;

typedef struct {
    const DataHandle data;
    png_size_t offset;
} ReadDataHandle;

typedef struct {
    const png_uint_32 width;
    const png_uint_32 height;
    const int color_type;
} PngInfo;

/* Copy one row of image into pixel buffer */
static void read_png_data_callback(png_structp png_ptr, png_byte* raw_data, png_size_t read_length) {
    ReadDataHandle* handle = (ReadDataHandle *) png_get_io_ptr(png_ptr);
    const png_byte* png_src = handle->data.data + handle->offset;

    memcpy(raw_data, png_src, read_length);
    handle->offset += read_length;
}

static PngInfo read_and_update_info(const png_structp png_ptr, const png_infop info_ptr) {
    png_uint_32 width, height;
    int bit_depth, color_type;

    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    /* Transparency -> alpha channel */
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    /* Handle greyscale */
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    /* Handle paletted image */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    /* Make sure we have alpha channel */
    if (color_type == PNG_COLOR_TYPE_PALETTE || color_type == PNG_COLOR_TYPE_RGB)
        png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

    /* Make sure we work with 8-bit images */
    if (bit_depth < 8)
        png_set_packing(png_ptr);
    else if (bit_depth == 16) {
        printf("16 bit image not supported\n");
        assert(0);
    }

    png_read_update_info(png_ptr, info_ptr);

    /* Get color type */
    color_type = png_get_color_type(png_ptr, info_ptr);

    return (PngInfo) {width, height, color_type};
}

/* Decode PNG-data */
static DataHandle read_entire_png_image(
        const png_structp png_ptr,
        const png_infop info_ptr,
        const png_uint_32 height)
{
    /* How many bytes per image row */
    const png_size_t row_size = png_get_rowbytes(png_ptr, info_ptr);
    /* How many byte per image */
    const png_size_t data_length = row_size * height;
    assert(row_size > 0);

    /* Image buffer */
    png_byte* raw_image = (png_byte *) malloc(data_length);
    assert(raw_image != NULL);

    /* Pointers to image rows */
    png_byte* row_ptrs[height];

    /* Initialize pointers */
    png_uint_32 i;
    for (i = 0; i < height; i++) {
        row_ptrs[i] = raw_image + i * row_size;
    }

    /*  Decode file */
    png_read_image(png_ptr, &row_ptrs[0]);

    return (DataHandle) {raw_image, data_length};
}

/* Transform PNG color type into EGL texture format */
static GLenum get_gl_color_format(const int png_color_format) {
    assert(png_color_format == PNG_COLOR_TYPE_GRAY
           || png_color_format == PNG_COLOR_TYPE_RGB_ALPHA
           || png_color_format == PNG_COLOR_TYPE_GRAY_ALPHA);

    switch (png_color_format) {
        case PNG_COLOR_TYPE_GRAY:
            return GL_LUMINANCE;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            return GL_RGBA;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            return GL_LUMINANCE_ALPHA;
    }

    return 0;
}

/* Free resources */
static void release_raw_image_data(const RawImageData* img) {
    assert(img != NULL);
    free((void*)img->data);
}

/* Decode PNG-file to pixels */
static RawImageData get_raw_image_data_from_png(const void* png_data, const png_size_t png_data_size) {
    /* Make sure it's PNG */
    assert(png_data != NULL && png_data_size > 8);
    assert(png_check_sig((png_bytep)png_data, 8));

    /* Prepare for decoding */
    png_structp png_ptr = png_create_read_struct(
            PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    assert(png_ptr != NULL);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    assert(info_ptr != NULL);

    /* Set our callback */
    ReadDataHandle png_data_handle = (ReadDataHandle) {{(const png_byte *) png_data, png_data_size}, 0};
    png_set_read_fn(png_ptr, &png_data_handle, read_png_data_callback);

    /* Set error handler */
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("Error reading PNG file");
        assert(0);
    }

    /* Process data */
    const PngInfo png_info = read_and_update_info(png_ptr, info_ptr);
    const DataHandle raw_image = read_entire_png_image(png_ptr, info_ptr, png_info.height);

    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    /* Result */
    return (RawImageData) {
            png_info.width,
            png_info.height,
            raw_image.size,
            get_gl_color_format(png_info.color_type),
            raw_image.data};
}

/* Create an EGL texture based on a pixel buffer */
static GLuint gl_load_texture(const GLsizei width, const GLsizei height, const GLenum type, const GLvoid* pixels) {
    GLuint texture_id = 0;

    /* Create texture */
    glGenTextures(1, &texture_id);
    assert(texture_id != 0);

    /* Initialize texture parameters */
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    /* Load pixels into texture */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    /* Return texture descriptor */
    return texture_id;
}

/* Load file into memory */
static png_bytep load_file(const char* path, size_t* data_length) {
    /* Open file, get it's size */
    FILE* fp = fopen(path, "r");
    assert( fp != NULL );
    assert( fseek(fp, 0, SEEK_END) != -1 );

    size_t file_len = ftell(fp);
    assert( fseek(fp, 0, SEEK_SET) != -1 );

    /* Load into memory */
    png_bytep data = (png_bytep) malloc(file_len);
    assert( data != NULL );
    assert( fread(data, 1, file_len, fp) == file_len );
    fclose(fp);

    if( data_length != NULL ) *data_length = file_len;

    return data;
}

/* Load and decode a PNG-file */
char* load_png_image(const char* path, size_t* width, size_t* height) {
    /* Load file into memory */
    size_t data_length = 0;
    png_bytep data = load_file(path, &data_length);

    /* Decode PNG-data into pixels */
    const RawImageData raw_image_data = get_raw_image_data_from_png(data, data_length);

    /* Get image size */
    if( width != NULL )  *width = raw_image_data.width;
    if( height != NULL ) *height = raw_image_data.height;

    /* Free memory */
    free(data);

    /* Return image data */
    return (char*) raw_image_data.data;
}

/* Create EGL texture out of PNG-file, get image size */
GLuint load_png_as_texture(const char* path, size_t* width, size_t* height) {
    /* Load file into memory */
    size_t data_length = 0;
    png_bytep data = load_file(path, &data_length);

    /* Decode file into pixels */
    const RawImageData raw_image_data = get_raw_image_data_from_png(data, data_length);

    /* Convert pixels into texture */
    const GLuint texture_object_id = gl_load_texture(
            raw_image_data.width, raw_image_data.height,
            raw_image_data.gl_color_format, raw_image_data.data);

    if( width != NULL ) *width = raw_image_data.width;
    if( height != NULL ) *height = raw_image_data.height;

    /* Free resources */
    release_raw_image_data(&raw_image_data);
    free(data);

    /* Return texture descriptor */
    return texture_object_id;
}

/* Create EGL texture out of PNG file */
GLuint load_png_as_texture(const char* path) {
    return load_png_as_texture(path, NULL, NULL);
}

