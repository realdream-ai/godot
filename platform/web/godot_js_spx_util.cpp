#include "core/extension/gdextension_spx_ext.h"
#include "core/extension/gdextension.h"

#include "godot_js_spx_util.h"

#include <emscripten.h>


static ObjectPool<GdVec2> vec2Pool(100);
static ObjectPool<GdString> stringPool(100);
static ObjectPool<GdObj> objPool(100);
static ObjectPool<GdInt> intPool(100);
static ObjectPool<GdFloat> floatPool(100);
static ObjectPool<GdBool> boolPool(100);
static ObjectPool<GdVec3> vec3Pool(100);
static ObjectPool<GdVec4> vec4Pool(100);
static ObjectPool<GdColor> colorPool(100);
static ObjectPool<GdRect2> rect2Pool(100);

extern "C" {

// other functions
EMSCRIPTEN_KEEPALIVE
float gdspx_get_value(float* array, int idx) {
    return array[idx];
}


// bool functions
EMSCRIPTEN_KEEPALIVE
GdBool* gdspx_alloc_bool() {
    return boolPool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdBool* gdspx_new_bool(bool val) {
    GdBool* ptr = gdspx_alloc_bool();
    *ptr = (GdBool)val;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_bool(GdBool* b) {
    boolPool.release(b);
}


EMSCRIPTEN_KEEPALIVE
void gdspx_print_bool(GdBool* value) {
    print_line("bool: %s", *value ? "true" : "false");
}

// float functions
EMSCRIPTEN_KEEPALIVE
GdFloat* gdspx_alloc_float() {
    return floatPool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdFloat* gdspx_new_float(float val) {
    GdFloat* ptr = gdspx_alloc_float();
    *ptr = (GdFloat)val;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_float(GdFloat* f) {
    floatPool.release(f);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_float(GdFloat* value) {
    print_line("float: %f", *value);
}

// int functions
EMSCRIPTEN_KEEPALIVE
GdInt* gdspx_alloc_int() {
    return intPool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdInt* gdspx_new_int(double val) {
    GdInt* ptr = gdspx_alloc_int();
    *ptr = (GdInt)val;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_int(GdInt* i) {
    intPool.release(i);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_int(GdInt* value) {
    print_line("int: %d", *value);
}

// object functions
EMSCRIPTEN_KEEPALIVE
GdObj* gdspx_alloc_obj() {
    return objPool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdObj* gdspx_new_obj(double val) {
    GdObj* ptr = gdspx_alloc_obj();
    *ptr = (GdObj)val;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_obj(GdObj* obj) {
    objPool.release(obj);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_obj(GdObj* obj) {
    print_line("object: %p", (void*)obj);
}

// vec2 functions
EMSCRIPTEN_KEEPALIVE
GdVec2* gdspx_alloc_vec2() {
    return vec2Pool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdVec2* gdspx_new_vec2(float x, float y) {
    GdVec2* ptr = gdspx_alloc_vec2();
    ptr->x = x;
    ptr->y = y;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_vec2(GdVec2* vec) {
    vec2Pool.release(vec);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_vec2(GdVec2* vec) {
    print_line("vec2: %f, %f", vec->x, vec->y);
}

// vec3 functions
EMSCRIPTEN_KEEPALIVE
GdVec3* gdspx_alloc_vec3() {
    return vec3Pool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdVec3* gdspx_new_vec3(float x, float y, float z) {
    GdVec3* ptr= gdspx_alloc_vec3();
    ptr->x = x;
    ptr->y = y;
    ptr->z = z;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_vec3(GdVec3* vec) {
    vec3Pool.release(vec);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_vec3(GdVec3* vec) {
    print_line("vec3: %f, %f, %f", vec->x, vec->y, vec->z);
}

// vec4 functions
EMSCRIPTEN_KEEPALIVE
GdVec4* gdspx_alloc_vec4() {
    return vec4Pool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdVec4* gdspx_new_vec4(float x, float y, float z, float w) {
    GdVec4* ptr = gdspx_alloc_vec4();
    ptr->x = x;
    ptr->y = y;
    ptr->z = z;
    ptr->w = w;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_vec4(GdVec4* vec) {
    vec4Pool.release(vec);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_vec4(GdVec4* vec) {
    print_line("vec4: %f, %f, %f, %f", vec->x, vec->y, vec->z, vec->w);
}

// color functions
EMSCRIPTEN_KEEPALIVE
GdColor* gdspx_alloc_color() {
    return colorPool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdColor* gdspx_new_color(float r, float g, float b, float a) {
    GdColor* ptr = gdspx_alloc_color();
    ptr->r = r;
    ptr->g = g;
    ptr->b = b;
    ptr->a = a;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_color(GdColor* color) {
    colorPool.release(color);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_color(GdColor* color) {
    print_line("color: %f, %f, %f, %f", color->r, color->g, color->b, color->a);
}

// rect2 functions
EMSCRIPTEN_KEEPALIVE
GdRect2* gdspx_alloc_rect2() {
    return rect2Pool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdRect2* gdspx_new_rect2(float x, float y, float width, float height) {
    GdRect2* ptr = gdspx_alloc_rect2();
    ptr->position.x = x;
    ptr->position.y = y;
    ptr->size.width = width;
    ptr->size.height = height;
    return ptr;
}

EMSCRIPTEN_KEEPALIVE
void gdspx_free_rect2(GdRect2* rect) {
    rect2Pool.release(rect);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_rect2(GdRect2* rect) {
    print_line("rect2: position(%f, %f), size(%f, %f)", rect->position.x, rect->position.y, rect->size.width, rect->size.height);
}

// string functions
EMSCRIPTEN_KEEPALIVE
GdString* gdspx_alloc_string() {
    return stringPool.acquire();
}

EMSCRIPTEN_KEEPALIVE
GdString* gdspx_new_string(const char* str) {
    GdString* ptr = gdspx_alloc_string();
    ptr->c_str = strdup(str);
    return ptr;
}

void gdspx_free_string(GdString* str) {
    stringPool.release(str);
}

EMSCRIPTEN_KEEPALIVE
void gdspx_print_string(GdString* str) {
    print_line("string: %s", str->c_str());
}

}
