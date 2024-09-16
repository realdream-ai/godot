// TODO @jiepengtan cache function pointer

// Bool-related functions
function ToGdBool(value) {
	func = GodotEngine.rtenv['_gdspx_new_bool']; 
	return func(value);
}

function ToJsBool(ptr) {
    const HEAPU8 = Module.HEAPU8;
    const boolValue = HEAPU8[ptr];
    return boolValue !== 0;
}

function AllocGdBool() {
	return GodotEngine.rtenv['_gdspx_alloc_bool']();
}

function PrintGdBool(ptr) {
	console.log(ToJsBool(ptr));
}

// Int-related functions
function ToGdInt(value) {
	func = GodotEngine.rtenv['_gdspx_new_int']; 
	return func(value);
}

function ToJsInt(ptr) {
    const memoryBuffer = Module.HEAPU8.buffer;
    const dataView = new DataView(memoryBuffer);
    const low = dataView.getUint32(ptr, true);  // 低32位
    const high = dataView.getUint32(ptr + 4, true);  // 高32位
    const int64Value = BigInt(high) << 32n | BigInt(low);
    return int64Value;
}

function AllocGdInt() {
	ptr = GodotEngine.rtenv['_gdspx_alloc_int']; 
	return ptr;
}

function PrintGdInt(ptr) {
	console.log(ToJsInt(ptr));
}

// Float-related functions
function ToGdFloat(value) {
	func = GodotEngine.rtenv['_gdspx_new_float']; 
	return func(value);
}

function ToJsFloat(ptr) {
    const HEAPF32 = Module.HEAPF32; 
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

// String-related functions
function ToGdString(str) {
	const encoder = new TextEncoder();
	const stringBytes = encoder.encode(str);
	const ptr = Module._malloc(stringBytes.length + 1); 
	Module.HEAPU8.set(stringBytes, ptr);
	Module.HEAPU8[ptr + stringBytes.length] = 0;
	return ptr;
}

function ToJsString(ptr) {
    const HEAPU8 = Module.HEAPU8;
    let length = 0;
    while (HEAPU8[ptr + length] !== 0) {
        length++;
    }
    const stringBytes = HEAPU8.subarray(ptr, ptr + length);
    const decoder = new TextDecoder();
    return decoder.decode(stringBytes);
}

function AllocGdString() {
	return GodotEngine.rtenv['_gdspx_alloc_string']();
}

function PrintGdString(ptr) {
	console.log(ToJsString(ptr));
}

// Object-related functions
function ToGdObj(object) {
	func = GodotEngine.rtenv['_gdspx_new_obj']; 
	return func(object);
}

function ToJsObj(ptr) {
    const memoryBuffer = Module.HEAPU8.buffer;
    const dataView = new DataView(memoryBuffer);
    const low = dataView.getUint32(ptr, true);  // 低32位
    const high = dataView.getUint32(ptr + 4, true);  // 高32位
    const int64Value = BigInt(high) << 32n | BigInt(low);
    return int64Value;
}

function AllocGdObj() {
	return GodotEngine.rtenv['_gdspx_alloc_obj']();
}

function PrintGdObj(ptr) {
	console.log(ToJsObj(ptr));
}

// Vec2-related functions
function ToGdVec2(vec) {
	func = GodotEngine.rtenv['_gdspx_new_vec2']; 
	return func(vec.x, vec.y);
}

function ToJsVec2(ptr) {
    const HEAPF32 = Module.HEAPF32;
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

// Vec3-related functions
function ToGdVec3(vec) {
	func = GodotEngine.rtenv['_gdspx_new_vec3']; 
	return func(vec.x, vec.y, vec.z);
}

function ToJsVec3(ptr) {
    const HEAPF32 = Module.HEAPF32;
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

// Vec4-related functions
function ToGdVec4(vec) {
	func = GodotEngine.rtenv['_gdspx_new_vec4']; 
	return func(vec.x, vec.y, vec.z, vec.w);
}

function ToJsVec4(ptr) {
    const HEAPF32 = Module.HEAPF32;
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

// Color-related functions
function ToGdColor(color) {
	func = GodotEngine.rtenv['_gdspx_new_color']; 
	return func(color.r, color.g, color.b, color.a);
}

function ToJsColor(ptr) {
    const HEAPF32 = Module.HEAPF32;
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

// Rect2-related functions
function ToGdRect2(rect) {
	func = GodotEngine.rtenv['_gdspx_new_rect2']; 
	return func(rect.position.x, rect.position.y, rect.size.width, rect.size.height);
}

function ToJsRect2(ptr) {
    const HEAPF32 = Module.HEAPF32;
    const floatIndex = ptr / 4;
    return {
        position: {
            x: HEAPF32[floatIndex],
            y: HEAPF32[floatIndex + 1]
        },
        size: {
            width: HEAPF32[floatIndex + 2],
            height: HEAPF32[floatIndex + 3]
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
