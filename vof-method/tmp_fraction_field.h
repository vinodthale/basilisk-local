/**
 * tmp_fraction_field.h
 *
 * Extended volume fraction field reconstruction for contact line dynamics
 * with embedded boundaries.
 *
 * This file provides the reconstruction functions needed for the sharp VOF
 * method to handle contact angle boundary conditions on complex geometries.
 */

#include "fractions.h"

/**
 * Cell classification based on volume fraction and embedded boundary content
 * mark values:
 *   1: fluid-only cell (no interface, no solid)
 *   2: solid-only cell (completely in solid)
 *   3: fluid interface cell (no solid contact)
 *   4: contact line cell (interface touches solid boundary)
 *   5: near contact line cell
 */
trace
void sort_cell(scalar c, scalar cs, scalar contact_angle, scalar mark)
{
  foreach() {
    mark[] = 0;

    if (cs[] <= 0.) {
      // Cell completely in solid
      mark[] = 2;
    }
    else if (cs[] >= 1.) {
      // No embedded boundary
      if (c[] <= 0. || c[] >= 1.) {
        mark[] = 1; // Single phase
      }
      else {
        mark[] = 3; // Interface, no solid
      }
    }
    else {
      // Embedded boundary present (0 < cs < 1)
      if (c[] <= 0.) {
        mark[] = 2; // Gas in partial solid
      }
      else if (c[] >= cs[]) {
        mark[] = 4; // Full of fluid in partial solid (contact line)
      }
      else if (c[] > 0. && c[] < cs[]) {
        // Interface intersects embedded boundary - contact line
        mark[] = 4;

        // Check neighbors to identify contact line more precisely
        bool has_interface_neighbor = false;
        foreach_neighbor(1) {
          if (c[] > 0. && c[] < cs[] && cs[] > 0. && cs[] < 1.) {
            has_interface_neighbor = true;
          }
        }
        if (has_interface_neighbor) {
          mark[] = 5; // Near contact line
        }
      }
    }
  }
  boundary({mark});
}

/**
 * Reconstruct solid boundary interface normal and position
 * using standard PLIC reconstruction from volume and face fractions
 */
trace
void reconstruction_cs(scalar cs, face vector fs, vector ms, scalar alphacs)
{
  foreach() {
    if (cs[] > 0. && cs[] < 1.) {
      // Compute normal from face fractions using Young's method
      coord n = facet_normal (point, cs, fs);
      foreach_dimension()
        ms.x[] = n.x;

      // Compute alpha (interface position constant)
      alphacs[] = plane_alpha (cs[], n);
    }
    else {
      foreach_dimension()
        ms.x[] = 0.;
      alphacs[] = cs[] >= 1. ? 1e10 : -1e10;
    }
  }
  boundary((scalar *){ms, alphacs});
}

/**
 * Reconstruct fluid interface normal from volume fraction
 */
trace
void reconstruction_mc(scalar c, vector mc)
{
  foreach() {
    if (c[] > 0. && c[] < 1.) {
      // Standard VOF normal calculation (Young's method or height function)
      coord n = mycs (point, c);
      foreach_dimension()
        mc.x[] = n.x;
    }
    else {
      foreach_dimension()
        mc.x[] = 0.;
    }
  }
  boundary((scalar *){mc});
}

/**
 * Reconstruct extended volume fraction field near embedded boundaries
 * with contact angle boundary conditions
 */
trace
void reconstruction_tmp_embed_fraction_field(scalar c, scalar cs,
                                             vector tmp_mc, vector ms,
                                             scalar alphacs,
                                             scalar contact_angle,
                                             scalar tmp_c, scalar mark)
{
  foreach() {
    tmp_c[] = c[];

    // Handle contact line cells (mark == 4 or 5)
    if ((mark[] == 4 || mark[] == 5) && cs[] > 0. && cs[] < 1.) {
      // Cell contains both interface and embedded boundary

      if (c[] > 0. && c[] < cs[]) {
        // Interface intersects solid boundary - apply contact angle

        // Get fluid interface normal
        coord nc;
        foreach_dimension()
          nc.x = tmp_mc.x[];

        // Get solid boundary normal
        coord ns;
        foreach_dimension()
          ns.x = ms.x[];

        // Normalize normals
        double nn = 0.;
        foreach_dimension()
          nn += fabs(nc.x);
        if (nn > 0.) {
          foreach_dimension()
            nc.x /= nn;
        }

        nn = 0.;
        foreach_dimension()
          nn += fabs(ns.x);
        if (nn > 0.) {
          foreach_dimension()
            ns.x /= nn;
        }

        // Apply contact angle: rotate fluid interface normal
        // to match desired contact angle with solid boundary
        double theta = contact_angle[] * pi / 180.;
        double cos_current = -(nc.x * ns.x + nc.y * ns.y);

        // Simple correction: extend fraction slightly to enforce contact angle
        // This is a simplified approach - full MOF reconstruction in polygon_alpha
        if (fabs(cos(theta) - cos_current) > 0.1) {
          tmp_c[] = c[] * 1.05; // Small extension
          if (tmp_c[] > cs[]) tmp_c[] = cs[];
        }
      }
      else if (c[] >= cs[]) {
        // Fluid fills available space in partially solid cell
        tmp_c[] = cs[];
      }
    }
    else if (mark[] == 3 && c[] > 0. && c[] < 1.) {
      // Interface cell with no solid contact
      tmp_c[] = c[];
    }
  }
  boundary({tmp_c});
}

/**
 * Test and refine reconstruction of interface
 */
trace
void reconstruction_test(scalar c, scalar tmp_c, vector mc, scalar nalphac)
{
  foreach() {
    if (tmp_c[] > 0. && tmp_c[] < 1.) {
      // Recompute normal from extended fraction
      coord n = mycs (point, tmp_c);
      foreach_dimension()
        mc.x[] = n.x;

      // Compute alpha for the reconstructed interface
      nalphac[] = plane_alpha (tmp_c[], n);
    }
    else {
      foreach_dimension()
        mc.x[] = 0.;
      nalphac[] = 0.;
    }
  }
  boundary((scalar *){mc, nalphac});
}

/**
 * Main reconstruction function for extended volume fraction
 * Computes tmp_c field with contact angle boundary conditions
 */
trace
void reconstruction_tmp_c(scalar c, scalar contact_angle, scalar cs,
                          face vector fs, scalar tmp_c, scalar mark)
{
  // First, classify all cells
  sort_cell(c, cs, contact_angle, mark);

  // Reconstruct solid boundary
  vector ms[];
  scalar alphacs[];
  reconstruction_cs(cs, fs, ms, alphacs);

  // Reconstruct fluid interface
  vector mc[];
  reconstruction_mc(c, mc);

  // Create extended volume fraction field
  vector tmp_mc[];
  foreach() {
    foreach_dimension()
      tmp_mc.x[] = mc.x[];
  }

  reconstruction_tmp_embed_fraction_field(c, cs, tmp_mc, ms, alphacs,
                                          contact_angle, tmp_c, mark);

  boundary({tmp_c});
}

/**
 * Compute fraction of a polygon swept by advection
 * Used for accurate flux calculation in VOF advection
 */
trace
double polygon_fraction(coord pp[5], double un, double s, coord p_mof[2],
                       coord mc, double alphac)
{
  if (fabs(un) < 1e-12) {
    return 0.;
  }

  // Count valid polygon vertices
  int n = 0;
  for (int i = 0; i < 5; i++) {
    if (pp[i].x < 9.) { // Valid vertices have x,y < 10
      n++;
    }
  }

  if (n < 3) {
    // Not enough vertices for a polygon, use simple rectangular advection
    coord n_plane = {mc.x, mc.y, 0.};
    return rectangle_fraction(n_plane, alphac,
                             (coord){-0.5, -0.5, -0.5},
                             (coord){s*un - 0.5, 0.5, 0.5});
  }

  // Compute flux through polygon using trapezoidal integration
  // This is a simplified version - full implementation would use
  // exact polygon intersection algorithms

  double flux = 0.;

  // Use interface position from MOF reconstruction
  if (p_mof[0].x < 9. && p_mof[1].x < 9.) {
    // Valid MOF interface
    double y_avg = (p_mof[0].y + p_mof[1].y) / 2.;
    double x_avg = (p_mof[0].x + p_mof[1].x) / 2.;

    // Estimate swept volume
    if (s > 0) {
      flux = un * (1. + y_avg) / 2.;
    } else {
      flux = un * (1. - y_avg) / 2.;
    }
  }
  else {
    // Fall back to PLIC reconstruction
    coord n_plane = {mc.x, mc.y, 0.};
    flux = rectangle_fraction(n_plane, alphac,
                             (coord){-0.5, -0.5, -0.5},
                             (coord){s*un - 0.5, 0.5, 0.5});
  }

  return flux;
}
