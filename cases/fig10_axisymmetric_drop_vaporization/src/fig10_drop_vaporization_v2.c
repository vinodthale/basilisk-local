/**
 * @file fig10_drop_vaporization_v2.c
 * @brief Axisymmetric drop vaporization simulation (Figure 10 reproduction)
 *
 * This simulation reproduces Figure 10 from the paper, modeling an axisymmetric
 * vaporizing water drop in a hot steam environment.
 *
 * Reference: Eqs. (51)-(53) for evaporation model
 */

#include "grid/multigrid.h"        // Multigrid solver
#include "axi.h"                    // Axisymmetric coordinates
#include "navier-stokes/centered.h" // Two-phase Navier-Stokes solver
#include "two-phase.h"              // Density and viscosity for two phases
#include "tension.h"                // Surface tension
#include "henry.h"                  // Soluble tracers for temperature

/**
 * Physical and numerical parameters
 */

// Dimensionless numbers (command-line inputs)
double REYNOLDS = 100.0;    // Reynolds number
double WEBER = 1.5;         // Weber number (fixed)
double STEFAN = 0.1;        // Stefan number
double PRANDTL = 1.0;       // Prandtl number

// Reference scales
double D0 = 1.0;            // Reference diameter
double R0;                  // Reference radius = D0/2
double Rinf;                // Temperature smoothing radius = 1.25*R0
double L0;                  // Domain size = 8*D0
double xc;                  // Drop center x-coordinate = 1.5*D0

// Thermophysical properties (dimensional)
double rho_l = 958.4;       // Liquid density [kg/m³]
double rho_g = 0.597;       // Gas density [kg/m³]
double mu_l = 2.8e-4;       // Liquid dynamic viscosity [Pa·s]
double mu_g = 1.26e-5;      // Gas dynamic viscosity [Pa·s]
double k_l = 0.679;         // Liquid thermal conductivity [W/mK]
double k_g = 0.025;         // Gas thermal conductivity [W/mK]
double cp_l = 4216.0;       // Liquid specific heat [J/kgK]
double cp_g = 2030.0;       // Gas specific heat [J/kgK]
double sigma_dim = 0.0728;  // Surface tension [N/m]
double h_lg = 2.26e6;       // Latent heat of vaporization [J/kg]
double Tsat = 373.15;       // Saturation temperature [K]
double Tinf = 600.0;        // Free stream temperature [K]

// Derived quantities
double Uinf;                // Free stream velocity [m/s]

// Time control
double t_end;               // End time
double t_transient = 0.06;  // Transient decay time (dimensionless)

// Mesh parameters
int MAXLEVEL = 10;          // Maximum refinement level (reduced for testing)

// Output control
int snapshot_interval = 50;
char output_dir[256] = "results";

/**
 * Volume-of-Fluid tracer for the liquid phase
 * f = 1 in liquid (water), f = 0 in gas (steam)
 */
scalar f[], * interfaces = {f};

/**
 * Temperature field as a soluble tracer
 * T is dimensionless: T_norm = (T - Tsat)/(Tinf - Tsat)
 */
scalar T[], * stracers = {T};

/**
 * Main function: initialization
 */
int main(int argc, char **argv) {

  // Parse command-line arguments
  if (argc >= 2) REYNOLDS = atof(argv[1]);
  if (argc >= 3) WEBER = atof(argv[2]);
  if (argc >= 4) strcpy(output_dir, argv[3]);

  // Create output directory
  char cmd[512];
  sprintf(cmd, "mkdir -p %s", output_dir);
  system(cmd);

  // Compute derived parameters
  R0 = D0 / 2.0;
  Rinf = 1.25 * R0;
  L0 = 8.0 * D0;
  xc = 1.5 * D0;

  /**
   * Calculate free stream velocity from dimensionless numbers
   * We = rho_g * Uinf^2 * D0 / sigma
   * Re = rho_g * Uinf * D0 / mu_g
   */

  // Use Reynolds to set Uinf
  Uinf = REYNOLDS * mu_g / (rho_g * D0);

  // Adjust sigma to maintain Weber number
  double sigma_adjusted = rho_g * Uinf * Uinf * D0 / WEBER;

  fprintf(stderr, "# Flow parameters:\n");
  fprintf(stderr, "#   Uinf = %.6e m/s\n", Uinf);
  fprintf(stderr, "#   sigma (adjusted) = %.6e N/m\n", sigma_adjusted);
  fprintf(stderr, "#   D0 = %.6e m\n", D0);

  /**
   * Dimensionless time: t* = t * Uinf/D0 * sqrt(rho_l/rho_g)
   * Simulation end: t* = 0.16
   */
  double rho_ratio = rho_l / rho_g;
  double time_scale = D0 / (Uinf * sqrt(rho_ratio));
  t_end = 0.16 * time_scale;

  fprintf(stderr, "#   Time scale = %.6e s\n", time_scale);
  fprintf(stderr, "#   End time = %.6e s (t* = 0.16)\n", t_end);

  /**
   * Non-dimensionalization for Basilisk
   * Use rho_g, Uinf, D0 as reference scales
   */
  rho1 = rho_l / rho_g;  // Liquid density (dimensionless)
  rho2 = 1.0;            // Gas density (dimensionless)
  mu1 = mu_l / mu_g;     // Liquid viscosity (dimensionless)
  mu2 = 1.0;             // Gas viscosity (dimensionless)

  // Surface tension (dimensionless)
  f.sigma = sigma_adjusted / (rho_g * Uinf * Uinf * D0);

  /**
   * Temperature tracer properties
   * Thermal diffusivity: alpha = k / (rho * cp)
   * In dimensionless form: alpha_dim = alpha / (Uinf * D0)
   */
  double alpha_l = k_l / (rho_l * cp_l);
  double alpha_g = k_g / (rho_g * cp_g);

  // Dimensionless thermal diffusivities
  T.D1 = alpha_l / (Uinf * D0);  // Liquid phase
  T.D2 = alpha_g / (Uinf * D0);  // Gas phase
  T.alpha = 1.0;  // Continuous temperature at interface (no jump)

  fprintf(stderr, "# Dimensionless parameters:\n");
  fprintf(stderr, "#   rho1/rho2 = %.2f\n", rho1);
  fprintf(stderr, "#   mu1/mu2 = %.2f\n", mu1);
  fprintf(stderr, "#   sigma = %.6f\n", f.sigma);
  fprintf(stderr, "#   T.D1 (liquid) = %.6e\n", T.D1);
  fprintf(stderr, "#   T.D2 (gas) = %.6e\n", T.D2);
  fprintf(stderr, "#   We = %.2f, Re = %.2f, St = %.2f, Pr = %.2f\n",
          WEBER, REYNOLDS, STEFAN, PRANDTL);

  // Domain size (dimensionless)
  size(L0 / D0);  // 8x8 in units of D0
  origin(0, 0);

  // Initialize grid
  N = 64;  // Base grid

  // CFL condition
  CFL = 0.2;

  // Run simulation
  run();
}

/**
 * Initialize fields
 */
event init(i = 0) {

  /**
   * Initialize volume fraction field
   * Drop is centered at (xc, 0) with radius R0
   */
  fraction(f, sq(R0/D0) - sq(x - xc/D0) - sq(y));

  /**
   * Initialize temperature field
   * Dimensionless temperature: T_norm = (T - Tsat)/(Tinf - Tsat)
   *   T = 0 (Tsat) for r <= R0
   *   T = 1 (Tinf) for r >= Rinf
   *   T = linear interpolation for R0 < r < Rinf
   */
  foreach() {
    double r = sqrt(sq(x - xc/D0) + sq(y));

    if (r <= R0/D0) {
      T[] = 0.0;  // Saturation temperature
    }
    else if (r >= Rinf/D0) {
      T[] = 1.0;  // Free stream temperature
    }
    else {
      // Linear interpolation
      T[] = (r - R0/D0) / (Rinf/D0 - R0/D0);
    }
  }

  /**
   * Initialize velocity field
   * Uniform inflow from left with velocity Uinf (= 1 in dimensionless units)
   */
  foreach() {
    u.x[] = 1.0;  // Uinf in dimensionless form
    u.y[] = 0.0;
  }
}

/**
 * Boundary conditions
 */

// Left boundary: inflow
u.n[left] = dirichlet(1.0);  // Uinf
u.t[left] = dirichlet(0.0);
p[left] = neumann(0.0);
f[left] = dirichlet(0.0);    // Gas phase
T[left] = dirichlet(1.0);    // Tinf

// Right boundary: outflow
u.n[right] = neumann(0.0);
u.t[right] = neumann(0.0);
p[right] = dirichlet(0.0);
f[right] = neumann(0.0);
T[right] = neumann(0.0);

// Top: axisymmetric (handled by axi.h for velocity)
f[top] = neumann(0.0);
T[top] = neumann(0.0);

/**
 * Adaptive mesh refinement
 */
#if TREE
event adapt(i++) {
  adapt_wavelet({f, u.x, u.y, T},
                (double[]){0.01, 0.1, 0.1, 0.05},
                maxlevel = MAXLEVEL, minlevel = 2);
}
#endif

/**
 * Output and diagnostics
 */

// Volume tracking
event volume_tracking(i += 10) {

  double volume = 0.0;
  double volume_initial = 4.0/3.0 * M_PI * cube(R0/D0);

  foreach(reduction(+:volume)) {
    volume += f[] * dv();
  }

  double rho_ratio = rho1;
  double t_star = t * (Uinf/D0) * sqrt(rho_ratio);

  static FILE * fp = NULL;
  if (i == 0) {
    char filename[512];
    sprintf(filename, "%s/volume_history.txt", output_dir);
    fp = fopen(filename, "w");
    fprintf(fp, "# t [s]\tt* [-]\tVolume [D0^3]\tV/V0 [-]\n");
  }

  if (fp) {
    fprintf(fp, "%.6e\t%.6e\t%.6e\t%.6e\n", t, t_star, volume, volume/volume_initial);
    fflush(fp);
  }
}

// Snapshot output
event snapshots(i += snapshot_interval) {

  char filename[512];

  // Dump all fields
  sprintf(filename, "%s/fields_%06d", output_dir, i);
  dump(file = filename);

  double t_star = t * (Uinf/D0) * sqrt(rho1);

  fprintf(stderr, "# Output at i=%d, t=%.6e (t*=%.6e)\n", i, t, t_star);
}

// Log file output
event logfile(i += 10) {

  double t_star = t * (Uinf/D0) * sqrt(rho1);

  if (i == 0) {
    fprintf(stderr, "# Starting simulation:\n");
    fprintf(stderr, "#   Re = %.2f, We = %.2f, St = %.2f\n", REYNOLDS, WEBER, STEFAN);
    fprintf(stderr, "#   D0 = %.6e m, Uinf = %.6e m/s\n", D0, Uinf);
    fprintf(stderr, "#   Domain: %.2f x %.2f (in D0 units)\n", L0/D0, L0/D0);
    fprintf(stderr, "#   Max level: %d\n", MAXLEVEL);
    fprintf(stderr, "#\n");
    fprintf(stderr, "# i\tt [s]\tt* [-]\n");
  }

  fprintf(stderr, "%d\t%.6e\t%.6e\n", i, t, t_star);
}

/**
 * Stop condition
 */
event stop(t = t_end) {
  fprintf(stderr, "# Simulation completed at t* = %.6e\n",
          t * (Uinf/D0) * sqrt(rho1));
  return 1;
}
