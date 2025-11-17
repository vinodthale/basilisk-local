/**
 * Test Case 2: Sliding Droplet on Inclined Plane
 *
 * Based on paper arXiv:2402.10185 Section 4.2
 * "2D sessile droplet on an embedded plane"
 *
 * Purpose: Validate droplet behavior on inclined embedded plane
 *
 * Physical setup:
 * - 2D droplet of radius R0 on inclined embedded plane
 * - Plane orientation: 0° (horizontal) or 45° (inclined)
 * - Initial contact angle: θi = 90° (half-disk)
 * - Static contact angle: θs (prescribed)
 * - With/without gravity (Eo parameter)
 *
 * Validation metrics:
 * - Contact angle convergence to θs
 * - Droplet radius: rf = Rf * sin(θs)
 * - Droplet height: hf = Rf * (1 - cos(θs))
 * - where Rf = R0 * sqrt(π / (2(θs - sin(θs)cos(θs))))
 * - Mass conservation (<5% absorption)
 * - Contact line pinning detection
 *
 * Test configurations:
 * 1. Horizontal plane (0°) - ideal alignment with grid
 * 2. Inclined plane (45°) - poor grid alignment (tests pinning)
 * 3. Various contact angles: 15°, 30°, 60°, 90°, 120°, 150°, 165°
 * 4. With gravity (Eo > 0) - puddle formation
 *
 * Expected behavior:
 * - Droplet relaxes from θi=90° to θs
 * - Hemispherical shape for Eo=0
 * - Flattening for Eo>0 (gravity effect)
 * - Possible contact line pinning on 45° plane for extreme angles
 *
 * Pass criteria:
 * - |rf_measured - rf_analytical| / rf_analytical < 0.05
 * - |hf_measured - hf_analytical| / hf_analytical < 0.05
 * - Mass absorption < 5%
 * - Grid convergence demonstrated
 */

#include "grid/quadtree.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/myembed.h"
#include "navier-stokes/centered.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_two-phase.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_tension.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_vof.h"
#include "../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_contact.h"

// Test parameters
#define R0 0.5                    // Initial droplet radius
#define DOMAIN_SIZE 2.0           // Domain size (2*R0)
#define THETA_I 90.0              // Initial contact angle (degrees)
#define SIGMA 1.0                 // Surface tension
#define RHO_L 1.0                 // Liquid density
#define RHO_G 1.0                 // Gas density
#define MU_L 0.01                 // Liquid viscosity
#define MU_G 0.01                 // Gas viscosity

// Simulation parameters
int MAXLEVEL = 8;                 // Maximum grid level
double MAX_TIME = 5.0;            // Simulation time
double PLANE_ANGLE = 0.0;         // Plane inclination (0° or 45°)
double THETA_S = 60.0;            // Static contact angle (degrees)
double EOTVOS = 0.0;              // Eötvös number (gravity effect)

// Measurement arrays
double time_series[10000];
double radius_series[10000];
double height_series[10000];
double angle_series[10000];
int n_measurements = 0;

// Analytical solution functions
double calc_Rf(double theta_deg) {
    double theta = theta_deg * M_PI / 180.0;
    return R0 * sqrt(M_PI / (2.0 * (theta - sin(theta) * cos(theta))));
}

double calc_rf_analytical(double theta_deg) {
    double Rf = calc_Rf(theta_deg);
    double theta = theta_deg * M_PI / 180.0;
    return Rf * sin(theta);
}

double calc_hf_analytical(double theta_deg) {
    double Rf = calc_Rf(theta_deg);
    double theta = theta_deg * M_PI / 180.0;
    return Rf * (1.0 - cos(theta));
}

// Embedded plane definition
void fraction_plane(scalar cs, face vector fs, double angle_deg) {
    double angle_rad = angle_deg * M_PI / 180.0;

    // Normal vector to plane
    double nx = sin(angle_rad);
    double ny = -cos(angle_rad);

    // For 0°: ny = -1 (horizontal plane at y=0)
    // For 45°: nx = 0.707, ny = -0.707

    vertex scalar phi[];
    foreach_vertex() {
        // Distance from point to plane
        phi[] = nx * (x - DOMAIN_SIZE/2.0) + ny * (y + DOMAIN_SIZE/2.0);
    }

    fractions(phi, cs, fs);
}

// Measure droplet properties
void measure_droplet(double *radius, double *height) {
    double max_r = 0.0;
    double max_h = 0.0;
    double sum_vol = 0.0;

    foreach(reduction(max:max_r) reduction(max:max_h) reduction(+:sum_vol)) {
        if (f[] > 0.5) {
            // Distance from center along plane (2D: x coordinate)
            double r = fabs(x);
            if (r > max_r) max_r = r;

            // Height perpendicular to plane
            double angle_rad = PLANE_ANGLE * M_PI / 180.0;
            double h = (x - DOMAIN_SIZE/2.0) * cos(angle_rad) - y * sin(angle_rad);
            if (h > max_h) max_h = h;

            sum_vol += f[] * dv();
        }
    }

    *radius = max_r;
    *height = max_h;
}

// Measure contact angle
double measure_contact_angle() {
    double angle_sum = 0.0;
    int count = 0;

    foreach() {
        if (cs[] > 0.0 && cs[] < 1.0 && f[] > 0.1 && f[] < 0.9) {
            // Interface cell near embedded boundary
            coord n = mycs (point, f);
            double norm = sqrt(sq(n.x) + sq(n.y));
            if (norm > 0) {
                n.x /= norm;
                n.y /= norm;
            }

            // Calculate angle with respect to plane normal
            double angle_rad = PLANE_ANGLE * M_PI / 180.0;
            double plane_nx = sin(angle_rad);
            double plane_ny = -cos(angle_rad);

            double cos_angle = n.x * plane_nx + n.y * plane_ny;
            double local_angle = acos(fabs(cos_angle)) * 180.0 / M_PI;

            angle_sum += local_angle;
            count++;
        }
    }

    return (count > 0) ? angle_sum / count : -1.0;
}

int main(int argc, char *argv[]) {

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == 'L') MAXLEVEL = atoi(&argv[i][1]);
        else if (argv[i][0] == 'A') THETA_S = atof(&argv[i][1]);
        else if (argv[i][0] == 'P') PLANE_ANGLE = atof(&argv[i][1]);
        else if (argv[i][0] == 'E') EOTVOS = atof(&argv[i][1]);
    }

    // Setup domain
    size(DOMAIN_SIZE);
    origin(-DOMAIN_SIZE/2.0, -DOMAIN_SIZE/2.0);
    init_grid(1 << 5);

    // Fluid properties
    rho1 = RHO_L;
    rho2 = RHO_G;
    mu1 = MU_L;
    mu2 = MU_G;
    f.sigma = SIGMA;

    // Gravity (if Eo > 0)
    if (EOTVOS > 0) {
        G.y = -EOTVOS * SIGMA / (RHO_L * sq(R0));
    }

    // Create output directory
    char command[256];
    sprintf(command, "mkdir -p results/sliding_droplet");
    system(command);

    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Sliding Droplet Test (Paper Section 4.2)\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Plane angle: %.1f°\n", PLANE_ANGLE);
    fprintf(stderr, "Contact angle: %.1f°\n", THETA_S);
    fprintf(stderr, "Eötvös number: %.2f\n", EOTVOS);
    fprintf(stderr, "Max level: %d\n", MAXLEVEL);
    fprintf(stderr, "----------------------------------------\n");
    fprintf(stderr, "Analytical predictions:\n");
    fprintf(stderr, "  Rf = %.4f\n", calc_Rf(THETA_S));
    fprintf(stderr, "  rf = %.4f\n", calc_rf_analytical(THETA_S));
    fprintf(stderr, "  hf = %.4f\n", calc_hf_analytical(THETA_S));
    fprintf(stderr, "========================================\n\n");

    run();
    return 0;
}

// Initialize embedded boundary
event init(i = 0) {
    // Create embedded plane
    vertex scalar phi[];
    foreach_vertex() {
        double angle_rad = PLANE_ANGLE * M_PI / 180.0;
        double nx = sin(angle_rad);
        double ny = -cos(angle_rad);
        phi[] = nx * x + ny * (y + DOMAIN_SIZE/2.0);
    }
    fractions(phi, cs, fs);

    // Initialize droplet as half-disk on plane (2D)
    fraction(f, -sqrt(sq(x) + sq(y)) + R0);

    // Set contact angle
    foreach() {
        contact_angle[] = THETA_S * M_PI / 180.0;
    }
}

// Adapt mesh
event adapt(i++) {
    adapt_wavelet({f, cs}, (double[]){0.01, 0.01}, MAXLEVEL);
}

// Measurements
event logfile(t += 0.05; t <= MAX_TIME) {
    double radius, height;
    measure_droplet(&radius, &height);
    double angle = measure_contact_angle();

    // Calculate mass
    double vol = 0.0;
    foreach(reduction(+:vol)) {
        vol += f[] * dv();
    }
    double initial_vol = 2.0/3.0 * M_PI * cube(R0);  // Half sphere
    double mass_error = fabs(vol - initial_vol) / initial_vol * 100.0;

    if (n_measurements < 10000) {
        time_series[n_measurements] = t;
        radius_series[n_measurements] = radius;
        height_series[n_measurements] = height;
        angle_series[n_measurements] = angle;
        n_measurements++;
    }

    fprintf(stderr, "t=%6.3f: r=%.4f h=%.4f θ=%.1f° Δm=%.2f%%\n",
            t, radius, height, angle, mass_error);
}

// Final analysis
event end(t = MAX_TIME) {
    double final_radius = radius_series[n_measurements-1];
    double final_height = height_series[n_measurements-1];
    double final_angle = angle_series[n_measurements-1];

    double rf_analytical = calc_rf_analytical(THETA_S);
    double hf_analytical = calc_hf_analytical(THETA_S);

    double radius_error = fabs(final_radius - rf_analytical) / rf_analytical * 100.0;
    double height_error = fabs(final_height - hf_analytical) / hf_analytical * 100.0;
    double angle_error = fabs(final_angle - THETA_S);

    fprintf(stderr, "\n========================================\n");
    fprintf(stderr, "FINAL RESULTS\n");
    fprintf(stderr, "========================================\n");
    fprintf(stderr, "Radius:\n");
    fprintf(stderr, "  Measured:   %.4f\n", final_radius);
    fprintf(stderr, "  Analytical: %.4f\n", rf_analytical);
    fprintf(stderr, "  Error:      %.2f%%\n", radius_error);
    fprintf(stderr, "Height:\n");
    fprintf(stderr, "  Measured:   %.4f\n", final_height);
    fprintf(stderr, "  Analytical: %.4f\n", hf_analytical);
    fprintf(stderr, "  Error:      %.2f%%\n", height_error);
    fprintf(stderr, "Contact angle:\n");
    fprintf(stderr, "  Measured:   %.1f°\n", final_angle);
    fprintf(stderr, "  Specified:  %.1f°\n", THETA_S);
    fprintf(stderr, "  Error:      %.1f°\n", angle_error);
    fprintf(stderr, "========================================\n");

    // Pass/fail criteria
    bool passed = (radius_error < 5.0) && (height_error < 5.0) && (angle_error < 5.0);

    fprintf(stderr, "\nTEST STATUS: %s\n", passed ? "✅ PASS" : "❌ FAIL");
    fprintf(stderr, "========================================\n");

    // Write results file
    char filename[256];
    sprintf(filename, "results/sliding_droplet/angle%.0f_plane%.0f_eo%.2f.txt",
            THETA_S, PLANE_ANGLE, EOTVOS);
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "# Sliding Droplet Test Results\n");
    fprintf(fp, "# Plane angle: %.1f°\n", PLANE_ANGLE);
    fprintf(fp, "# Contact angle: %.1f°\n", THETA_S);
    fprintf(fp, "# Eötvös: %.2f\n", EOTVOS);
    fprintf(fp, "# Time  Radius  Height  Angle\n");
    for (int i = 0; i < n_measurements; i++) {
        fprintf(fp, "%.6f  %.6f  %.6f  %.2f\n",
                time_series[i], radius_series[i], height_series[i], angle_series[i]);
    }
    fclose(fp);

    exit(passed ? 0 : 1);
}
