#pragma once

#include "core/object/class_db.h"
#include "scene/2d/node_2d.h"
#include "core/variant/variant.h"

/**
 * MassSpring2D
 *
 * A simple 2D mass-spring simulation node.
 * - Supports particle creation, spring connections, damping, and user interaction.
 * - Designed for real-time interactive physics experiments.
 */
class MassSpring2D : public Node2D {
    GDCLASS(MassSpring2D, Node2D);

private:
    // Particle state arrays
    PackedVector2Array positions;    // Positions of particles
    PackedVector2Array velocities;   // Velocities of particles
    PackedInt32Array   fixed_flags;  // 0 = free, 1 = fixed

    // Flattened rest-lengths for springs between particles (i, j)
    PackedFloat32Array rest_length_flat;

    // Physical parameters
    real_t spring_Y        = 1000.0;  // Spring stiffness (Hooke's constant)
    real_t drag_damping    = 1.0;     // Linear drag damping coefficient
    real_t dashpot_damping = 100.0;   // Dashpot damping (velocity-based spring damping)
    real_t particle_mass   = 1.0;     // Mass per particle

    // Integration parameters
    real_t dt        = 1e-3;   // Time step
    int    substeps  = 10;     // Number of sub-steps per frame for stability

    // Simulation control
    bool paused           = false;  // Whether simulation is paused
    real_t connect_radius = 0.15;   // Max distance for auto-connecting springs
    real_t default_rest   = 0.1;    // Default spring rest length
    real_t pixel_scale    = 1086.0; // World-to-screen scale factor

    // Max capacity (preallocated size for rest-length matrix)
    static constexpr int MAX_PARTICLES = 1024;

private:
    // Godot binding
    static void _bind_methods();

    // Helper: index into flattened rest length array
    _ALWAYS_INLINE_ int _idx(int i, int j) const { return i * MAX_PARTICLES + j; }

    // Internal helpers
    void _ensure_capacity();                         // Ensure arrays are large enough
    void _integrate_one_substep();                   // Perform one physics integration substep
    void _reset_forces_temp(PackedVector2Array &forces); // Clear temporary force buffer

protected:
    // Overridden Godot methods
    void _process(double delta); 
    void _draw();  
    void _notification(int p_what);
    void _ready();
    void input(const Ref<InputEvent> &p_event) override;

public:
    MassSpring2D() = default;
    ~MassSpring2D() = default;

    // ---- Setters & Getters for parameters ----
    void set_spring_Y(real_t v)        { spring_Y = v; }
    real_t get_spring_Y() const        { return spring_Y; }

    void set_drag_damping(real_t v)    { drag_damping = v; }
    real_t get_drag_damping() const    { return drag_damping; }

    void set_dashpot_damping(real_t v) { dashpot_damping = v; }
    real_t get_dashpot_damping() const { return dashpot_damping; }

    void set_pixel_scale(real_t v)     { pixel_scale = v; }
    real_t get_pixel_scale() const     { return pixel_scale; }

    void set_connect_radius(real_t v)  { connect_radius = v; }
    real_t get_connect_radius() const  { return connect_radius; }

    void set_default_rest(real_t v)    { default_rest = v; }
    real_t get_default_rest() const    { return default_rest; }

    void set_paused(bool p)            { paused = p; }
    bool is_paused() const             { return paused; }

    // ---- Query functions ----
    int get_num_particles() const               { return positions.size(); }
    Vector2 get_particle_position(int i) const; // Get particle position
    bool is_particle_fixed(int i) const;        // Is particle fixed in space
    real_t get_rest_length(int i, int j) const; // Get spring rest length between (i, j)

    // ---- Simulation control ----
    void clear();                                 // Clear all particles and springs
    void new_particle(const Vector2 &p, bool fixed); // Add new particle
    void attract_to(const Vector2 &p, real_t strength = 100.0); // Apply attraction force
    void step_simulation(int steps = 1);          // Advance simulation manually

    // ---- Drawing parameters ----
    real_t circle_radius_px = 5.0; // Particle circle radius in pixels
    real_t line_width_px    = 2.0; // Spring line width in pixels
};