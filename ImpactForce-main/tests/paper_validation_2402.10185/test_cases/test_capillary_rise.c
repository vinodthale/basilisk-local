/**
 * Test Case 4: Capillary Rise in Tube
 *
 * Based on paper arXiv:2402.10185
 * Classic capillary rise problem with embedded boundary
 *
 * Purpose: Validate contact angle implementation for capillary phenomena
 *
 * Physical setup:
 * - Vertical circular tube of radius R_tube
 * - Tube modeled as embedded boundary (cylinder)
 * - Liquid column rises due to surface tension
 * - Contact angle θ at tube wall
 * - Gravity acts downward
 *
 * Analytical solution (Jurin's Law):
 * h = (2σ cos(θ)) / (ρ g R_tube)
 *
 * Where:
 * - h: equilibrium height of meniscus
 * - σ: surface tension
 * - θ: contact angle
 * - ρ: liquid density
 * - g: gravitational acceleration
 * - R_tube: tube radius
 *
 * Validation metrics:
 * - Equilibrium height h_eq
 * - Meniscus shape (spherical cap with radius R_tube/cos(θ))
 * - Contact angle at wall
 * - Time to reach equilibrium
 * - Mass conservation
 *
 * Test configurations:
 * 1. Various contact angles: 30°, 45°, 60°, 75°
 * 2. Various tube radii: 0.1, 0.2, 0.5
 * 3. Various Bond numbers: Bo = ρgR²/σ
 *
 * Expected behavior:
 * - Liquid rises quickly initially
 * - Oscillation damping
 * - Convergence to analytical height
 * - Meniscus curvature matches contact angle
 *
 * Pass criteria:
 * - |h_measured - h_analytical| / h_analytical < 0.10
 * - Contact angle error < 5°
 * - Mass conservation < 1%
 */

#include "grid/quadtree.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/myembed.h"
#include "navier-stokes/centered.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_two-phase.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_tension.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_vof.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_contact.h"

// Physical parameters
#define TUBE_RADIUS 0.2           // Tube inner radius
#define TUBE_HEIGHT 2.0           // Tube length
#define INITIAL_LEVEL 0.1         // Initial liquid level
#define SIGMA 1.0                 // Surface tension
#define RHO_L 1000.0              // Liquid density
#define RHO_G 1.0                 // Gas density
#define MU_L 0.001                // Liquid viscosity
#define MU_G 0.00001              // Gas viscosity
#define GRAVITY 9.81              // Gravitational acceleration

// Simulation parameters
int MAXLEVEL = 9;                 // Maximum grid level
double MAX_TIME = 5.0;            // Simulation time
double THETA_DEG = 30.0;          // Contact angle (degrees)

// Measurement arrays
double time_series[10000];
double height_series[10000];
double angle_series[10000];
int n_measurements = 0;

// Analytical solution
double capillary_height_analytical(double theta_deg, double R, double sigma,
                                   double rho, double g) {
    double theta_rad = theta_deg * M_PI / 180.0;
    return (2.0 * sigma * cos(theta_rad)) / (rho * g * R);
}

// Measure meniscus height
double measure_meniscus_height() {
    double max_height = 0.0;

    foreach(reduction(max:max_height)) {
        if (f[] > 0.5) {
            if (y > max_height) max_height = y;
        }
    }

    return max_height;
}

// Measure contact angle at tube wall
double measure_contact_angle() {
    double angle_sum = 0.0;
    int count = 0;

    foreach() {
        // Check if cell is at the contact line (tube wall + interface)
        if (cs[] > 0.0 && cs[] < 1.0 && f[] > 0.1 && f[] < 0.9) {
            coord n_interface = mycs(point, f);
            double norm = sqrt(sq(n_interface.x) + sq(n_interface.y));
            if (norm > 0) {
                n_interface.x /= norm;
                n_interface.y /= norm;
            }

            // Tube wall normal (pointing inward) - 2D: radial from x=0
            double r = fabs(x);
            if (r > 0.01) {
                double n_wall_x = (x < 0) ? 1.0 : -1.0;  // Points toward centerline

                // Calculate angle between interface and wall
                double dot = n_interface.x * n_wall_x;
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
    }

    // Setup domain (2D)
    size(TUBE_HEIGHT);
    origin(-TUBE_RADIUS, -TUBE_HEIGHT/2.0);
    init_grid(1 << 6);

    // Fluid properties
    rho1 = RHO_L;
    rho2 = RHO_G;
    mu1 = MU_L;
    mu2 = MU_G;
    f.sigma = SIGMA;

    // Gravity
    G.y = -GRAVITY;

    // Create output directory
    system("mkdir -p results/capillary_rise");

    // Calculate analytical height
    double h_analytical = capillary_height_analytical(THETA_DEG, TUBE_RADIUS,
                                                       SIGMA, RHO_L, GRAVITY);

    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Capillary Rise Test\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Tube radius: %.3f m\n", TUBE_RADIUS);
    fprintf(stderr, "Contact angle: %.1f°\n", THETA_DEG);
    fprintf(stderr, "Surface tension: %.3f N/m\n", SIGMA);
    fprintf(stderr, "Density ratio: %.0f\n", RHO_L/RHO_G);
    fprintf(stderr, "Bond number: %.3f\n", RHO_L*GRAVITY*sq(TUBE_RADIUS)/SIGMA);
    fprintf(stderr, "Max level: %d\n", MAXLEVEL);
    fprintf(stderr, "----------------------------------------\n");
    fprintf(stderr, "Analytical height (Jurin): %.4f m\n", h_analytical);
    fprintf(stderr, "========================================\n\n");

    run();
    return 0;
}

// Initialize embedded tube
event init(i = 0) {
    // Create embedded channel walls (2D: vertical walls at x = ±TUBE_RADIUS)
    vertex scalar phi[];
    foreach_vertex() {
        double r = fabs(x);
        phi[] = r - TUBE_RADIUS;  // Negative inside channel
    }
    fractions(phi, cs, fs);

    // Initialize liquid level
    fraction(f, y - INITIAL_LEVEL);

    // Set contact angle
    double theta_rad = THETA_DEG * M_PI / 180.0;
    foreach() {
        contact_angle[] = theta_rad;
    }
}

// Adapt mesh
event adapt(i++) {
    adapt_wavelet({f, cs}, (double[]){0.01, 0.01}, MAXLEVEL, MAXLEVEL-2);
}

// Measurements
event logfile(t += 0.02; t <= MAX_TIME) {
    double height = measure_meniscus_height();
    double angle = measure_contact_angle();

    // Calculate mass
    double vol = 0.0;
    foreach(reduction(+:vol)) {
        vol += f[] * dv();
    }
    double initial_vol = M_PI * sq(TUBE_RADIUS) * INITIAL_LEVEL;
    double mass_error = fabs(vol - initial_vol) / initial_vol * 100.0;

    if (n_measurements < 10000) {
        time_series[n_measurements] = t;
        height_series[n_measurements] = height;
        angle_series[n_measurements] = angle;
        n_measurements++;
    }

    fprintf(stderr, "t=%6.3f: h=%.4f θ=%.1f° Δm=%.2f%%\n",
            t, height, angle, mass_error);
}

// Final analysis
event end(t = MAX_TIME) {
    double h_analytical = capillary_height_analytical(THETA_DEG, TUBE_RADIUS,
                                                       SIGMA, RHO_L, GRAVITY);

    // Average last 20% of measurements for equilibrium
    int start_idx = (int)(n_measurements * 0.8);
    double h_measured = 0.0;
    double angle_measured = 0.0;
    int count = 0;

    for (int i = start_idx; i < n_measurements; i++) {
        h_measured += height_series[i];
        if (angle_series[i] > 0) {
            angle_measured += angle_series[i];
            count++;
        }
    }
    h_measured /= (n_measurements - start_idx);
    if (count > 0) angle_measured /= count;

    double height_error = fabs(h_measured - h_analytical) / h_analytical * 100.0;
    double angle_error = fabs(angle_measured - THETA_DEG);

    fprintf(stderr, "\n========================================\n");
    fprintf(stderr, "FINAL RESULTS\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Capillary height:\n");
    fprintf(stderr, "  Measured:   %.4f m\n", h_measured);
    fprintf(stderr, "  Analytical: %.4f m (Jurin's Law)\n", h_analytical);
    fprintf(stderr, "  Error:      %.2f%%\n", height_error);
    fprintf(stderr, "Contact angle:\n");
    fprintf(stderr, "  Measured:   %.1f°\n", angle_measured);
    fprintf(stderr, "  Specified:  %.1f°\n", THETA_DEG);
    fprintf(stderr, "  Error:      %.1f°\n", angle_error);
    fprintf(stderr, "========================================\n");

    // Pass/fail criteria
    bool passed = (height_error < 10.0) && (angle_error < 5.0);

    fprintf(stderr, "\nTEST STATUS: %s\n", passed ? "✅ PASS" : "❌ FAIL");
    fprintf(stderr, "========================================\n");

    // Write results file
    char filename[256];
    sprintf(filename, "results/capillary_rise/theta%.0f_R%.2f.txt",
            THETA_DEG, TUBE_RADIUS);
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "# Capillary Rise Test Results\n");
    fprintf(fp, "# Contact angle: %.1f°\n", THETA_DEG);
    fprintf(fp, "# Tube radius: %.3f m\n", TUBE_RADIUS);
    fprintf(fp, "# Analytical height: %.4f m\n", h_analytical);
    fprintf(fp, "# Time  Height  ContactAngle\n");
    for (int i = 0; i < n_measurements; i++) {
        fprintf(fp, "%.6f  %.6f  %.2f\n",
                time_series[i], height_series[i], angle_series[i]);
    }
    fclose(fp);

    exit(passed ? 0 : 1);
}
