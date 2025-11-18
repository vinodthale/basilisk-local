/**
 * First Basilisk Simulation
 * A simple test to verify the Basilisk installation
 */

#include "grid/multigrid.h"
#include "navier-stokes/centered.h"

int main() {
  // Initialize a 128x128 grid
  init_grid (128);

  // Run the simulation
  run();
}
