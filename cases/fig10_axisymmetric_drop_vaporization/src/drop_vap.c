/**
 * Axisymmetric drop vaporization simulation (Figure 10)
 *
 * Water drop vaporizing in hot steam environment
 * Based on static_bubble.c structure
 */

// Author: Vinod Thale (corrected header structure for axisymmetric simulation)
#include "axi.h"                       // Axisymmetric geometry (handles grid internally)
#include "navier-stokes/centered.h"    // Two-phase Navier-Stokes solver
#define FILTERED                       // Smear density and viscosity jumps
#include "two-phase.h"                 // Density and viscosity for two phases
#include "tension.h"                   // Surface tension

// Parameters (avoid conflicts with Basilisk built-ins like L0)
double drop_diameter = 1.0;
double drop_radius = 0.5;
double domain_size = 8.0;
double drop_center_x = 1.5;

double REYNOLDS = 100.0;
double WEBER = 1.5;

int MAXLEVEL = 8;

int main(int argc, char **argv)
{
  if (argc >= 2) REYNOLDS = atof(argv[1]);
  if (argc >= 3) WEBER = atof(argv[2]);

  size(domain_size);
  origin(0, 0);

  N = 1 << 6;  // 64x64 base grid

  // Density ratio (water/steam)
  rho1 = 958.4 / 0.597;
  rho2 = 1.0;

  // Viscosity ratio
  mu1 = 2.8e-4 / 1.26e-5;
  mu2 = 1.0;

  // Surface tension
  f.sigma = 1.0 / WEBER;

  // (Temperature field removed for now)

  TOLERANCE = 1e-4;
  DT = 0.01;

  run();
}

event init(t = 0)
{
  // Initialize drop as VOF
  fraction(f, sq(drop_radius) - sq(x - drop_center_x) - sq(y));

  // Initial velocity (inflow from left)
  foreach() {
    u.x[] = 1.0;
    u.y[] = 0.0;
  }
}

// Boundary conditions
u.n[left] = dirichlet(1.0);
f[left] = dirichlet(0.0);

// Adaptive refinement
#if TREE
event adapt(i++)
{
  adapt_wavelet({f, u.x, u.y},
                (double[]){0.01, 0.1, 0.1},
                maxlevel = MAXLEVEL, minlevel = 2);
}
#endif

// Volume tracking
event volume(i += 10)
{
  double vol = 0.0;
  foreach(reduction(+:vol))
    vol += f[] * dv();

  if (i == 0)
    fprintf(stderr, "# i t volume\n");

  fprintf(stderr, "%d %g %g\n", i, t, vol);
}

event stop(t = 0.16)
{
  return 1;
}
