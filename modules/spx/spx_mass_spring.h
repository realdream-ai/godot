#pragma once

#include "core/object/class_db.h"
#include "scene/2d/node_2d.h"
#include "core/variant/variant.h"

class MassSpring2D : public Node2D {
    GDCLASS(MassSpring2D, Node2D);

private:
    PackedVector2Array positions;
    PackedVector2Array velocities;
    PackedInt32Array   fixed_flags;     // 0/1

    PackedFloat32Array rest_length_flat;

    real_t spring_Y        = 1000.0;  
    real_t drag_damping    = 1.0;     
    real_t dashpot_damping = 100.0;   
    real_t particle_mass   = 1.0;


    real_t dt        = 1e-3;
    int    substeps  = 10;

    bool paused           = false;
    real_t connect_radius = 0.15; 
    real_t default_rest   = 0.1;  
    real_t pixel_scale    = 1086.0; 

    static constexpr int MAX_PARTICLES = 1024;

private:
    static void _bind_methods();

    _ALWAYS_INLINE_ int _idx(int i, int j) const { return i * MAX_PARTICLES + j; }
    void _ensure_capacity(); 
    void _integrate_one_substep(); 
    void _reset_forces_temp(PackedVector2Array &forces); 

protected:
    void _process(double delta);
    void _draw();
    void _notification(int p_what);
    void _ready();
    void input(const Ref<InputEvent> &p_event) override;

public:
    MassSpring2D() = default;
    ~MassSpring2D() = default;

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

    int  get_num_particles() const     { return positions.size(); }
    Vector2 get_particle_position(int i) const;
    bool is_particle_fixed(int i) const;
    real_t get_rest_length(int i, int j) const;

    void clear();
    void new_particle(const Vector2 &p, bool fixed);
    void attract_to(const Vector2 &p, real_t strength = 100.0);
    void step_simulation(int steps = 1);

    real_t circle_radius_px = 5.0;
    real_t line_width_px    = 2.0;
};
