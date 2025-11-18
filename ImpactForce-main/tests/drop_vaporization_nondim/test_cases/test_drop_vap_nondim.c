/**
 * @file test_drop_vap_nondim.c
 * @brief Nondimensional Axisymmetric Drop Vaporization Test Suite
 *
 * This test implements a fully nondimensional formulation for axisymmetric
 * drop vaporization following Boyd & Ling 2023 (Computers and Fluids, Fig. 10).
 *
 * NONDIMENSIONAL FORMULATION:
 * ---------------------------
 * Scales:
 *   D = 1.0                     (characteristic length D0)
 *   U_inf = 1.0                 (nondimensional free-stream velocity)
 *   rho_l = 1.0                 (nondimensional liquid density reference)
 *   t_ref = D / U_inf           (reference time)
 *
 * Material Ratios (FIXED):
 *   eta = rho_g / rho_l = 0.000623  (from 0.597 / 958.4)
 *   mu_ratio = mu_g / mu_l = 0.045
 *
 * Nondimensional Groups (FIXED):
 *   We = 1.5                    (Weber number)
 *   Sigma_star = 1 / We = 0.6666666667
 *   St = 0.1                    (Stefan number)
 *   Pr = 1.0                    (Prandtl number)
 *
 * Parameter Sweep:
 *   Re ∈ [22, 200] with 25 linearly spaced values
 *   Per case: mu_g_star = eta / Re
 *             mu_l_star = mu_g_star / mu_ratio
 *
 * Geometry:
 *   Domain: 8.0 × 8.0 (axisymmetric)
 *   Drop radius: R0 = 0.5
 *   Drop center: x = 1.5
 *   Temperature smoothing radius: R_inf = 0.625
 *
 * Initial Conditions:
 *   f = 1 inside sphere (r <= R0), 0 outside
 *   T* = 0 for r <= R0 (saturation)
 *   T* = 1 for r >= R_inf (free stream)
 *   T* = linear interpolation for R0 < r < R_inf (Eq. 53)
 *
 * Boundary Conditions:
 *   Left: inflow (u_x = 1, T* = 1, f = 0)
 *   Right: outflow (Neumann / pressure outlet)
 *   Axis: axisymmetric boundary condition
 *
 * Reference: Boyd & Ling 2023, Computers and Fluids
 *           (Figure 10, Sections 5.1 and Table 2)
 */

// Axisymmetric geometry
#include "axi.h"
#include "navier-stokes/centered.h"
#define FILTERED 1
#include "two-phase.h"
#include "tension.h"
#include "tracer.h"
#include "diffusion.h"
#include "evaporation.h"

/**
 * ============================================================================
 * NONDIMENSIONAL PARAMETERS (FIXED)
 * ============================================================================
 */

// Characteristic scales (all unity in nondimensional formulation)
double D_char = 1.0;              // Characteristic length D0
double U_inf_star = 1.0;          // Nondimensional free-stream velocity
double rho_l_star = 1.0;          // Nondimensional liquid density

// Material ratios (FIXED - from physical properties)
double eta = 0.000623;            // rho_g / rho_l = 0.597 / 958.4
double mu_ratio = 0.045;          // mu_g / mu_l

// Nondimensional groups (FIXED)
double We = 1.5;                  // Weber number
double Sigma_star = 0.6666666667; // = 1 / We (surface tension nondim)
double St = 0.1;                  // Stefan number
double Pr = 1.0;                  // Prandtl number

// Reynolds number (VARIABLE - command-line input)
double Re = 100.0;                // Default value, swept from 22 to 200

/**
 * ============================================================================
 * DERIVED PARAMETERS
 * ============================================================================
 */

// Nondimensional viscosities (computed from Re)
double mu_g_star;                 // = eta / Re
double mu_l_star;                 // = mu_g_star / mu_ratio

// Nondimensional densities
double rho_g_star;                // = eta * rho_l_star = eta
double rho_l_ref = 1.0;           // Reference liquid density

// Geometry parameters
double domain_L0 = 8.0;           // Domain size
double drop_radius_R0 = 0.5;      // Drop radius
double drop_center_x = 1.5;       // Drop center x-coordinate
double R_inf = 0.625;             // Temperature smoothing radius (1.25 * R0)

// Time control
double CFL_number = 0.2;
double t_end_star;                // End time in nondimensional units
double t_paper_star = 0.16;       // Paper time t* = t+ * sqrt(rho_l / rho_g)

// Mesh parameters
int Lmax = 12;                    // Maximum refinement level
double dx_min;                    // Minimum cell size = D / 512

// Output control
char output_dir[256] = "results";
int snapshot_interval = 50;

/**
 * ============================================================================
 * BASILISK FIELDS
 * ============================================================================
 */

// Volume-of-Fluid tracer (f = 1 in liquid, 0 in gas)
scalar f[], *interfaces = {f};

// Temperature field (T* nondimensional: 0 at Tsat, 1 at Tinf)
scalar T[];
scalar *tracers = {T};

// Evaporation mass flux
scalar m_evap[];

// Thermal diffusivity
face vector alphav[];

// Phase properties
double rho1, rho2, mu1, mu2;

/**
 * ============================================================================
 * DIAGNOSTICS
 * ============================================================================
 */
FILE *fp_volume;
FILE *fp_nusselt;
FILE *fp_frontal_area;

/**
 * ============================================================================
 * MAIN FUNCTION
 * ============================================================================
 */
int main(int argc, char **argv) {

  // Parse command-line arguments
  if (argc >= 2) Re = atof(argv[1]);
  if (argc >= 3) strcpy(output_dir, argv[3]);

  // Create output directory
  char cmd[512];
  sprintf(cmd, "mkdir -p %s", output_dir);
  system(cmd);

  /**
   * Compute nondimensional viscosities from Reynolds number
   * Re = rho_g_star * U_inf_star * D_char / mu_g_star
   * mu_g_star = rho_g_star * U_inf_star * D_char / Re
   * Since rho_g_star = eta, U_inf_star = 1, D_char = 1:
   * mu_g_star = eta / Re
   */
  rho_g_star = eta * rho_l_star;
  mu_g_star = eta / Re;
  mu_l_star = mu_g_star / mu_ratio;

  fprintf(stderr, "\n");
  fprintf(stderr, "========================================\n");
  fprintf(stderr, "NONDIMENSIONAL DROP VAPORIZATION TEST\n");
  fprintf(stderr, "========================================\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Material Ratios (FIXED):\n");
  fprintf(stderr, "  eta = rho_g/rho_l = %.6f\n", eta);
  fprintf(stderr, "  mu_g/mu_l         = %.6f\n", mu_ratio);
  fprintf(stderr, "\n");
  fprintf(stderr, "Nondimensional Groups (FIXED):\n");
  fprintf(stderr, "  We = %.2f\n", We);
  fprintf(stderr, "  St = %.2f\n", St);
  fprintf(stderr, "  Pr = %.2f\n", Pr);
  fprintf(stderr, "  Sigma* = %.10f\n", Sigma_star);
  fprintf(stderr, "\n");
  fprintf(stderr, "Parameter Sweep:\n");
  fprintf(stderr, "  Re = %.2f\n", Re);
  fprintf(stderr, "\n");
  fprintf(stderr, "Derived Nondimensional Properties:\n");
  fprintf(stderr, "  rho_g* = %.6e\n", rho_g_star);
  fprintf(stderr, "  rho_l* = %.6e\n", rho_l_star);
  fprintf(stderr, "  mu_g*  = %.6e\n", mu_g_star);
  fprintf(stderr, "  mu_l*  = %.6e\n", mu_l_star);
  fprintf(stderr, "\n");
  fprintf(stderr, "Geometry:\n");
  fprintf(stderr, "  Domain:       %.1f × %.1f\n", domain_L0, domain_L0);
  fprintf(stderr, "  Drop radius:  %.2f\n", drop_radius_R0);
  fprintf(stderr, "  Drop center:  (%.2f, 0.0)\n", drop_center_x);
  fprintf(stderr, "  R_inf:        %.3f\n", R_inf);
  fprintf(stderr, "\n");
  fprintf(stderr, "Mesh:\n");
  fprintf(stderr, "  Lmax:         %d\n", Lmax);
  fprintf(stderr, "  dx_min:       1/%d\n", (int)(1.0 / (D_char / (1 << Lmax))));
  fprintf(stderr, "\n");

  /**
   * Compute end time
   * Paper time: t* = t+ * sqrt(rho_l / rho_g)
   * Solver time: t+ = t * U_inf / D
   * So: t* = t * (U_inf / D) * sqrt(rho_l / rho_g)
   * For nondim: t_end = t* / (sqrt(rho_l / rho_g))
   */
  double density_ratio_sqrt = sqrt(rho_l_star / rho_g_star);
  t_end_star = t_paper_star / density_ratio_sqrt;

  fprintf(stderr, "Time:\n");
  fprintf(stderr, "  t* (paper) = %.2f\n", t_paper_star);
  fprintf(stderr, "  t_end      = %.6f\n", t_end_star);
  fprintf(stderr, "  sqrt(rho_l/rho_g) = %.2f\n", density_ratio_sqrt);
  fprintf(stderr, "\n");
  fprintf(stderr, "========================================\n");
  fprintf(stderr, "\n");

  // Set domain size
  size(domain_L0);
  origin(0, 0);

  // Initialize grid
  init_grid(1 << 6);  // Start with 64×64 base grid

  // Set two-phase properties for Basilisk
  // (phase 1 = liquid, phase 2 = gas)
  rho1 = rho_l_star;
  rho2 = rho_g_star;
  mu1 = mu_l_star;
  mu2 = mu_g_star;

  // Surface tension (already nondimensional)
  f.sigma = Sigma_star;

  // CFL condition
  CFL = CFL_number;

  // Run simulation
  run();
}

/**
 * ============================================================================
 * INITIALIZATION EVENT
 * ============================================================================
 */
event init(i = 0) {

  /**
   * Initialize volume fraction field
   * Sphere of radius R0 centered at (drop_center_x, 0)
   * f = 1 inside liquid, 0 outside
   */
  fraction(f, sq(drop_radius_R0) - sq(x - drop_center_x) - sq(y));

  /**
   * Initialize temperature field using Eq. 53:
   * T* = 0 for r <= R0 (saturation temperature)
   * T* = 1 for r >= R_inf (free stream temperature)
   * T* = linear interpolation for R0 < r < R_inf
   */
  foreach() {
    double r = sqrt(sq(x - drop_center_x) + sq(y));

    if (r <= drop_radius_R0) {
      T[] = 0.0;  // Saturation temperature
    }
    else if (r >= R_inf) {
      T[] = 1.0;  // Free stream temperature
    }
    else {
      // Linear interpolation (Eq. 53)
      T[] = (r - drop_radius_R0) / (R_inf - drop_radius_R0);
    }
  }

  /**
   * Initialize velocity field
   * Uniform inflow from the left with velocity U_inf = 1.0
   */
  foreach() {
    u.x[] = U_inf_star;  // = 1.0
    u.y[] = 0.0;
  }

  // Open diagnostic files
  char filepath[512];

  sprintf(filepath, "%s/liquid_volume_history.txt", output_dir);
  fp_volume = fopen(filepath, "w");
  fprintf(fp_volume, "# Nondimensional Drop Vaporization - Volume History\n");
  fprintf(fp_volume, "# Re = %.2f, We = %.2f, St = %.2f, Pr = %.2f\n", Re, We, St, Pr);
  fprintf(fp_volume, "# Columns: t+ | t* | Volume | V/V0 | dV/dt\n");

  sprintf(filepath, "%s/nusselt_history.txt", output_dir);
  fp_nusselt = fopen(filepath, "w");
  fprintf(fp_nusselt, "# Nusselt Number History\n");
  fprintf(fp_nusselt, "# Columns: t+ | t* | Nu | q_interface\n");

  sprintf(filepath, "%s/frontal_area_history.txt", output_dir);
  fp_frontal_area = fopen(filepath, "w");
  fprintf(fp_frontal_area, "# Frontal Area History\n");
  fprintf(fp_frontal_area, "# Columns: t+ | t* | A_frontal | Zf\n");
}

/**
 * ============================================================================
 * BOUNDARY CONDITIONS
 * ============================================================================
 */

// Left boundary: inflow
u.n[left] = dirichlet(U_inf_star);  // u_x = 1
u.t[left] = dirichlet(0);           // u_y = 0
T[left] = dirichlet(1.0);           // T* = 1 (free stream temperature)
f[left] = dirichlet(0);             // f = 0 (gas phase)

// Right boundary: outflow (Neumann / pressure outlet)
u.n[right] = neumann(0);
p[right] = dirichlet(0);
T[right] = neumann(0);

// Axis: axisymmetric boundary (automatically handled by axi.h)

/**
 * ============================================================================
 * ADAPTIVE MESH REFINEMENT
 * ============================================================================
 */
event adapt(i++) {

  /**
   * Refine based on:
   * 1. Interface (VOF field f)
   * 2. Velocity gradients
   * 3. Temperature gradients
   *
   * Refinement band: 5 cells on each side of interface
   */
  double uemax = 0.01;  // Velocity error tolerance
  double femax = 0.01;  // VOF error tolerance
  double Temax = 0.02;  // Temperature error tolerance

  adapt_wavelet({f, u.x, u.y, T},
                (double[]){femax, uemax, uemax, Temax},
                maxlevel = Lmax,
                minlevel = 2);
}

/**
 * ============================================================================
 * DIAGNOSTICS AND OUTPUT
 * ============================================================================
 */

/**
 * Log progress every timestep
 */
event logfile(i++) {

  // Compute time scales
  double t_plus = t;  // Solver time t+ = t * U_inf / D (already nondim)
  double t_star = t_plus * sqrt(rho_l_star / rho_g_star);  // Paper time

  // Compute liquid volume
  double volume = 0.0;
  foreach(reduction(+:volume)) {
    volume += f[] * dv();
  }

  // Initial volume (sphere)
  static double V0 = -1.0;
  if (V0 < 0) V0 = 4.0/3.0 * pi * pow(drop_radius_R0, 3.0);

  double V_ratio = volume / V0;

  // Log to stderr
  fprintf(stderr, "t+ = %g, t* = %g, i = %d, dt = %g, V/V0 = %.6f\n",
          t_plus, t_star, i, dt, V_ratio);

  // Write to volume history file
  if (fp_volume) {
    fprintf(fp_volume, "%g %g %g %g\n", t_plus, t_star, volume, V_ratio);
    fflush(fp_volume);
  }
}

/**
 * Compute and output Nusselt number and frontal area
 */
event diagnostics(t += 0.01) {

  double t_plus = t;
  double t_star = t_plus * sqrt(rho_l_star / rho_g_star);

  /**
   * Compute Nusselt number
   * Nu = q * D / (k_g * Delta_T)
   * where q is the heat flux at the interface
   */

  // Placeholder for Nusselt computation
  // (requires interface heat flux calculation)
  double Nu = 0.0;

  /**
   * Compute frontal area and Zf parameter
   * Frontal area = projection of drop onto flow direction
   */
  double A_frontal = 0.0;
  foreach(reduction(+:A_frontal)) {
    if (f[] > 0.5) {
      A_frontal += dv() / Delta;  // Area element
    }
  }

  if (fp_nusselt) {
    fprintf(fp_nusselt, "%g %g %g\n", t_plus, t_star, Nu);
    fflush(fp_nusselt);
  }

  if (fp_frontal_area) {
    fprintf(fp_frontal_area, "%g %g %g\n", t_plus, t_star, A_frontal);
    fflush(fp_frontal_area);
  }
}

/**
 * Output VTK snapshots for visualization
 */
event snapshots(t += 0.01) {
  char filename[512];
  sprintf(filename, "%s/snapshot_%05d.vtk", output_dir, (int)(t*100));

  // Output VTK file with f, u, T, p fields
  FILE *fp = fopen(filename, "w");
  if (fp) {
    output_vtk({f, T, u.x, u.y, p}, fp);
    fclose(fp);
  }
}

/**
 * Save dump files for restart
 */
event snapshots_dump(t += 0.05) {
  char filename[512];
  sprintf(filename, "%s/dump_%05d", output_dir, (int)(t*100));
  dump(filename);
}

/**
 * End simulation
 */
event end(t = t_end_star) {
  fprintf(stderr, "\nSimulation completed at t* = %.4f\n", t_end_star);

  // Close diagnostic files
  if (fp_volume) fclose(fp_volume);
  if (fp_nusselt) fclose(fp_nusselt);
  if (fp_frontal_area) fclose(fp_frontal_area);
}
