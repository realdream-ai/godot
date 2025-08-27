#include "spx_mass_spring.h"
#include "core/os/keyboard.h"
#include "spx_engine.h"
#include "spx_camera_mgr.h"
#include "scene/2d/camera_2d.h"
#include "scene/main/viewport.h"
#include <complex>

void MassSpring2D::_bind_methods() {
    // Spring stiffness (Young’s modulus)
    ClassDB::bind_method(D_METHOD("set_spring_Y", "v"), &MassSpring2D::set_spring_Y);
    ClassDB::bind_method(D_METHOD("get_spring_Y"), &MassSpring2D::get_spring_Y);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spring_Y", PROPERTY_HINT_RANGE, "10,10000,1,or_greater"), "set_spring_Y", "get_spring_Y");

    // Air drag damping
    ClassDB::bind_method(D_METHOD("set_drag_damping", "v"), &MassSpring2D::set_drag_damping);
    ClassDB::bind_method(D_METHOD("get_drag_damping"), &MassSpring2D::get_drag_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drag_damping", PROPERTY_HINT_RANGE, "0,10,0.01"), "set_drag_damping", "get_drag_damping");

    // Dashpot damping (spring damping between particles)
    ClassDB::bind_method(D_METHOD("set_dashpot_damping", "v"), &MassSpring2D::set_dashpot_damping);
    ClassDB::bind_method(D_METHOD("get_dashpot_damping"), &MassSpring2D::get_dashpot_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dashpot_damping", PROPERTY_HINT_RANGE, "0,2000,1,or_greater"), "set_dashpot_damping", "get_dashpot_damping");

    // Pixel scale for rendering
    ClassDB::bind_method(D_METHOD("set_pixel_scale", "v"), &MassSpring2D::set_pixel_scale);
    ClassDB::bind_method(D_METHOD("get_pixel_scale"), &MassSpring2D::get_pixel_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixel_scale", PROPERTY_HINT_RANGE, "100,4096,1"), "set_pixel_scale", "get_pixel_scale");

    // Connection radius (distance threshold for creating a spring)
    ClassDB::bind_method(D_METHOD("set_connect_radius", "v"), &MassSpring2D::set_connect_radius);
    ClassDB::bind_method(D_METHOD("get_connect_radius"), &MassSpring2D::get_connect_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "connect_radius", PROPERTY_HINT_RANGE, "0.01,1,0.01"), "set_connect_radius", "get_connect_radius");

    // Default spring rest length
    ClassDB::bind_method(D_METHOD("set_default_rest", "v"), &MassSpring2D::set_default_rest);
    ClassDB::bind_method(D_METHOD("get_default_rest"), &MassSpring2D::get_default_rest);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_rest", PROPERTY_HINT_RANGE, "0.01,1,0.01"), "set_default_rest", "get_default_rest");

    // Pause flag
    ClassDB::bind_method(D_METHOD("set_paused", "p"), &MassSpring2D::set_paused);
    ClassDB::bind_method(D_METHOD("is_paused"), &MassSpring2D::is_paused);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "paused"), "set_paused", "is_paused");

    // Query methods
    ClassDB::bind_method(D_METHOD("get_num_particles"), &MassSpring2D::get_num_particles);
    ClassDB::bind_method(D_METHOD("get_particle_position", "i"), &MassSpring2D::get_particle_position);
    ClassDB::bind_method(D_METHOD("is_particle_fixed", "i"), &MassSpring2D::is_particle_fixed);
    ClassDB::bind_method(D_METHOD("get_rest_length", "i", "j"), &MassSpring2D::get_rest_length);

    // Core simulation methods
    ClassDB::bind_method(D_METHOD("clear"), &MassSpring2D::clear);
    ClassDB::bind_method(D_METHOD("new_particle", "p", "fixed"), &MassSpring2D::new_particle);
    ClassDB::bind_method(D_METHOD("attract_to", "p", "strength"), &MassSpring2D::attract_to);
    ClassDB::bind_method(D_METHOD("step_simulation", "steps"), &MassSpring2D::step_simulation);

    ClassDB::bind_method(D_METHOD("generate_fractal"), &MassSpring2D::generate_fractal);
}

/**
 * Ensure the rest length matrix has the right size.
 * This stores pairwise spring rest lengths in a flattened NxN array.
 */
void MassSpring2D::_ensure_capacity() {
    if (rest_length_flat.size() != MAX_PARTICLES * MAX_PARTICLES) {
        rest_length_flat.resize(MAX_PARTICLES * MAX_PARTICLES);
        for (int i = 0; i < rest_length_flat.size(); ++i) 
            rest_length_flat.set(i, 0.0f);
    }
}

/**
 * Reset the simulation (remove all particles and springs).
 */
void MassSpring2D::clear() {
    positions.clear();
    velocities.clear();
    fixed_flags.clear();
    _ensure_capacity();

    for (int i = 0; i < rest_length_flat.size(); ++i) 
        rest_length_flat.set(i, 0.0f);
    queue_redraw();
}

/**
 * Get particle position by index.
 */
Vector2 MassSpring2D::get_particle_position(int i) const {
    if (i >= 0 && i < positions.size())
        return positions[i];
    return Vector2();
}

/**
 * Check if a particle is fixed.
 */
bool MassSpring2D::is_particle_fixed(int i) const {
    if (i >= 0 && i < fixed_flags.size())
        return fixed_flags[i] != 0;
    return false;
}

/**
 * Get spring rest length between particle i and j.
 */
real_t MassSpring2D::get_rest_length(int i, int j) const {
    if (i < 0 || j < 0 || i >= MAX_PARTICLES || j >= MAX_PARTICLES) return 0.0;
    return rest_length_flat[_idx(i, j)];
}

/**
 * Add a new particle. 
 * If close to existing particles, connect with a spring of rest length = default_rest.
 */
void MassSpring2D::new_particle(const Vector2 &p, bool fixed) {
    _ensure_capacity();
    print_line("MassSpring2D::new_particle", p, fixed);

    const int n = positions.size();
    if (n >= MAX_PARTICLES) {
        print_error("MassSpring2D: reached MAX_PARTICLES.");
        return;
    }

    positions.push_back(p);
    velocities.push_back(Vector2());
    fixed_flags.push_back(fixed ? 1 : 0);

    // Connect springs with nearby particles
    for (int i = 0; i < n; ++i) {
        const real_t dist = (positions[n] - positions[i]).length();
        if (dist < connect_radius) {
            rest_length_flat.set(_idx(i, n), default_rest);
            rest_length_flat.set(_idx(n, i), default_rest);
        }
    }
    queue_redraw();
}

/**
 * Reset temporary force buffer.
 */
void MassSpring2D::_reset_forces_temp(PackedVector2Array &forces) {
    const int n = positions.size();
    forces.resize(n);
    for (int i = 0; i < n; ++i) 
        forces.set(i, Vector2(0, 0));
}

/**
 * Perform one integration substep:
 *  - Compute spring and damping forces
 *  - Integrate positions and velocities with drag
 *  - Clamp within [-0.5, 0.5] bounds
 */
void MassSpring2D::_integrate_one_substep() {
    const int n = positions.size();
    if (n == 0) return;

    PackedVector2Array forces;
    _reset_forces_temp(forces);

    // --- Compute forces ---
    for (int i = 0; i < n; ++i) {
        Vector2 f = Vector2(0, -9.8) * particle_mass; // gravity
        const Vector2 &xi = positions[i];
        const Vector2 &vi = velocities[i];

        for (int j = 0; j < n; ++j) {
            const float rl = rest_length_flat[_idx(i, j)];
            if (rl != 0.0f) {
                const Vector2 xij = xi - positions[j];
                const real_t len = xij.length();
                if (len > 0.0) {
                    const Vector2 d = xij / len;
                    // Hooke’s law spring force
                    f += d * (-spring_Y * (len / rl - 1.0));
                    // Dashpot damping force
                    const real_t v_rel = (vi - velocities[j]).dot(d);
                    f += d * (-dashpot_damping * v_rel);
                }
            }
        }
        forces.set(i, f);
    }

    // --- Integrate motion ---
    for (int i = 0; i < n; ++i) {
        if (fixed_flags[i] == 0) {
            Vector2 v = velocities[i] + forces[i] * (dt / particle_mass);
            v *= Math::exp(-dt * drag_damping); // exponential drag
            Vector2 x = positions[i] + v * dt;

            // Boundary conditions
            if (x.x < -0.5) { x.x = -0.5; v.x = 0; }
            if (x.x > 0.5) { x.x = 0.5; v.x = 0; }
            if (x.y < -0.5) { x.y = -0.5; v.y = 0; }
            if (x.y > 0.5) { x.y = 0.5; v.y = 0; }

            velocities.set(i, v);
            positions.set(i, x);
        } else {
            velocities.set(i, Vector2());
        }
    }
}

/**
 * Step the simulation by N steps.
 */
void MassSpring2D::step_simulation(int steps) {
    steps = MAX(1, steps);
    for (int s = 0; s < steps; ++s) 
        _integrate_one_substep();
    queue_redraw(); 
}

/**
 * Apply attraction toward a point (used for right-click interaction).
 */
void MassSpring2D::attract_to(const Vector2 &p, real_t strength) {
    const int n = positions.size();
    if (n == 0) return;
    const real_t scale = -dt * substeps * strength;
    for (int i = 0; i < n; ++i) {
        Vector2 v = velocities[i] + (positions[i] - p) * scale;
        velocities.set(i, v);
    }
}

/**
 * Called every frame during process loop.
 */
void MassSpring2D::_process(double /*delta*/) {
    if (!paused) {
        for (int s = 0; s < substeps; ++s) 
            _integrate_one_substep();
        queue_redraw();
    }
}

/**
 * Draw particles and springs.
 */
void MassSpring2D::_draw() {
    const int n = positions.size();
    for (int i = 0; i < n; ++i) {
        const Vector2 pi = positions[i] * pixel_scale;
        // Draw particle (red = fixed, black = free)
        draw_circle(pi, circle_radius_px, is_particle_fixed(i) ? Color(1,0,0) : Color(0,0,0));
        // Draw springs
        for (int j = i + 1; j < n; ++j) {
            if (rest_length_flat[_idx(i, j)] != 0.0f) {
                const Vector2 pj = positions[j] * pixel_scale;
                draw_line(pi, pj, Color(0,0,0), line_width_px);
            }
        }
    }
}

/**
 * Notification handler for Godot (ready, process, draw, etc).
 */
void MassSpring2D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _ready();
    }
    if (p_what == NOTIFICATION_PROCESS) {
        _process(0.0);
    }
    if (p_what == NOTIFICATION_DRAW) {
        _draw();
    }
}

/**
 * Initialization on ready.
 */
void MassSpring2D::_ready() {
    set_process(true); 
    set_process_input(true);
    queue_redraw();  
}

/**
 * Input handler:
 *  - Left click: add particle (shift = fixed)
 *  - Right click: attract particles
 *  - Space: pause/unpause
 *  - C: clear system
 */
void MassSpring2D::input(const Ref<InputEvent> &p_event) {
    if (const InputEventMouseButton *mb = Object::cast_to<InputEventMouseButton>(p_event.ptr())) {
        if (mb->is_pressed()) {
            Vector2 screen_pos = mb->get_position();
            Vector2 world_pos = screen_pos;
            auto spx = SpxEngine::get_singleton();
            Camera2D *camera = nullptr;

            if (spx != nullptr && spx->get_camera() != nullptr) {
                camera = spx->get_camera()->get_camera2d();
            }

            if (camera != nullptr) {
                print_line("MassSpring2D::input: using SPX camera's affine_inverse");
                world_pos = camera->get_screen_transform().affine_inverse().xform(screen_pos);
            }

            Vector2 pos_norm = world_pos / pixel_scale;
            print_line("MassSpring2D::input: mouse button", mb->get_button_index(), screen_pos, world_pos, pos_norm);

            if (mb->get_button_index() == MouseButton::LEFT) {
                const bool shift = Input::get_singleton()->is_key_pressed(Key::SHIFT);
                new_particle(pos_norm, shift);
            } else if (mb->get_button_index() == MouseButton::RIGHT) {
                attract_to(pos_norm, 100.0);
            }
        }
    }

    if (const InputEventKey *k = Object::cast_to<InputEventKey>(p_event.ptr())) {
        if (k->is_pressed()) {
            if (k->get_keycode() == Key::SPACE) {
                paused = !paused;
            } else if (k->get_keycode() == Key::C) {
                clear();
            } else if (k->get_keycode() == Key::F) {
                generate_fractal();
            }
        }
    }
}

/**
 * Generate a fractal pattern of particles (Julia set).
 */
void MassSpring2D::generate_fractal() {
    const int width = 50; 
    const int height = 50; 
    const int max_iter = 20;
    const float zoom = 1.5f;
    const Vector2 offset(0.0f, 0.0f);
    const std::complex<float> c(-0.7f, 0.27015f); 

    clear(); 

    for (int ix = 0; ix < width; ix += 2) { 
        for (int iy = 0; iy < height; iy += 2) {
            float x0 = ((float)ix / width) * 2.0f * zoom - zoom + offset.x;
            float y0 = ((float)iy / height) * 2.0f * zoom - zoom + offset.y;
            std::complex<float> z(x0, y0);

            int iter = 0;
            while (abs(z) < 2.0f && iter < max_iter) {
                z = z * z + c;
                iter++;
            }

            if (iter == max_iter) { 
                Vector2 pos_norm(x0, y0);
                bool is_fixed = (Math::randf() < 0.2);
                new_particle(pos_norm, is_fixed);
            }
        }
    }
}
