#pragma once

#include "core/object/class_db.h"
#include "scene/2d/node_2d.h"
#include "core/variant/variant.h"
#include <complex>

struct InputData
{
    Vector2 pos;
    bool is_fixed;
};

class FractalInput{
private:
    static constexpr int width = 50; 
    static constexpr int height = 50; 
    static constexpr int max_iter = 20;
    static constexpr float zoom = 1.5f;
    const Vector2 offset{0.0f, 0.0f};
    const std::complex<float> c{-0.7f, 0.27015f}; 
public:
    FractalInput() = default;
    ~FractalInput() = default;

    Vector<InputData> generate() {
        Vector<InputData> result;

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
                    result.append({pos_norm, is_fixed});
                }
            }
        }

        return result;
    }
};


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
    PackedVector2Array positions;    
    PackedVector2Array velocities;   
    PackedInt32Array   fixed_flags;  

    // Flattened rest-lengths for springs between particles (i, j)
    PackedFloat32Array rest_length_flat;

    // Physical parameters
    real_t spring_Y        = 1000.0;  
    real_t drag_damping    = 1.0;     
    real_t dashpot_damping = 100.0;   
    real_t particle_mass   = 1.0;     

    // Integration parameters
    real_t dt        = 1e-3;   
    int    substeps  = 10;     

    // Simulation control
    bool paused           = false;  
    real_t connect_radius = 0.15;   
    real_t default_rest   = 0.1;    
    real_t pixel_scale    = 1086.0; 

    // Max capacity (preallocated size for rest-length matrix)
    static constexpr int MAX_PARTICLES = 1024;

private:
    // Godot binding
    static void _bind_methods();

    // Helper: index into flattened rest length array
    _ALWAYS_INLINE_ int _idx(int i, int j) const { return i * MAX_PARTICLES + j; }

    // Internal helpers
    void _ensure_capacity();  
    void _integrate_one_substep(); 
    void _reset_forces_temp(PackedVector2Array &forces);

protected:
    void _process(double delta); 
    void _draw();  
    void _notification(int p_what);
    void _ready();
    void input(const Ref<InputEvent> &p_event) override;

    void fractal_drive();
public:
    MassSpring2D() = default;
    ~MassSpring2D() = default;

    FractalInput special_input;

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
    Vector2 get_particle_position(int i) const; 
    bool is_particle_fixed(int i) const; 
    real_t get_rest_length(int i, int j) const; 

    // ---- Simulation control ----
    void clear();  
    void new_particle(const Vector2 &p, bool fixed); 
    void attract_to(const Vector2 &p, real_t strength = 100.0); 
    void step_simulation(int steps = 1);  

    // ---- Drawing parameters ----
    real_t circle_radius_px = 5.0; 
    real_t line_width_px    = 2.0; 
};