// ============================================================================
// Axisymmetric two-phase setup (Basilisk C)
// This file defines constants, boundary conditions, storage fields, and
// initializes the solver using nondimensional properties derived from
// dimensional reference values.
// ============================================================================

#include "axi.h"                       // Axisymmetric geometry (2D meridional plane with y as radial coordinate)
#include "navier-stokes/centered.h"    // Incompressible Navier–Stokes solver (centered scheme for u,p)
#define FILTERED 1                     // Use filtered jump conditions for density/viscosity across the VOF interface
#include "two-phase.h"                 // Two-phase (VOF) model: volume fraction f, interface advection, mixture props
#include "navier-stokes/conserving.h"  // Conservative form for momentum transport (helps with sharp property jumps)
#include "tension.h"                   // Capillary forces: adds surface tension term proportional to curvature
//#include "tag.h"                       // Utilities for tagging connected VOF regions (e.g., to track droplets)
//#include "curvature.h"                 // Curvature computation for the VOF interface
// --------------------------- Dimensional reference values ---------------------------
// These are convenient physical references (SI units). The actual run below
// uses nondimensionalized properties computed from these values.
#define VELOCITY            19.588      // m/s   : freestream gas velocity
#define DROP_DIAMETER       0.0019      // m     : droplet diameter
#define RHO_L               1000.0      // kg/m³ : liquid density (water)
#define RHO_G               1.2         // kg/m³ : gas   density (air)
#define MU_L                0.001       // Pa·s  : liquid dynamic viscosity
#define MU_G                0.000018    // Pa·s  : gas   dynamic viscosity
#define SIGMA               0.0729      // N/m   : surface tension (water–air)


// --------------------------- Nondimensional base scales -----------------------------
// Reference scales for nondimensionalization (set to 1 here so that derived
// groups like Re, We, Oh match dimensional values above).
#define bvaluesrhoG         1.0         // reference gas density scale
#define bvaluesvel          1.0         // reference velocity scale
#define bvaluesdiameter     1.0         // reference length scale

// --------------------------- Dimensionless groups ----------------------------------
// Classic dimensionless numbers built from the dimensional references.
#define bvaluesReynolds (RHO_G * VELOCITY * DROP_DIAMETER / MU_G)                     // Re  = ρ_g U D / μ_g
#define bvaluesWeber   (RHO_G * VELOCITY * VELOCITY * DROP_DIAMETER / SIGMA)          // We  = ρ_g U² D / σ
#define bvaluesOh      (MU_L / sqrt(RHO_L * SIGMA * DROP_DIAMETER))                    // Oh  = μ_l / √(ρ_l σ D)

// --------------------------- Derived nondimensional properties ----------------------
// These are the actual nondimensional material parameters used in main().
// They are built so that when U*=1 and D*=1, the solver “feels” the target Re/We.
#define bvaluesmuG   (bvaluesrhoG * bvaluesvel * bvaluesdiameter / bvaluesReynolds)    // μ*_g from Re
#define bvaluesSigma (bvaluesrhoG * bvaluesvel * bvaluesvel * bvaluesdiameter / bvaluesWeber) // σ* from We
#define bvaluesrhoL  ((RHO_L / RHO_G) * bvaluesrhoG)                                   // ρ*_l = ρ_l/ρ_g × ρ*_g
#define bvaluesmuL   ((MU_L  / MU_G ) * bvaluesmuG)                                    // μ*_l = μ_l/μ_g × μ*_g

// --------------------------- Geometric helpers (ellipsoid from AR) ------------------
// If used for initial geometry: Dh ∼ AR^(1/3), Dv ∼ AR^(−2/3) so that Dh/Dv = AR.
#define HORIZONTAL_DIAMETER (bvaluesdiameter * pow(ASPECT_RATIO, 1.0/3.0))   // D_h*
#define VERTICAL_DIAMETER   (bvaluesdiameter * pow(ASPECT_RATIO, -2.0/3.0))  // D_v*

// --------------------------- AMR and runtime controls --------------------------------
int maxlevel = 13;                    // maximum refinement level (finest grid)
int minlevel = 10;                    // minimum refinement level (initial grid)

#define tmax              250.0         // nominal final time (may be overridden elsewhere)
#define tsnap             1.00          // output snapshot interval (solver time units)

#define ASPECT_RATIO      2.00          // Dh/Dv : horizontal-to-vertical diameter ratio for an ellipsoidal drop
#define INITIALDISTANCE   5.00          // initial drop center offset from inlet (nondimensional)
#define REFINEGAP         0.02          // half-thickness of refinement band around interface
#define R_PI 3.14159265358979323846     // high-precision π (Basilisk also provides 'pi')

// --------------------------- Boundary conditions ------------------------------------
// LEFT boundary (inflow along +x)
u.n[left] = dirichlet(1.);            // impose u_x = 1 (sets the nondimensional freestream speed)
u.t[left] = dirichlet(0.);            // no cross-stream velocity at inflow (u_y = 0)
p[left]   = neumann(0.);              // zero normal gradient of pressure at inflow

// RIGHT boundary (outflow)
u.n[right] = neumann(0.);             // zero normal velocity gradient (convective outflow)
u.t[right] = neumann(0.);             // zero tangential velocity gradient
p[right]   = dirichlet(0.);           // set reference pressure level at the outlet

// TOP boundary (slip wall at large radius y)
// Axis y=0 is the symmetry line handled internally by axi.h (no explicit BC needed there).
u.n[top] = dirichlet(0.);             // no penetration at the far radial boundary
u.t[top] = neumann(0.);               // free slip (no shear stress)
//p[top]   = dirichlet(0.);            // optional: fix pressure on top boundary if needed

// --------------------------- Convenience fields for post-processing -----------------
scalar pressure[], pressureGas[], pressureLiquid[];  // will be filled from p and f when writing snapshots

// --------------------------- Global accumulators (optional use) ----------------------
double de = 0., dee = 0., se = 0.;    // placeholders for dissipation/energy tracking if used later

int main(int argc, char *argv[]) {
  // ------------------------- Domain and grid ----------------------------------------
  // Square domain [X0, X0+L0] × [Y0, Y0+L0]; in axi, y≥0 is the radial direction.
  L0 = 16.0;                          // domain length in both directions (nondimensional)
  X0 = 0.;                            // domain origin in x
  Y0 = 0.;                            // domain origin in y (axis passes through y=0)
  init_grid(1 << minlevel);           // initialize a uniform grid at the coarsest allowed level

  // Create an output directory once (rank 0 only) for intermediate dumps/fields
  if (pid() == 0)
    system("mkdir -p intermediate");

  // ------------------------- Material properties (nondimensional) -------------------
  // These are the values actually used by the solver. They are consistent with the
  // target Re and We computed above when U*=1 and D*=1.
  rho2    = bvaluesrhoG;              // gas   density  ρ*_g
  rho1    = bvaluesrhoL;              // liquid density ρ*_l
  mu1     = bvaluesmuL;               // liquid viscosity μ*_l
  mu2     = bvaluesmuG;               // gas   viscosity μ*_g
  f.sigma = bvaluesSigma;             // surface tension σ*

  TOLERANCE = 1e-6;                   // Poisson/pressure solver tolerance (tight for interface flows)

  run();                              // hand control to Basilisk event loop
}

// -----------------------------------------------------------------------------
// Initialization: create initial droplet or restore from saved state
// -----------------------------------------------------------------------------
event init(t = 0) {
  if (!restore(file = "dump")) {  
    // Ellipsoidal droplet (VOF fraction) in x–y plane
    fraction(f, sq(HORIZONTAL_DIAMETER * 0.5) - (sq((x - (INITIALDISTANCE + HORIZONTAL_DIAMETER)) / HORIZONTAL_DIAMETER) + sq(y / VERTICAL_DIAMETER)));
    // One-line refinement band around interface (2D axi: no z-term)
    refine((sq((x - (INITIALDISTANCE + HORIZONTAL_DIAMETER)) / HORIZONTAL_DIAMETER) + sq(y / VERTICAL_DIAMETER) < sq(0.5 * HORIZONTAL_DIAMETER + REFINEGAP)) && (sq((x - (INITIALDISTANCE + HORIZONTAL_DIAMETER)) / HORIZONTAL_DIAMETER) + sq(y / VERTICAL_DIAMETER) > sq(0.5 * HORIZONTAL_DIAMETER - REFINEGAP)) && level < maxlevel); // Refinement band
    // Start from rest
        foreach() {
      u.x[] = 0.0;
      u.y[] = 0.0;
    }
  } 
  else {
    fprintf(stderr, "Restarted from saved state.\n");
  }
}

// -----------------------------------------------------------------------------
// Adaptive mesh refinement control
// -----------------------------------------------------------------------------
event adapt(i++) {
  adapt_wavelet({f, u}, (double[]){1e-6, 1e-5, 1e-5}, maxlevel);
}

event energy_budgetDCB(i = 0; i++) {
  // --- Open output files (append mode) ---
  static FILE *fp = fopen("energy.txt", "a");
  // --- Write headers on first call ---
  if (i == 0) {
     fprintf(fp, "i  t  KE1  KE2  KE  KEE  SE  DE  DEE  TENG  AREA\n");
  }

  double vd = 0, VD = 0;
  double ke1 = 0, ke2 = 0, ke = 0;
  double area = 0;// gpe = 0;
  double kn = 1.0;
  double PreFactor = 2.0 * pi;
  //reduction(+:gpe)
  foreach (reduction(+:ke1) reduction(+:ke2) reduction(+:ke) reduction(+:area) reduction(+:vd) reduction(+:VD)) {
    double ry = fabs(y) > 1e-10 ? y : 1e-10;

    // Kinetic energy
    ke1 += PreFactor * 0.5 * (rho1 * dv() * f[] * sq(u.x[]) +
                              rho2 * dv() * (1 - f[]) * sq(u.x[]));
    ke2 += PreFactor * 0.5 * (rho1 * dv() * f[] * sq(u.y[]) +
                              rho2 * dv() * (1 - f[]) * sq(u.y[]));
    ke  += PreFactor * 0.5 * rho1 * (sq(u.x[]) + sq(u.y[])) * dv() * f[];

    // Gravitational potential energy
    //gpe += PreFactor * ((rho1 * f[]) + (rho2 * (1 - f[]))) * dv() * G.x * x;

    // Surface energy
    if (f[] > 1e-6 && f[] < 1. - 1e-6) {
      coord p, n = mycs(point, f);
      double alpha = plane_alpha(f[], n);
      double s = plane_area_center(n, alpha, &p);
      area += PreFactor * s * dv() / Delta;
    }

    // Dissipation: liquid only
    vd += PreFactor * f[] * mu1 * dv() * (
      (2. * (sq(u.x[1] - u.x[-1]) +
             sq(u.y[0,1] - u.y[0,-1])) +
       sq(u.y[1] - u.y[-1] + u.x[0,1] - u.x[0,-1])) / sq(2. * Delta)
      - (2./3.) * (u.y[0,1] - u.y[0,-1] + u.x[1] - u.x[-1]) / (2. * Delta)
      + 2. * sq(u.y[] / ry) - (2./3.) * u.y[] / ry
    );

    // Dissipation: full domain
    VD += PreFactor * (
      f[] * mu1 + (1 - f[]) * mu2
    ) * dv() * (
      (2. * (sq(u.x[1] - u.x[-1]) +
             sq(u.y[0,1] - u.y[0,-1])) +
       sq(u.y[1] - u.y[-1] + u.x[0,1] - u.x[0,-1])) / sq(2. * Delta)
      - (2./3.) * (u.y[0,1] - u.y[0,-1] + u.x[1] - u.x[-1]) / (2. * Delta)
      + 2. * sq(u.y[] / ry) - (2./3.) * u.y[] / ry
    );
  }
  double KE1 = kn * ke1, KE2 = kn * ke2, KE = KE1 + KE2;
  double KEE = kn * ke;
  se = area * f.sigma;
  double SE = kn * se;
  de += vd * dt;
  double DE = kn * de;
  dee += VD * dt;
  double DEE = kn * dee;
  //double GPE = kn * gpe;
  double TENG = KE + SE + DEE;
  //double TEYG = KE + SE + DEE + GPE;
  fprintf(fp, "%d  %f  %.10f  %.10f  %.10f  %.10f  %.10f  %.10f  %.10f  %.10f  %.10f\r\n", i, t, KE1, KE2, KE, KEE, SE, DE, DEE, TENG, area);
  fflush(fp);
}

// Event to log drag forces, drag coefficients, and drop kinematics
event log_drag_and_cd(i = 0; i++) {
  // --- Open output files (append mode) ---
  static FILE *fp_drag = fopen("PressureViscousDrag.txt", "a");
  static FILE *fp_cd = fopen("CD.txt", "a");

  // --- Write headers on first call ---
  if (i == 0) {
    fprintf(fp_drag,
      "Step  Time       Total_Drag       Pressure_Force    Viscous_Force     Interface_Area    A_proj    CD_total    CD_pressure    CD_viscous    dR_dt    d2R_dt2    yMax    x_cm\n");
    fprintf(fp_cd,
      "Step    Time    CD    ud    Volume \n");
  }

  // --- Compute height function (interface geometry) ---
  vector h[];
  heights(f, h);  // Computes height function field for interface

  // --- Initialize accumulators for forces and interface area ---
  double interface_area = 0., pressure_force = 0., viscous_force = 0.;

  // --- Loop over interfacial cells to compute:
  //     - Interface area (axisymmetric)
  //     - Pressure drag force (x-direction)
  //     - Viscous drag force (x-direction)
  // Formulas used:
  // dA = 2 * pi * s * vol / Delta   (axisymmetric surface element)
  // Pressure force = -p * n_x * dA
  // Viscous force = (tau_zz * n_x + tau_zr * n_y) * dA
  foreach (reduction(+:interface_area) reduction(+:pressure_force) reduction(+:viscous_force)) {
    if (f[] > 1e-6 && f[] < 1. - 1e-6) {  // Interface cells only
      double vol = dv();  // Cell volume
      coord n = mycs(point, f);  // Interface normal vector
      double alpha = plane_alpha(f[], n);
      coord pc;
      double s = plane_area_center(n, alpha, &pc);  // Interface plane area and center

      double dA = 2. * R_PI * s * vol / Delta;  // Axisymmetric surface element area
      interface_area += dA;

      pressure_force += -p[] * n.x * dA;  // Pressure force contribution

      // Compute viscous stress components (tau_zz and tau_zr)
      double duz_dz = (u.x[1,0] - u.x[-1,0]) / (2. * Delta);
      double duz_dr = (u.x[0,1] - u.x[0,-1]) / (2. * Delta);
      double tau_zz = 2. * mu1 * duz_dz;
      double tau_zr = mu1 * duz_dr;
      viscous_force += (tau_zz * n.x + tau_zr * n.y) * dA;  // Viscous force contribution
    }
  }

  // --- Initialize integrals for drop volume, momentum, mass, max radius, and x_cm numerator ---
  double num_uz = 0., drop_volume = 0., massdrop = 0.0;
  double yMax = -HUGE;
  double x_cm_num = 0.0;
  double x_cm = 0.0;
  // --- Integrate over liquid cells to compute:
  //     - Drop volume: V = ∫ f dV (axisymmetric factor included)
  //     - Momentum: ∫ f * u_x dV
  //     - Mass: m = rho_L * V
  //     - Max radial extent yMax (for projected area)
  //     - Center of mass numerator: ∫ x * f dV
  foreach (reduction(+:num_uz) reduction(+:drop_volume) reduction(+:massdrop) reduction(max:yMax) reduction(+:x_cm_num)) {
    num_uz += f[] * u.x[] * 2.0 * R_PI * dv();   // ∫ f u_x dV (momentum)
    drop_volume += f[] * 2.0 * R_PI * dv();      // ∫ f dV (volume)
    massdrop += rho1 * f[] * 2.0 * R_PI * dv();  // mass = rho_L * volume

    x_cm_num += x * f[] * 2.0 * R_PI * dv();    // ∫ x f dV (center of mass numerator)

    if (h.y[] != nodata) {
      double yi = y + height(h.y[]) * Delta;
      if (yi > yMax) yMax = yi;
    }
  }

  // --- Compute center of mass position in x:
  //     x_cm = (1/V) ∫ x f dV
  if (drop_volume > 1e-12)
    x_cm = x_cm_num / drop_volume;
  else
    x_cm = 0.0;

  // --- Calculate projected area (assuming circular projection):
  //     A_proj = pi * yMax^2
  double A_proj = pi * sq(yMax);

  // --- Calculate total drag force:
  double total_drag = pressure_force + viscous_force;

  // --- Dynamic pressure: q_inf = 0.5 * rho_g * U^2
  double q_inf = 0.5 * rho2 * 1.0 * 1.0;

  // --- Calculate drag coefficients:
  //     CD_pressure = F_pressure / (q_inf * A_proj)
  //     CD_viscous = F_viscous / (q_inf * A_proj)
  //     CD_total = CD_pressure + CD_viscous (should equal total drag / (q_inf * A_proj))
  double CD_p = pressure_force / (q_inf * A_proj);
  double CD_v = viscous_force / (q_inf * A_proj);
  double CD_calc = total_drag / (q_inf * A_proj);

  // --- Numerical consistency check for drag coefficients ---
  if (fabs(CD_calc - (CD_p + CD_v)) > 1e-10) {
    fprintf(stderr,
      "Warning: Drag coefficient check failed at t = %g, |CD - (CD_p + CD_v)| = %.3e\n",
      t, fabs(CD_calc - (CD_p + CD_v)));
  }

  // --- Compute radial velocity dR/dt and acceleration d2R/dt2:
  //     Using backward finite differences on max radius yMax
  static double yMax_prev = -HUGE;
  static double dR_dt_prev = 0.0;
  double dR_dt = 0.0;
  if (yMax_prev != -HUGE) {
    dR_dt = (yMax - yMax_prev) / dt;
  }
  double d2R_dt2 = 0.0;
  if (i > 1) {
    d2R_dt2 = (dR_dt - dR_dt_prev) / dt;
  }

  // --- Write all drag, kinematic, and geometry data to drag file ---
  fprintf(fp_drag, "%5d  %10.4f  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e\n",
          i, t,
          total_drag, pressure_force, viscous_force,
          interface_area, A_proj,
          CD_calc, CD_p, CD_v,
          dR_dt, d2R_dt2, yMax,
          x_cm);
  fflush(fp_drag);

  // --- Calculate center-of-mass velocity and acceleration ---
  static double ud_prev = 0.;
  double ud = 0., dud_dt = 0., CD = NAN;

  if (drop_volume > 1e-12) {
    ud = num_uz / drop_volume;            // u_d = (1/V) ∫ f u_x dV
    dud_dt = (ud - ud_prev) / dt;         // du_d/dt (backward difference)
    double relU = 1.0 - ud;         // Relative velocity between gas and drop

    // --- Calculate drag coefficient from acceleration:
    //     CD = (2 m / (rho_g * A_proj * relU^2)) * du_d/dt
    if (fabs(relU) > 1e-8 && yMax > 1e-12) {
      CD = (2.0 * massdrop * dud_dt) / (rho2 * relU * relU * A_proj);
    } else {
      fprintf(stderr, "Warning: relU ≈ 0 or yMax too small at t = %g — Skipping CD calculation.\n", t);
    }
  } else {
    fprintf(stderr, "Warning: drop_volume ≈ 0 at t = %g — Skipping force calculation.\n", t);
  }

  // --- Write drag coefficient, center-of-mass velocity, and drop volume to CD file ---
  fprintf(fp_cd, "%5d  %10.4f  %12.6e  %12.6e  %12.6e\n", i, t, CD, ud, drop_volume);
  fflush(fp_cd);

  // --- Update previous step values for next iteration ---
  ud_prev = ud;
  yMax_prev = yMax;
  dR_dt_prev = dR_dt;
}


// Event to quantify gas-phase vorticity and circulation in 2D axisymmetric simulation
event quantify_gas_vorticity (i = 0; i++) {
  static FILE *fp_vort = fopen("GasVorticity.txt", "a"); // Open file once in append mode

  // Write header on first call
  if (i == 0) {
    fprintf(fp_vort, "Step  Time    Omega_max    Omega_min    Omega_mean    Omega_RMS    Omega_pos    Omega_neg    Circulation\n");
  }

  scalar omega[];
  vorticity(u, omega); // Compute the vorticity field ω = ∂x u_y - ∂y u_x with metric corrections

  // Initialize accumulators for vorticity statistics and circulation
  double omega_max = -HUGE, omega_min = HUGE;
  double omega_sum = 0., omega_sq_sum = 0.;
  double omega_pos_sum = 0., omega_neg_sum = 0.;
  double volume = 0.;
  double circulation = 0.;

  // User-defined threshold for identifying gas phase cells:
  // Cells with volume fraction f[] less than gas_threshold are considered gas
  const double gas_threshold = 1e-3;

  /*
  | Quantity                               | Physical Meaning                | Formula / Calculation                                       |
  | -------------------------------------- |---------------------------------|-------------------------------------------------------------|
  | Max/Min Vorticity                      | Peak rotational strength        | max(ω), min(ω)                                              |
  | Mean Vorticity                         | Average rotation in gas         | \overline{ω} = (1/V) ∫ ω dV                                 |
  | RMS Vorticity                          | Fluctuation intensity           | sqrt[(1/V) ∫ ω² dV]                                         |
  | Integrated Positive/Negative Vorticity | Net positive/negative rotation  | ∫_{ω>0} ω dV, ∫_{ω<0} ω dV                                  |
  | Circulation Γ                          | Net rotation enclosed by path   | Γ = ∫∫ ω · 2 π r dA                                         |
  | Enstrophy                              | Turbulent intensity measure     | E = ∫ ω² dV                                                 |
  */

  foreach (reduction(max:omega_max) reduction(min:omega_min) reduction(+:omega_sum) reduction(+:omega_sq_sum) reduction(+:omega_pos_sum) reduction(+:omega_neg_sum) reduction(+:volume) reduction(+:circulation)) {
    // Identify gas phase cells using the user-defined threshold
    if (f[] < gas_threshold) {
      double w = omega[];
      omega_max = max(omega_max, w);
      omega_min = min(omega_min, w);

      // Axisymmetric volume element: dv * 2π * y
      double vol_axi = f[] * 2.0 * R_PI * dv();      // ∫ f dV (volume)

      omega_sum += w * vol_axi;           // For mean calculation
      omega_sq_sum += w * w * vol_axi;    // For RMS calculation

      if (w > 0.)
        omega_pos_sum += w * vol_axi;     // Integrated positive vorticity
      else
        omega_neg_sum += w * vol_axi;     // Integrated negative vorticity

      volume += vol_axi;                  // Total gas volume in axisymmetric domain
      circulation += w * vol_axi;         // Circulation Γ = ∫∫ ω * 2πr dA
    }
  }

  // Avoid division by zero for mean and RMS calculations
  double omega_mean = (volume > 1e-12) ? (omega_sum / volume) : 0.0;
  double omega_rms  = (volume > 1e-12) ? sqrt(omega_sq_sum / volume) : 0.0;

  // Write results to file
  fprintf(fp_vort, "%5d  %10.4f  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e  %12.6e\n",
          i, t,
          omega_max, omega_min, omega_mean, omega_rms,
          omega_pos_sum, omega_neg_sum, circulation);
  fflush(fp_vort);
  // Close file if this is the last iteration (optional, can keep open for continuous logging)
}


event printall(i++) {
  printf("i[%06d], dt[%e], t[%.2f]\n", i, dt, t);
}

event writingFiles(t = 0; t <= tmax; t += tsnap) {
  char nameOut[80];

  // Compute pressures
  foreach () {
    pressure[] = p[];
    pressureGas[] = p[] * (1. - f[]);
    pressureLiquid[] = p[] * f[];
  }

  scalar *list = (scalar *){f, u.x, u.y, p, pressure, pressureGas, pressureLiquid};

  // Save full state
  dump(file = "dump", list = list);

  // Save intermediate snapshot
  sprintf(nameOut, "intermediate/snapshot-%5.4f", t);
  dump(file = nameOut, list = list);
}



