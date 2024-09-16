
function ToGdString(str) {
	const encoder = new TextEncoder();
	const stringBytes = encoder.encode(str);
	const ptr = Module._malloc(stringBytes.length + 1); 
	Module.HEAPU8.set(stringBytes, ptr);
	Module.HEAPU8[ptr + stringBytes.length] = 0;
	return ptr
}
function ToGdFloat(value) {
	return value
}
function ToGdBool(value) {
	return value
}

function ToGdObj(object) {
	func = GodotEngine.rtenv['_gdspx_new_obj']; 
	return func(object)
}

function ToGdVec2(vec) {
	func = GodotEngine.rtenv['_gdspx_new_vec2']; 
	return func(vec.x, vec.y)
}
function ToGdVec3(vec) {
	func = GodotEngine.rtenv['_gdspx_new_vec3']; 
	return func(vec.x, vec.y, vec.z)
}

function ToGdVec4(vec) {
	func = GodotEngine.rtenv['_gdspx_new_vec4']; 
	return func(vec.x, vec.y, vec.z, vec.w)
}

function ToGdColor(color) {
	func = GodotEngine.rtenv['_gdspx_new_color']; 
	return func(color.r, color.g, color.b, color.a)
}

function ToGdRect2(rect) {
	func = GodotEngine.rtenv['_gdspx_new_rect2']; 
	return func(rect.position.x, rect.position.y, rect.size.width, rect.size.height)
}

function AllocGdInt() {
	ptr = GodotEngine.rtenv['_gdspx_alloc_int']; 
	return ptr
}

function AllocGdBool() {
	return GodotEngine.rtenv['_gdspx_alloc_bool']();
}

function AllocGdFloat() {
	return GodotEngine.rtenv['_gdspx_alloc_float']();
}

function AllocGdString() {
	return GodotEngine.rtenv['_gdspx_alloc_string']();
}

function AllocGdVec2() {
	return GodotEngine.rtenv['_gdspx_alloc_vec2']();
}

function AllocGdVec3() {
	return GodotEngine.rtenv['_gdspx_alloc_vec3']();
}

function AllocGdVec4() {
	return GodotEngine.rtenv['_gdspx_alloc_vec4']();
}

function AllocGdColor() {
	return GodotEngine.rtenv['_gdspx_alloc_color']();
}

function AllocGdRect2() {
	return GodotEngine.rtenv['_gdspx_alloc_rect2']();
}

function PrintGdFloat(obj) {
	console.log(obj);
}

function PrintGdBool(obj) {
	console.log(obj);
}

function PrintGdString(obj) {
	GodotEngine.rtenv['_gdspx_print_string'](obj);
}

function PrintGdVec4(obj) {
	GodotEngine.rtenv['_gdspx_print_vec4'](obj);
}
function PrintGdVec2(obj) {
	GodotEngine.rtenv['_gdspx_print_vec2'](obj);
}

function PrintGdVec3(obj) {
	GodotEngine.rtenv['_gdspx_print_vec3'](obj);
}

function PrintGdColor(obj) {
	GodotEngine.rtenv['_gdspx_print_color'](obj);
}

function PrintGdRect2(obj) {
	GodotEngine.rtenv['_gdspx_print_rect2'](obj);
}

function PrintGdInt(obj) {
	GodotEngine.rtenv['_gdspx_print_int'](obj);
}

