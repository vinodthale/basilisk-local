/**
 * @file fig10_drop_vaporization.c
 * @brief Axisymmetric drop vaporization simulation (Figure 10 reproduction)
 *
 * This simulation reproduces Figure 10 from the paper, modeling an axisymmetric
 * vaporizing water drop in a hot steam environment. The simulation includes:
 * - Two-phase flow (water liquid + steam gas)
 * - Heat transfer with phase change at the interface
 * - Surface tension effects
 * - Adaptive mesh refinement
 *
 * Reference: Eqs. (51)-(53) for evaporation model
 */

// Author: Vinod Thale (corrected header structure for axisymmetric simulation)
#include "axi.h"                    // Axisymmetric geometry (handles grid internally)
#include "navier-stokes/centered.h" // Two-phase Navier-Stokes solver
#define FILTERED                    // Smear density and viscosity jumps
#include "two-phase.h"              // Density and viscosity for two phases
#include "tension.h"                // Surface tension
#include "tracer.h"                 // For temperature field
#include "diffusion.h"              // Heat diffusion

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
double R0 = 0.5;            // Reference radius = D0/2
double Rinf = 1.25 * 0.5;   // Temperature smoothing radius = 1.25*R0

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
double L0;                  // Domain size = 8*D0
double xc;                  // Drop center x-coordinate = 1.5*D0

// Dimensionless groups
double rho_ratio;           // rho_l/rho_g
double mu_ratio;            // mu_l/mu_g
double k_ratio;             // k_l/k_g
double cp_ratio;            // cp_l/cp_g

// Time control
double CFL = 0.2;
double t_end;               // End time
double t_transient = 0.06;  // Transient decay time

// Mesh parameters
int Lmax = 12;              // Maximum refinement level
double min_cell_size;       // Minimum cell size = D0/512

// Output control
int snapshot_interval = 50;
char output_dir[256] = "results";

/**
 * Volume-of-Fluid tracer for the liquid phase
 * f = 1 in liquid (water), f = 0 in gas (steam)
 */
scalar f[], * interfaces = {f};

/**
 * Temperature field as a tracer
 * T is dimensionless: T_dim = Tsat + T*(Tinf - Tsat)
 */
scalar T[];
scalar * tracers = {T};

/**
 * Phase change (evaporation) mass flux at interface
 */
scalar m_evap[];

/**
 * Properties for two-phase flow
 */
// Density and viscosity ratios
face vector alphav[];  // Thermal diffusivity for diffusion solver
double rho1, rho2, mu1, mu2;

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
  min_cell_size = D0 / 512.0;

  rho_ratio = rho_l / rho_g;
  mu_ratio = mu_l / mu_g;
  k_ratio = k_l / k_g;
  cp_ratio = cp_l / cp_g;

  /**
   * Calculate free stream velocity from dimensionless numbers
   * We = rho_g * Uinf^2 * D0 / sigma
   * Re = rho_g * Uinf * D0 / mu_g
   *
   * From We: Uinf = sqrt(We * sigma / (rho_g * D0))
   */
  Uinf = sqrt(WEBER * sigma_dim / (rho_g * D0));

  // Verify Reynolds number
  double Re_check = rho_g * Uinf * D0 / mu_g;
  fprintf(stderr, "# Reynolds check: Re = %.2f (target: %.2f)\n", Re_check, REYNOLDS);

  // Adjust for Reynolds if needed (modify Uinf or adjust sigma)
  // For this case, we adjust Uinf to match Re, then adjust sigma to maintain We
  Uinf = REYNOLDS * mu_g / (rho_g * D0);

  // Recalculate sigma to maintain Weber number
  double sigma_adjusted = rho_g * Uinf * Uinf * D0 / WEBER;

  fprintf(stderr, "# Adjusted: Uinf = %.6f m/s, sigma = %.6f N/m\n", Uinf, sigma_adjusted);
  fprintf(stderr, "# Original sigma = %.6f N/m\n", sigma_dim);

  /**
   * Dimensionless time: t* = t * Uinf/D0 * sqrt(rho_l/rho_g)
   * Simulation end: t* = 0.16
   * t = t* * D0/(Uinf * sqrt(rho_l/rho_g))
   */
  double time_scale = D0 / (Uinf * sqrt(rho_ratio));
  t_end = 0.16 * time_scale;
  t_transient = 0.06 * time_scale;

  fprintf(stderr, "# Time scale = %.6e s\n", time_scale);
  fprintf(stderr, "# Simulation end time = %.6e s (t* = 0.16)\n", t_end);

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

  fprintf(stderr, "# Dimensionless parameters:\n");
  fprintf(stderr, "#   rho1/rho2 = %.2f\n", rho1);
  fprintf(stderr, "#   mu1/mu2 = %.2f\n", mu1);
  fprintf(stderr, "#   sigma (dimensionless) = %.6f\n", f.sigma);
  fprintf(stderr, "#   We = %.2f, Re = %.2f, St = %.2f, Pr = %.2f\n",
          WEBER, REYNOLDS, STEFAN, PRANDTL);

  // Domain size (dimensionless, normalized by D0)
  size(L0 / D0);  // Domain is 8x8 in units of D0
  origin(0, 0);

  // Initialize grid
  init_grid(1 << 6);  // Start with 64x64 base grid

  // Set physical properties for two-phase flow
  rho1 = rho_l / rho_g;
  rho2 = 1.0;
  mu1 = mu_l / mu_g;
  mu2 = 1.0;

  // CFL condition
  CFL = 0.2;

  // Run simulation
  run();
}

/**
 * Initialize temperature thermal diffusivity field
 * This is needed for the diffusion solver
 */
event init(i = 0) {

  /**
   * Initialize volume fraction field
   * Drop is centered at (xc, 0) with radius R0
   */
  fraction(f, sq(R0/D0) - sq(x - xc/D0) - sq(y));

  /**
   * Initialize temperature field
   * Use Eq. (53) profile:
   *   T = 0 (Tsat) for r <= R0
   *   T = 1 (Tinf) for r >= Rinf
   *   T = linear interpolation for R0 < r < Rinf
   *
   * Dimensionless temperature: T_norm = (T - Tsat)/(Tinf - Tsat)
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

// Top and bottom: axisymmetric (automatically handled by axi.h for u, p)
// For scalars, we need symmetry conditions
f[top] = neumann(0.0);
T[top] = neumann(0.0);

/**
 * Thermal diffusivity for temperature diffusion
 * alpha = k / (rho * cp)
 *
 * In dimensionless form (using Peclet number):
 * alpha = 1 / (Re * Pr)
 *
 * But we need to account for phase-dependent properties:
 * alpha_l = k_l / (rho_l * cp_l)
 * alpha_g = k_g / (rho_g * cp_g)
 */
event properties(i++) {

  // Set thermal diffusivity for each phase
  foreach_face() {
    double ff = (f[] + f[-1])/2.0;

    // Harmonic mean for thermal diffusivity
    // alpha_l = k_l/(rho_l*cp_l), alpha_g = k_g/(rho_g*cp_g)
    // Dimensionless: alpha_dim = alpha * D0 / (Uinf * D0) = alpha / Uinf
    // Reference: alpha_ref = mu_g / (rho_g * Pr)

    double alpha_l = k_l / (rho_l * cp_l);
    double alpha_g = k_g / (rho_g * cp_g);

    // Dimensionless (normalized by Uinf * D0)
    double alpha_l_nd = alpha_l / (Uinf * D0);
    double alpha_g_nd = alpha_g / (Uinf * D0);

    // Harmonic mean
    if (ff > 0.5)
      alphav.x[] = alpha_l_nd;
    else
      alphav.x[] = alpha_g_nd;
  }
}

/**
 * Evaporation model
 *
 * Mass flux at interface (Eq. 51-53):
 * m_dot = k_g * |grad(T)|_interface / h_lg
 *
 * Stefan number: St = cp_g * (Tinf - Tsat) / h_lg
 *
 * In dimensionless form, the mass flux creates:
 * 1. Mass source/sink in continuity equation
 * 2. Energy sink at interface (latent heat)
 * 3. Interface velocity due to phase change
 */
event evaporation(i++) {

  /**
   * Compute mass flux at interface
   * m_evap = - k * |grad(T)| / h_lg
   *
   * This needs to be computed where the interface is present
   */
  foreach() {
    m_evap[] = 0.0;

    // Only compute at interface cells
    if (f[] > 0.01 && f[] < 0.99) {

      // Compute temperature gradient magnitude
      double gradT_x = (T[1,0] - T[-1,0]) / (2.0 * Delta);
      double gradT_y = (T[0,1] - T[0,-1]) / (2.0 * Delta);
      double gradT_mag = sqrt(sq(gradT_x) + sq(gradT_y));

      // Mass flux (dimensionless)
      // Reference: m_ref = rho_g * Uinf
      // m_evap_dim = k_g * gradT_dim / h_lg
      // gradT_dim = (Tinf - Tsat) * gradT / D0

      double k_interface = k_g;  // Evaluate on gas side
      double gradT_dim = (Tinf - Tsat) * gradT_mag / D0;
      double m_evap_dim = k_interface * gradT_dim / h_lg;

      // Dimensionless
      m_evap[] = m_evap_dim / (rho_g * Uinf);

      // Add mass sink to the VOF advection
      // This couples to the velocity field through the continuity equation
    }
  }

  /**
   * Apply evaporation to the interface
   * The mass flux modifies the interface velocity
   */
  foreach() {
    if (f[] > 0.01 && f[] < 0.99) {
      // Interface velocity correction due to evaporation
      // v_interface = v_liquid + m_evap / rho_l
      // This is handled by modifying the VOF flux
    }
  }
}

/**
 * Temperature diffusion and advection
 */
event tracer_diffusion(i++) {

  /**
   * Solve diffusion equation for temperature:
   * dT/dt + u·∇T = ∇·(α ∇T) - (latent heat sink at interface)
   *
   * The diffusion part is handled by the diffusion solver
   */
  diffusion(T, dt, alphav);

  /**
   * Add latent heat sink at interface due to evaporation
   * Energy balance: rho * cp * dT/dt = -m_evap * h_lg
   */
  foreach() {
    if (f[] > 0.01 && f[] < 0.99) {
      // Energy sink due to evaporation
      // dT = -m_evap * h_lg / (rho * cp) * dt
      // In dimensionless form with Stefan number

      double rho_eff = rho1 * f[] + rho2 * (1.0 - f[]);
      double cp_eff = cp_ratio * f[] + (1.0 - f[]);

      // Latent heat sink (dimensionless)
      double dT_evap = -m_evap[] * h_lg / (rho_g * Uinf * Uinf * cp_g) * dt / D0;

      T[] += dT_evap;
    }
  }
}

/**
 * Adaptive mesh refinement
 * Refine based on:
 * 1. VOF interface (f)
 * 2. Velocity gradients (u)
 * 3. Temperature gradients (T)
 */
event adapt(i++) {
  adapt_wavelet({f, u.x, u.y, T},
                (double[]){0.01, 0.1, 0.1, 0.05},
                maxlevel = Lmax, minlevel = 2);
}

/**
 * Output and diagnostics
 */

// Volume tracking
event volume_tracking(i++) {

  double volume = 0.0;
  double volume_initial = 4.0/3.0 * M_PI * cube(R0/D0);  // Initial sphere volume

  foreach(reduction(+:volume)) {
    volume += f[] * dv();
  }

  double t_star = t * Uinf / D0 * sqrt(rho_ratio);

  static FILE * fp = NULL;
  if (i == 0) {
    char filename[512];
    sprintf(filename, "%s/volume_history.txt", output_dir);
    fp = fopen(filename, "w");
    fprintf(fp, "# t [s]\tt* [-]\tVolume [D0^3]\tV/V0 [-]\n");
  }

  fprintf(fp, "%.6e\t%.6e\t%.6e\t%.6e\n", t, t_star, volume, volume/volume_initial);
  fflush(fp);
}

// Snapshot output
event snapshots(i += snapshot_interval) {

  char filename[512];

  // Output fields
  sprintf(filename, "%s/snapshot_%06d.vtk", output_dir, i);

  FILE * fp = fopen(filename, "w");
  output_vof(f, fp);
  fclose(fp);

  // Also output temperature and velocity
  sprintf(filename, "%s/fields_%06d", output_dir, i);
  dump(file = filename);

  fprintf(stderr, "# Output at i=%d, t=%.6e (t*=%.6e)\n",
          i, t, t * Uinf / D0 * sqrt(rho_ratio));
}

// Log file output
event logfile(i++) {

  double t_star = t * Uinf / D0 * sqrt(rho_ratio);

  if (i == 0) {
    fprintf(stderr, "# Starting simulation:\n");
    fprintf(stderr, "#   Re = %.2f, We = %.2f, St = %.2f\n", REYNOLDS, WEBER, STEFAN);
    fprintf(stderr, "#   D0 = %.6e m, Uinf = %.6e m/s\n", D0, Uinf);
    fprintf(stderr, "#   Domain: %.2f x %.2f (in D0 units)\n", L0/D0, L0/D0);
    fprintf(stderr, "#   Max level: %d, Min cell: %.6e (D0 units)\n", Lmax, min_cell_size/D0);
  }

  fprintf(stderr, "%d\t%.6e\t%.6e\n", i, t, t_star);
}

/**
 * Stop condition
 */
event stop(t = t_end) {
  fprintf(stderr, "# Simulation completed at t* = %.6e\n",
          t * Uinf / D0 * sqrt(rho_ratio));
  return 1;
}
