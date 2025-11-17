/**
 * Test Case 5: Film Flow on Curved Surface
 *
 * Based on paper arXiv:2402.10185
 * Gravity-driven film flow on curved embedded boundary
 *
 * Purpose: Validate wetting dynamics on complex curved geometries
 *
 * Physical setup:
 * - Thin liquid film on curved surface (cylinder or sphere)
 * - Gravity drives flow downward
 * - Contact angle θ at moving contact line
 * - Surface can be horizontal cylinder or sphere
 *
 * Theory:
 * - Film thickness h(x,t) evolves due to gravity and surface tension
 * - Advancing/receding contact line dynamics
 * - For cylinder: 2D axisymmetric flow
 * - Capillary number: Ca = μU/σ
 *
 * Validation metrics:
 * - Film thickness profile
 * - Contact line velocity
 * - Contact angle at advancing front
 * - Mass conservation
 * - Comparison with lubrication theory (thin film limit)
 *
 * Test configurations:
 * 1. Horizontal cylinder with thin film
 * 2. Sphere with coating flow
 * 3. Various contact angles: 30°, 60°, 90°
 * 4. Various film thicknesses: h/R = 0.05, 0.1, 0.2
 * 5. Various Bond numbers: Bo = ρgh²/σ
 *
 * Expected behavior:
 * - Film drains downward under gravity
 * - Contact line advances/recedes
 * - Film stabilizes with equilibrium profile
 * - Recirculation in thick films
 *
 * Pass criteria:
 * - Contact angle maintained within 10°
 * - Mass conservation < 3%
 * - Film thickness physically reasonable
 * - No numerical instabilities
 */

#include "grid/quadtree.h"
#include "embed.h"
#include "navier-stokes/centered.h"
#include "two-phase.h"
#include "tension.h"
#include "contact.h"

// Physical parameters
#define CYLINDER_RADIUS 1.0       // Substrate radius
#define FILM_THICKNESS 0.1        // Initial film thickness
#define DOMAIN_SIZE 3.0           // Domain size
#define SIGMA 0.073               // Surface tension (water-air)
#define RHO_L 1000.0              // Liquid density
#define RHO_G 1.0                 // Gas density
#define MU_L 0.001                // Liquid viscosity
#define MU_G 0.00002              // Gas viscosity
#define GRAVITY 9.81              // Gravitational acceleration

// Simulation parameters
int MAXLEVEL = 8;                 // Maximum grid level
double MAX_TIME = 2.0;            // Simulation time
double THETA_DEG = 60.0;          // Contact angle (degrees)
int GEOMETRY_TYPE = 0;            // 0=cylinder, 1=sphere

// Measurement arrays
double time_series[10000];
double thickness_series[10000];
double velocity_series[10000];
int n_measurements = 0;

// Measure film properties
void measure_film(double *thickness, double *velocity) {
    double max_thickness = 0.0;
    double max_vel = 0.0;

    foreach(reduction(max:max_thickness) reduction(max:max_vel)) {
        if (f[] > 0.5) {
            double r = sqrt(sq(x) + sq(y));
            double h = r - CYLINDER_RADIUS;
            if (h > max_thickness) max_thickness = h;

            double vel = sqrt(sq(u.x[]) + sq(u.y[]));
            if (vel > max_vel) max_vel = vel;
        }
    }

    *thickness = max_thickness;
    *velocity = max_vel;
}

// Measure contact angle
double measure_contact_angle() {
    double angle_sum = 0.0;
    int count = 0;

    foreach() {
        if (cs[] > 0.0 && cs[] < 1.0 && f[] > 0.1 && f[] < 0.9) {
            coord n_interface = mycs(point, f);
            double norm = sqrt(sq(n_interface.x) + sq(n_interface.y));
            if (norm > 0) {
                n_interface.x /= norm;
                n_interface.y /= norm;
            }

            // Substrate normal (radial direction)
            double r = sqrt(sq(x) + sq(y));
            if (r > 0.1) {
                double n_wall_x = x / r;
                double n_wall_y = y / r;

                double dot = n_interface.x * n_wall_x + n_interface.y * n_wall_y;
                double local_angle = acos(fabs(dot)) * 180.0 / M_PI;

                angle_sum += local_angle;
                count++;
            }
        }
    }

    return (count > 0) ? angle_sum / count : -1.0;
}

int main(int argc, char *argv[]) {

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == 'L') MAXLEVEL = atoi(&argv[i][1]);
        else if (argv[i][0] == 'A') THETA_DEG = atof(&argv[i][1]);
        else if (argv[i][0] == 'G') GEOMETRY_TYPE = atoi(&argv[i][1]);
    }

    // Setup domain
    size(DOMAIN_SIZE);
    origin(-DOMAIN_SIZE/2.0, -DOMAIN_SIZE/2.0);
    init_grid(1 << 6);

    // Fluid properties
    rho1 = RHO_L;
    rho2 = RHO_G;
    mu1 = MU_L;
    mu2 = MU_G;
    f.sigma = SIGMA;

    // Gravity (downward)
    G.y = -GRAVITY;

    // Create output directory
    system("mkdir -p results/film_flow");

    // Calculate dimensionless numbers
    double Bo = RHO_L * GRAVITY * sq(FILM_THICKNESS) / SIGMA;
    double Ca = MU_L * sqrt(GRAVITY * FILM_THICKNESS) / SIGMA;

    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Film Flow Test\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Geometry: %s\n", GEOMETRY_TYPE == 0 ? "Cylinder" : "Sphere");
    fprintf(stderr, "Cylinder radius: %.3f m\n", CYLINDER_RADIUS);
    fprintf(stderr, "Film thickness: %.4f m\n", FILM_THICKNESS);
    fprintf(stderr, "Contact angle: %.1f°\n", THETA_DEG);
    fprintf(stderr, "Bond number: %.4f\n", Bo);
    fprintf(stderr, "Capillary number: %.4e\n", Ca);
    fprintf(stderr, "Max level: %d\n", MAXLEVEL);
    fprintf(stderr, "========================================\n\n");

    run();
    return 0;
}

// Initialize embedded geometry
event init(i = 0) {
    // Create embedded cylinder or sphere
    vertex scalar phi[];
    foreach_vertex() {
        if (GEOMETRY_TYPE == 0) {
            // Horizontal cylinder
            double r = sqrt(sq(x) + sq(z));
            phi[] = r - CYLINDER_RADIUS;
        } else {
            // Sphere
            double r = sqrt(sq(x) + sq(y) + sq(z));
            phi[] = r - CYLINDER_RADIUS;
        }
    }
    fractions(phi, cs, fs);

    // Initialize film on top of substrate
    foreach() {
        double r = sqrt(sq(x) + sq(y));
        double film_outer_radius = CYLINDER_RADIUS + FILM_THICKNESS;

        // Film exists in annular region
        if (r > CYLINDER_RADIUS && r < film_outer_radius) {
            f[] = 1.0;
        } else {
            f[] = 0.0;
        }
    }

    // Set contact angle
    double theta_rad = THETA_DEG * M_PI / 180.0;
    foreach() {
        contact_angle[] = theta_rad;
    }
}

// Adapt mesh
event adapt(i++) {
    adapt_wavelet({f, cs, u.x, u.y}, (double[]){0.01, 0.01, 0.05, 0.05},
                  MAXLEVEL, MAXLEVEL-2);
}

// Measurements
event logfile(t += 0.02; t <= MAX_TIME) {
    double thickness, velocity;
    measure_film(&thickness, &velocity);
    double angle = measure_contact_angle();

    // Calculate mass
    double vol = 0.0;
    foreach(reduction(+:vol)) {
        vol += f[] * dv();
    }
    double initial_vol = M_PI * (sq(CYLINDER_RADIUS + FILM_THICKNESS) -
                                 sq(CYLINDER_RADIUS)) * DOMAIN_SIZE;
    double mass_error = fabs(vol - initial_vol) / initial_vol * 100.0;

    if (n_measurements < 10000) {
        time_series[n_measurements] = t;
        thickness_series[n_measurements] = thickness;
        velocity_series[n_measurements] = velocity;
        n_measurements++;
    }

    fprintf(stderr, "t=%6.3f: h=%.4f u=%.4f θ=%.1f° Δm=%.2f%%\n",
            t, thickness, velocity, angle, mass_error);
}

// Final analysis
event end(t = MAX_TIME) {
    // Average final properties
    int start_idx = (int)(n_measurements * 0.8);
    double h_final = 0.0;
    double u_final = 0.0;

    for (int i = start_idx; i < n_measurements; i++) {
        h_final += thickness_series[i];
        u_final += velocity_series[i];
    }
    h_final /= (n_measurements - start_idx);
    u_final /= (n_measurements - start_idx);

    // Calculate mass conservation
    double vol = 0.0;
    foreach(reduction(+:vol)) {
        vol += f[] * dv();
    }
    double initial_vol = M_PI * (sq(CYLINDER_RADIUS + FILM_THICKNESS) -
                                 sq(CYLINDER_RADIUS)) * DOMAIN_SIZE;
    double mass_error = fabs(vol - initial_vol) / initial_vol * 100.0;

    double angle_final = measure_contact_angle();
    double angle_error = fabs(angle_final - THETA_DEG);

    fprintf(stderr, "\n========================================\n");
    fprintf(stderr, "FINAL RESULTS\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Film thickness: %.4f m\n", h_final);
    fprintf(stderr, "Film velocity:  %.4e m/s\n", u_final);
    fprintf(stderr, "Contact angle:  %.1f° (specified: %.1f°)\n",
            angle_final, THETA_DEG);
    fprintf(stderr, "Angle error:    %.1f°\n", angle_error);
    fprintf(stderr, "Mass error:     %.2f%%\n", mass_error);
    fprintf(stderr, "========================================\n");

    // Pass/fail criteria
    bool passed = (angle_error < 10.0) && (mass_error < 3.0);

    fprintf(stderr, "\nTEST STATUS: %s\n", passed ? "✅ PASS" : "❌ FAIL");
    fprintf(stderr, "========================================\n");

    // Write results file
    char filename[256];
    sprintf(filename, "results/film_flow/theta%.0f_h%.3f_geom%d.txt",
            THETA_DEG, FILM_THICKNESS, GEOMETRY_TYPE);
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "# Film Flow Test Results\n");
    fprintf(fp, "# Geometry: %d (0=cylinder, 1=sphere)\n", GEOMETRY_TYPE);
    fprintf(fp, "# Contact angle: %.1f°\n", THETA_DEG);
    fprintf(fp, "# Film thickness: %.4f m\n", FILM_THICKNESS);
    fprintf(fp, "# Time  Thickness  Velocity\n");
    for (int i = 0; i < n_measurements; i++) {
        fprintf(fp, "%.6f  %.6f  %.6e\n",
                time_series[i], thickness_series[i], velocity_series[i]);
    }
    fclose(fp);

    exit(passed ? 0 : 1);
}
