// TODO @jiepengtan cache function pointer

// Bool-related functions
function ToGdBool(value) {
    func = GodotEngine.rtenv['_gdspx_new_bool']; 
    return func(value);
}

function ToJsBool(ptr) {
    const HEAPU8 = GodotModule.HEAPU8;
    const boolValue = HEAPU8[ptr];
    return boolValue !== 0;
}

function AllocGdBool() {
    return GodotEngine.rtenv['_gdspx_alloc_bool']();
}

function PrintGdBool(ptr) {
    console.log(ToJsBool(ptr));
}

function FreeGdBool(ptr) {
    GodotEngine.rtenv['_gdspx_free_bool'](ptr);
}



// Object-related functions
function ToGdObject(object) {
    return ToGdObj(object);
}
function ToJsObject(ptr) {
    return ToJsObj(ptr);
}
function FreeGdObject(ptr) {
    FreeGdObj(ptr);
}
function AllocGdObject() {
    return AllocGdObj();
}
function PrintGdObject(ptr) {
    PrintGdObj(ptr);
}

function ToGdObj(value) {
    func = GodotEngine.rtenv['_gdspx_new_obj']; 
    return func(value.high, value.low);
}

function ToJsObj(ptr) {
    const memoryBuffer = GodotModule.HEAPU8.buffer;
    const dataView = new DataView(memoryBuffer);
    const low = dataView.getUint32(ptr, true);  // 低32位
    const high = dataView.getUint32(ptr + 4, true);  // 高32位
    //const int64Value = BigInt(high) << 32n | BigInt(low);
    return {
        low : low,
        high : high
    };
}

function AllocGdObj() {
    return GodotEngine.rtenv['_gdspx_alloc_obj']();
}

function PrintGdObj(ptr) {
    console.log(ToJsObj(ptr));
}

function FreeGdObj(ptr) {
    GodotEngine.rtenv['_gdspx_free_obj'](ptr);
}

// Int-related functions
function ToGdInt(value) {
    func = GodotEngine.rtenv['_gdspx_new_int']; 
    return func(value.high, value.low);
}

function ToJsInt(ptr) {
    const memoryBuffer = GodotModule.HEAPU8.buffer;
    const dataView = new DataView(memoryBuffer);
    const low = dataView.getUint32(ptr, true);  // 低32位
    const high = dataView.getUint32(ptr + 4, true);  // 高32位
    return {
        low : low,
        high : high
    };
}

function AllocGdInt() {
    ptr = GodotEngine.rtenv['_gdspx_alloc_int']; 
    return ptr;
}

function PrintGdInt(ptr) {
    console.log(ToJsInt(ptr));
}

function FreeGdInt(ptr) {
    GodotEngine.rtenv['_gdspx_free_int'](ptr);
}


// Float-related functions
function ToGdFloat(value) {
    func = GodotEngine.rtenv['_gdspx_new_float']; 
    return func(value);
}

function ToJsFloat(ptr) {
    const HEAPF32 = GodotModule.HEAPF32; 
    const floatIndex = ptr / 4;
    const floatValue = HEAPF32[floatIndex];
    return floatValue;
}

function AllocGdFloat() {
    return GodotEngine.rtenv['_gdspx_alloc_float']();
}

function PrintGdFloat(ptr) {
    console.log(ToJsFloat(ptr));
}

function FreeGdFloat(ptr) {
    GodotEngine.rtenv['_gdspx_free_float'](ptr);
}

// String-related functions
function ToGdString(str) {
    const encoder = new TextEncoder();
    const stringBytes = encoder.encode(str);
    const ptr = GodotModule._malloc(stringBytes.length + 1); 
    GodotModule.HEAPU8.set(stringBytes, ptr);
    GodotModule.HEAPU8[ptr + stringBytes.length] = 0;
    func = GodotEngine.rtenv['_gdspx_new_string']; 
    gdstrPtr= func(ptr);
    return gdstrPtr;
}

function ToJsString(gdstrPtr) {
    func = GodotEngine.rtenv['_gdspx_get_string']; 
    ptr = func(gdstrPtr)
    const HEAPU8 = GodotModule.HEAPU8;
    let length = 0;
    while (HEAPU8[ptr + length] !== 0) {
        length++;
    }
    const stringBytes = HEAPU8.subarray(ptr, ptr + length);
    result = decodeFromSharedBuffer(stringBytes,length)
    return result;
}

function decodeFromSharedBuffer(sharedBuffer, maxLength) {
  const decoder = new TextDecoder()
  const copyLength = Math.min(sharedBuffer.byteLength, maxLength)

  // Create a temporary ArrayBuffer and copy the contents of the shared buffer
  // into it.
  const tempBuffer = new ArrayBuffer(copyLength)
  const tempView = new Uint8Array(tempBuffer)

  let sharedView = new Uint8Array(sharedBuffer)
  if (sharedBuffer.byteLength != copyLength) {
    sharedView = sharedView.subarray(0, copyLength)
  }
  tempView.set(sharedView)

  return decoder.decode(tempBuffer)
}


function AllocGdString() {
    return GodotEngine.rtenv['_gdspx_alloc_string']();
}

function PrintGdString(ptr) {
    console.log(ToJsString(ptr));
}

function FreeGdString(ptr) {
    GodotEngine.rtenv['_gdspx_free_string'](ptr);
}



// Vec2-related functions
function ToGdVec2(vec) {
    func = GodotEngine.rtenv['_gdspx_new_vec2']; 
    return func(vec.x, vec.y);
}

function ToJsVec2(ptr) {
    const HEAPF32 = GodotModule.HEAPF32;
    const floatIndex = ptr / 4;
    return {
        x: HEAPF32[floatIndex],
        y: HEAPF32[floatIndex + 1]
    };
}

function AllocGdVec2() {
    return GodotEngine.rtenv['_gdspx_alloc_vec2']();
}

function PrintGdVec2(ptr) {
    console.log(ToJsVec2(ptr));
}

function FreeGdVec2(ptr) {
    GodotEngine.rtenv['_gdspx_free_vec2'](ptr);
}


// Vec3-related functions
function ToGdVec3(vec) {
    func = GodotEngine.rtenv['_gdspx_new_vec3']; 
    return func(vec.x, vec.y, vec.z);
}

function ToJsVec3(ptr) {
    const HEAPF32 = GodotModule.HEAPF32;
    const floatIndex = ptr / 4;
    return {
        x: HEAPF32[floatIndex],
        y: HEAPF32[floatIndex + 1],
        z: HEAPF32[floatIndex + 2]
    };
}

function AllocGdVec3() {
    return GodotEngine.rtenv['_gdspx_alloc_vec3']();
}

function PrintGdVec3(ptr) {
    const vec3 = ToJsVec3(ptr);
    console.log(`Vec3(${vec3.x}, ${vec3.y}, ${vec3.z})`);
}

function FreeGdVec3(ptr) {
    GodotEngine.rtenv['_gdspx_free_vec3'](ptr);
}


// Vec4-related functions
function ToGdVec4(vec) {
    func = GodotEngine.rtenv['_gdspx_new_vec4']; 
    return func(vec.x, vec.y, vec.z, vec.w);
}

function ToJsVec4(ptr) {
    const HEAPF32 = GodotModule.HEAPF32;
    const floatIndex = ptr / 4;
    return {
        x: HEAPF32[floatIndex],
        y: HEAPF32[floatIndex + 1],
        z: HEAPF32[floatIndex + 2],
        w: HEAPF32[floatIndex + 3]
    };
}

function AllocGdVec4() {
    return GodotEngine.rtenv['_gdspx_alloc_vec4']();
}

function PrintGdVec4(ptr) {
    const vec4 = ToJsVec4(ptr);
    console.log(`Vec4(${vec4.x}, ${vec4.y}, ${vec4.z}, ${vec4.w})`);
}

function FreeGdVec4(ptr) {
    GodotEngine.rtenv['_gdspx_free_vec4'](ptr);
}


// Color-related functions
function ToGdColor(color) {
    func = GodotEngine.rtenv['_gdspx_new_color']; 
    return func(color.r, color.g, color.b, color.a);
}

function ToJsColor(ptr) {
    const HEAPF32 = GodotModule.HEAPF32;
    const floatIndex = ptr / 4;
    return {
        r: HEAPF32[floatIndex],
        g: HEAPF32[floatIndex + 1],
        b: HEAPF32[floatIndex + 2],
        a: HEAPF32[floatIndex + 3]
    };
}

function AllocGdColor() {
    return GodotEngine.rtenv['_gdspx_alloc_color']();
}

function PrintGdColor(ptr) {
    const color = ToJsColor(ptr);
    console.log(`Color(${color.r}, ${color.g}, ${color.b}, ${color.a})`);
}

function FreeGdColor(ptr) {
    GodotEngine.rtenv['_gdspx_free_color'](ptr);
}


// Rect2-related functions
function ToGdRect2(rect) {
    func = GodotEngine.rtenv['_gdspx_new_rect2']; 
    return func(rect.position.x, rect.position.y, rect.size.width, rect.size.height);
}

function ToJsRect2(ptr) {
    const HEAPF32 = GodotModule.HEAPF32;
    const floatIndex = ptr / 4;
    return {
        position: {
            x: HEAPF32[floatIndex],
            y: HEAPF32[floatIndex + 1]
        },
        size: {
            x: HEAPF32[floatIndex + 2],
            y: HEAPF32[floatIndex + 3]
        }
    };
}

function AllocGdRect2() {
    return GodotEngine.rtenv['_gdspx_alloc_rect2']();
}

function PrintGdRect2(ptr) {
    const rect = ToJsRect2(ptr);
    console.log(`Rect2(position: (${rect.position.x}, ${rect.position.y}), size: (${rect.size.width}, ${rect.size.height}))`);
}

function FreeGdRect2(ptr) {
    GodotEngine.rtenv['_gdspx_free_rect2'](ptr);
}
