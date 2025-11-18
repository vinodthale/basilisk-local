/**
 * @file fig10_drop_vaporization_simple.c
 * @brief Simplified axisymmetric drop vaporization (for testing)
 */

// Author: Vinod Thale (corrected header structure for axisymmetric simulation)
#include "axi.h"                       // Axisymmetric geometry (handles grid internally)
#include "navier-stokes/centered.h"    // Two-phase Navier-Stokes solver
#define FILTERED                       // Smear density and viscosity jumps
#include "two-phase.h"                 // Density and viscosity for two phases
#include "tension.h"                   // Surface tension

// Dimensionless numbers
double REYNOLDS = 100.0;
double WEBER = 1.5;

// Reference scales
double D0 = 1.0;
double R0 = 0.5;
double L0 = 8.0;
double xc = 1.5;

// VOF tracer
scalar f[], * interfaces = {f};

// Properties
double rho1, rho2, mu1, mu2;

int main(int argc, char **argv) {

  // Parse arguments
  if (argc >= 2) REYNOLDS = atof(argv[1]);
  if (argc >= 3) WEBER = atof(argv[2]);

  // Domain size
  size(L0);
  origin(0, 0);

  // Initialize grid
  N = 64;

  // Set properties (dimensionless)
  rho1 = 958.4 / 0.597;  // Liquid/gas ratio
  rho2 = 1.0;
  mu1 = 2.8e-4 / 1.26e-5;  // Liquid/gas ratio
  mu2 = 1.0;

  // Surface tension
  f.sigma = 1.0 / WEBER;

  // CFL
  CFL = 0.2;

  run();
}

// Initialize drop
event init (i = 0) {
  fraction (f, sq(R0) - sq(x - xc) - sq(y));

  foreach() {
    u.x[] = 1.0;  // Inflow velocity
    u.y[] = 0.0;
  }
}

// Boundary conditions
u.n[left] = dirichlet(1.0);
u.t[left] = dirichlet(0.0);
p[left] = neumann(0.0);
f[left] = dirichlet(0.0);

u.n[right] = neumann(0.0);
p[right] = dirichlet(0.0);
f[right] = neumann(0.0);

// Adaptive mesh refinement
event adapt (i++) {
  adapt_wavelet ({f, u.x, u.y}, (double[]){0.01, 0.1, 0.1}, maxlevel = 10, minlevel = 2);
}

// Output
event logfile (i++) {
  fprintf (stderr, "%d %g\n", i, t);
}

event stop (t = 0.1) {
  return 1;
}
