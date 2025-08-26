#include "spx_mass_spring.h"
#include "core/os/keyboard.h"

void MassSpring2D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_spring_Y", "v"), &MassSpring2D::set_spring_Y);
    ClassDB::bind_method(D_METHOD("get_spring_Y"),       &MassSpring2D::get_spring_Y);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spring_Y", PROPERTY_HINT_RANGE, "10,10000,1,or_greater"), "set_spring_Y", "get_spring_Y");

    ClassDB::bind_method(D_METHOD("set_drag_damping", "v"), &MassSpring2D::set_drag_damping);
    ClassDB::bind_method(D_METHOD("get_drag_damping"),       &MassSpring2D::get_drag_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drag_damping", PROPERTY_HINT_RANGE, "0,10,0.01"), "set_drag_damping", "get_drag_damping");

    ClassDB::bind_method(D_METHOD("set_dashpot_damping", "v"), &MassSpring2D::set_dashpot_damping);
    ClassDB::bind_method(D_METHOD("get_dashpot_damping"),       &MassSpring2D::get_dashpot_damping);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dashpot_damping", PROPERTY_HINT_RANGE, "0,2000,1,or_greater"), "set_dashpot_damping", "get_dashpot_damping");

    ClassDB::bind_method(D_METHOD("set_pixel_scale", "v"), &MassSpring2D::set_pixel_scale);
    ClassDB::bind_method(D_METHOD("get_pixel_scale"),       &MassSpring2D::get_pixel_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixel_scale", PROPERTY_HINT_RANGE, "100,4096,1"), "set_pixel_scale", "get_pixel_scale");

    ClassDB::bind_method(D_METHOD("set_connect_radius", "v"), &MassSpring2D::set_connect_radius);
    ClassDB::bind_method(D_METHOD("get_connect_radius"),       &MassSpring2D::get_connect_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "connect_radius", PROPERTY_HINT_RANGE, "0.01,1,0.01"), "set_connect_radius", "get_connect_radius");

    ClassDB::bind_method(D_METHOD("set_default_rest", "v"), &MassSpring2D::set_default_rest);
    ClassDB::bind_method(D_METHOD("get_default_rest"),       &MassSpring2D::get_default_rest);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_rest", PROPERTY_HINT_RANGE, "0.01,1,0.01"), "set_default_rest", "get_default_rest");

    ClassDB::bind_method(D_METHOD("set_paused", "p"), &MassSpring2D::set_paused);
    ClassDB::bind_method(D_METHOD("is_paused"),       &MassSpring2D::is_paused);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "paused"), "set_paused", "is_paused");

    ClassDB::bind_method(D_METHOD("get_num_particles"), &MassSpring2D::get_num_particles);
    ClassDB::bind_method(D_METHOD("get_particle_position", "i"), &MassSpring2D::get_particle_position);
    ClassDB::bind_method(D_METHOD("is_particle_fixed", "i"),     &MassSpring2D::is_particle_fixed);
    ClassDB::bind_method(D_METHOD("get_rest_length", "i", "j"),  &MassSpring2D::get_rest_length);

    ClassDB::bind_method(D_METHOD("clear"), &MassSpring2D::clear);
    ClassDB::bind_method(D_METHOD("new_particle", "p", "fixed"), &MassSpring2D::new_particle);
    ClassDB::bind_method(D_METHOD("attract_to", "p", "strength"), &MassSpring2D::attract_to);
    ClassDB::bind_method(D_METHOD("step_simulation", "steps"), &MassSpring2D::step_simulation);

}

void MassSpring2D::_ensure_capacity() {
    if (rest_length_flat.size() != MAX_PARTICLES * MAX_PARTICLES) {
        rest_length_flat.resize(MAX_PARTICLES * MAX_PARTICLES);

        for (int i = 0; i < rest_length_flat.size(); ++i) rest_length_flat.set(i, 0.0f);
    }
}

void MassSpring2D::clear() {
    positions.clear();
    velocities.clear();
    fixed_flags.clear();
    _ensure_capacity();

    for (int i = 0; i < rest_length_flat.size(); ++i) rest_length_flat.set(i, 0.0f);
    queue_redraw();
}

Vector2 MassSpring2D::get_particle_position(int i) const {
    if (i >= 0 && i < positions.size())
        return positions[i];
    return Vector2();
}

bool MassSpring2D::is_particle_fixed(int i) const {
    if (i >= 0 && i < fixed_flags.size())
        return fixed_flags[i] != 0;
    return false;
}

real_t MassSpring2D::get_rest_length(int i, int j) const {
    if (i < 0 || j < 0 || i >= MAX_PARTICLES || j >= MAX_PARTICLES) return 0.0;
    return rest_length_flat[_idx(i, j)];
}

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

    for (int i = 0; i < n; ++i) {
        const real_t dist = (positions[n] - positions[i]).length();
        if (dist < connect_radius) {
            rest_length_flat.set(_idx(i, n), default_rest);
            rest_length_flat.set(_idx(n, i), default_rest);
        }
    }
    queue_redraw();
}

void MassSpring2D::_reset_forces_temp(PackedVector2Array &forces) {
    const int n = positions.size();
    forces.resize(n);
    for (int i = 0; i < n; ++i) forces.set(i, Vector2(0, 0));
}

void MassSpring2D::_integrate_one_substep() {
    const int n = positions.size();
    if (n == 0) return;

    PackedVector2Array forces;
    _reset_forces_temp(forces);

    for (int i = 0; i < n; ++i) {
        Vector2 f = Vector2(0, -9.8) * particle_mass; // 重力
        const Vector2 &xi = positions[i];
        const Vector2 &vi = velocities[i];
        for (int j = 0; j < n; ++j) {
            const float rl = rest_length_flat[_idx(i, j)];
            if (rl != 0.0f) {
                const Vector2 xij = xi - positions[j];
                const real_t len = xij.length();
                if (len > 0.0) {
                    const Vector2 d = xij / len;
                    f += d * (-spring_Y * (len / rl - 1.0));
                    const real_t v_rel = (vi - velocities[j]).dot(d);
                    f += d * (-dashpot_damping * v_rel);
                }
            }
        }
        forces.set(i, f);
    }

    for (int i = 0; i < n; ++i) {
        if (fixed_flags[i] == 0) {
            Vector2 v = velocities[i] + forces[i] * (dt / particle_mass);
            v *= Math::exp(-dt * drag_damping);
            Vector2 x = positions[i] + v * dt;

            if (x.x < 0) { x.x = 0; v.x = 0; }
            if (x.x > 1) { x.x = 1; v.x = 0; }
            if (x.y < 0) { x.y = 0; v.y = 0; }
            if (x.y > 1) { x.y = 1; v.y = 0; }

            velocities.set(i, v);
            positions.set(i, x);
        } else {
            velocities.set(i, Vector2());
        }
    }
}

void MassSpring2D::step_simulation(int steps) {
    steps = MAX(1, steps);
    for (int s = 0; s < steps; ++s) _integrate_one_substep();
    queue_redraw(); 
}

void MassSpring2D::attract_to(const Vector2 &p, real_t strength) {
    const int n = positions.size();
    if (n == 0) return;
    const real_t scale = -dt * substeps * strength;
    for (int i = 0; i < n; ++i) {
        Vector2 v = velocities[i] + (positions[i] - p) * scale;
        velocities.set(i, v);
    }
}

void MassSpring2D::_process(double /*delta*/) {
    if (!paused) {
        for (int s = 0; s < substeps; ++s) _integrate_one_substep();
        queue_redraw();
    }
}

void MassSpring2D::_draw() {
    const int n = positions.size();
    for (int i = 0; i < n; ++i) {
        const Vector2 pi = positions[i] * pixel_scale;
        draw_circle(pi, circle_radius_px, is_particle_fixed(i) ? Color(1,0,0) : Color(0,0,0));
        for (int j = i + 1; j < n; ++j) {
            if (rest_length_flat[_idx(i, j)] != 0.0f) {
                const Vector2 pj = positions[j] * pixel_scale;
                draw_line(pi, pj, Color(0,0,0), line_width_px);
            }
        }
    }
}

void MassSpring2D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _ready();
    }
    if (p_what == NOTIFICATION_PROCESS) {
        _process(0.0);
    }
    if(p_what == NOTIFICATION_DRAW) {
        _draw();
    }
}

void MassSpring2D::_ready() {
    print_line("MassSpring2D::_ready");
    set_process(true); 
    set_process_input(true);
    queue_redraw();  
}

void MassSpring2D::input(const Ref<InputEvent> &p_event) {
    if (const InputEventMouseButton *mb = Object::cast_to<InputEventMouseButton>(p_event.ptr())) {
        if (mb->is_pressed()) {
            Vector2 pos_norm = mb->get_position() / pixel_scale;
            print_line("MassSpring2D::input", mb->get_button_index(), mb->get_position() , pos_norm);
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
            }
        }
    }
}
