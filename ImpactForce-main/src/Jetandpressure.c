// Force calculation post-processing for Basilisk simulations
// Supports both 2D Cartesian and Axisymmetric modes
// Compile with: qcc -dimension=2 -DAXI=0 Jetandpressure.c  (for 2D Cartesian)
//          or: qcc -dimension=2 -DAXI=1 Jetandpressure.c  (for Axisymmetric)

#if dimension != 2
  #error "This code requires dimension = 2. Compile with -dimension=2 flag"
#endif

#if AXI
  #include "axi.h"
#endif
#include "navier-stokes/centered.h"
#define FILTERED 1          // Smear density and viscosity jumps
#include "two-phase.h"
#include "tension.h"
#include "tag.h"
#include "curvature.h"


#define VOFFOLDER "FVFVDb0.00delta0.00V0.18"
scalar pressure[];
double cfdbvbubblediameter = 0.00;

#if AXI
  // Axisymmetric: integrate with 2πr factor for revolution
  double PreFactor = 2*pi;
#else
  // 2D Cartesian: integrate without revolution factor (force per unit depth)
  double PreFactor = 1.0;
#endif
int main (int argc, char **argv)
{
  run();
}

event init (t = 0)
{
    double PressureDropMaxima;
    double timebgn = 0.00;
    double timestp = 0.01;
    double timeend = 2.04;
    ;
    char namefile[500];
    char name[100];
    double timeload;
    FILE *ForceonLfet;
    char folder[500];
    strcpy(folder, "mkdir");
    strcat(folder, VOFFOLDER);
    system(folder);
    for (timeload = timebgn; timeload <= timeend; timeload += timestp)
   {
    sprintf (namefile, "intermediate/snapshot-%5.4f", timeload);
    printf ("load the file %s!\r\n", namefile);
    restore (file = namefile);
   //calculate the force on the substrate
    double pleft = 0.;
    double pForce  = 0.;
    static int nff = 0;
      sprintf (name, "%s/ForceFVDb0.00delta0.00V0.18.txt", VOFFOLDER);
    if (!nff)
      ForceonLfet = fopen(name, "w");
    else
    ForceonLfet = fopen(name, "a");
    double pdatum = 0, wt = 0;

    // Calculate reference pressure at top boundary
    foreach_boundary(top){
      #if AXI
        // Axisymmetric: weight by 2πr (where r = y coordinate in Basilisk axi)
        pdatum += 2*pi*y*pressure[]*(Delta);
        wt += 2*pi*y*(Delta);
      #else
        // 2D Cartesian: simple linear integration
        pdatum += pressure[]*(Delta);
        wt += (Delta);
      #endif
    }
    if (wt > 0){
      pdatum /= wt;
    }

    // Calculate force on left boundary
    foreach_boundary(left)
    {
      #if AXI
        // Axisymmetric: integrate with 2πr factor for full 3D force
        pForce += 2*pi*y*(Delta)*(pressure[]-pdatum);
      #else
        // 2D Cartesian: force per unit depth (no revolution factor)
        pForce += (Delta)*(pressure[]-pdatum);
      #endif
      pleft += pressure[];
    }
    boundary((scalar *){f, u.x, u.y, pressure});
    ;
    fprintf (ForceonLfet, "%f  %.10f %.10f\r\n", timeload, pForce, pleft);
    fclose (ForceonLfet);
    nff++;
 }
}

event end(t = 0.0)
{
    printf("\r\n-------\r\nEND!\r\n");
}





