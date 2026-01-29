/**************************************************************************/
/*  spx_object_mgr.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef SPX_OBJECT_MGR_H
#define SPX_OBJECT_MGR_H

#include "spx_base_mgr.h"
#include "core/os/mutex.h"
#include "core/os/rw_lock.h"
#include "core/templates/hash_map.h"
#include "gdextension_spx_ext.h"

/**
 * @brief Generic object manager template for SPX objects
 * 
 * This template provides common functionality for managing SPX objects:
 * - Thread-safe object storage and retrieval using RWLock
 * - Automatic lifecycle management (create, destroy, update)
 * - Common helper macros for null checks
 * 
 * Thread Safety:
 * - Read operations (get_object) use shared locks for better concurrency
 * - Write operations (create, destroy) use exclusive locks
 * - All public methods are thread-safe
 * 
 * @tparam T The type of object being managed (e.g., SpxAudio, SpxPen)
 */
template <typename T>
class SpxObjectMgr : public SpxBaseMgr {
protected:
	HashMap<GdObj, T *> id_objects;
	mutable RWLock rw_lock;  // Read-Write lock for better concurrent performance
	Node2D *root = nullptr;
	
	/**
	 * @brief Create and register a new object
	 * Can be called directly from derived classes
	 * Thread-safe: Uses write lock
	 */
	GdObj _create_object() {
		auto id = get_unique_id();
		T *node = memnew(T);
		rw_lock.write_lock();
		node->on_create(id, root);
		id_objects[id] = node;
		rw_lock.write_unlock();
		return id;
	}

	/**
	 * @brief Get object by ID (internal, assumes lock is held)
	 * @warning NOT thread-safe - caller must hold appropriate lock
	 */
	T *_get_object_unsafe(GdObj obj) const {
		if (id_objects.has(obj)) {
			return id_objects[obj];
		}
		return nullptr;
	}

	/**
	 * @brief Create root node for this manager's objects
	 */
	void _create_root(const String &name) {
		root = memnew(Node2D);
		root->set_name(name);
		get_spx_root()->add_child(root);
	}

	/**
	 * @brief Destroy all managed objects and root node
	 */
	void _destroy_all();

	/**
	 * @brief Update all managed objects
	 */
	void _update_all(float delta);

	/**
	 * @brief Reset all managed objects
	 */
	void _reset_all(int reset_code);

public:
	/**
	 * @brief Get object by ID for reading (thread-safe with shared lock)
	 * Use this for read-only operations - allows multiple concurrent reads
	 * @param obj Object ID
	 * @return Pointer to object, or nullptr if not found
	 */
	T *get_object(GdObj obj) {
		rw_lock.read_lock();
		T *result = _get_object_unsafe(obj);
		rw_lock.read_unlock();
		return result;
	}

	/**
	 * @brief Get object by ID (const version for read-only access)
	 * Thread-safe with shared lock
	 */
	const T *get_object(GdObj obj) const {
		rw_lock.read_lock();
		const T *result = _get_object_unsafe(obj);
		rw_lock.read_unlock();
		return result;
	}

	/**
	 * @brief Destroy a managed object by ID (thread-safe with exclusive lock)
	 */
	void destroy_object(GdObj obj);

	/**
	 * @brief Get the number of managed objects (thread-safe)
	 */
	int get_object_count() const {
		rw_lock.read_lock();
		int count = id_objects.size();
		rw_lock.read_unlock();
		return count;
	}

	virtual ~SpxObjectMgr() = default;
};

template <typename T>
void SpxObjectMgr<T>::_destroy_all() {
	rw_lock.write_lock();
	for (const KeyValue<GdObj, T *> &E : id_objects) {
		E.value->on_destroy();
	}
	id_objects.clear();
	rw_lock.write_unlock();
	
	if (root) {
		root->queue_free();
		root = nullptr;
	}
}

template <typename T>
void SpxObjectMgr<T>::_update_all(float delta) {
	rw_lock.read_lock();
	// Create a copy to avoid holding lock during callbacks
	Vector<T *> objects_copy;
	for (const KeyValue<GdObj, T *> &E : id_objects) {
		objects_copy.push_back(E.value);
	}
	rw_lock.read_unlock();
	
	// Call callbacks without holding lock to avoid deadlocks
	for (T *obj : objects_copy) {
		obj->on_update(delta);
	}
}

template <typename T>
void SpxObjectMgr<T>::_reset_all(int reset_code) {
	rw_lock.write_lock();
	for (const KeyValue<GdObj, T *> &E : id_objects) {
		E.value->on_reset(reset_code);
	}
	id_objects.clear();
	rw_lock.write_unlock();
}

template <typename T>
void SpxObjectMgr<T>::destroy_object(GdObj obj) {
	rw_lock.write_lock();
	T *object = _get_object_unsafe(obj);
	if (object != nullptr) {
		id_objects.erase(obj);
		rw_lock.write_unlock();
		// Destroy outside of lock to avoid potential deadlocks
		object->on_destroy();
	} else {
		rw_lock.write_unlock();
	}
}

// Common macros for checking and getting objects with error handling
#define SPX_CHECK_AND_GET_OBJECT_V(obj, getter, obj_type)                     \
	auto obj = getter;                                                         \
	if (obj == nullptr) {                                                      \
		print_error("try to access null " #obj_type " object");                \
		return;                                                                \
	}

#define SPX_CHECK_AND_GET_OBJECT_R(obj, getter, obj_type, ret_value)          \
	auto obj = getter;                                                         \
	if (obj == nullptr) {                                                      \
		print_error("try to access null " #obj_type " object");                \
		return ret_value;                                                      \
	}

#endif // SPX_OBJECT_MGR_H
