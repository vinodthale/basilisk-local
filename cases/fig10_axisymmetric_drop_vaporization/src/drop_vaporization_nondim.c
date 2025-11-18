/**
 * @file drop_vaporization_nondim.c
 * @brief Fully nondimensional axisymmetric drop vaporization solver
 * @author Based on ImpactForce-main/Bdropimpact.c structure
 * @date 2025-11-18
 *
 * Complete implementation of axisymmetric drop vaporization following:
 * Boyd & Ling 2023, Computers and Fluids (Figure 10)
 *
 * NONDIMENSIONAL FORMULATION:
 * ==========================
 * Scales:
 *   D = 1.0          (characteristic length D₀)
 *   U_inf = 1.0      (free-stream velocity)
 *   rho_l = 1.0      (liquid density reference)
 *
 * Governing Equations:
 *   - Navier-Stokes (two-phase)
 *   - VOF interface tracking
 *   - Energy equation with diffusion
 *   - Stefan condition for evaporation
 *
 * Parameter Sweep:
 *   Re ∈ [22, 200] (command-line input)
 *   We = 1.5 (fixed)
 *   St = 0.1 (fixed)
 *   Pr = 1.0 (fixed)
 */

#include "constants_vaporization.h"

// Timing variables
double simulation_time_1file, writefile_time_1file;
double simulation_time_total, writefile_time_total;
clock_t simulation_str_time, simulation_end_time;

// CFD values structure
struct CFDValues cfdbv;

// Temperature tracer list
scalar *tracers = {T};

/**
 * ===========================================================================
 * BOUNDARY CONDITIONS
 * ===========================================================================
 */

// Left boundary: inflow
u.n[left] = dirichlet(U_INF_STAR);   // Free-stream velocity
u.t[left] = dirichlet(0);            // No tangential velocity
f[left] = dirichlet(0.);             // Gas phase
T[left] = dirichlet(1.0);            // Free-stream temperature

// Right boundary: outflow
u.n[right] = neumann(0);             // Free outflow
p[right] = dirichlet(0);             // Zero pressure
T[right] = neumann(0);               // Zero gradient

// Top boundary: outflow/far-field
u.n[top] = neumann(0);
p[top] = dirichlet(0);
T[top] = neumann(0);

// Bottom boundary: symmetry (automatically handled by axi.h)

/**
 * ===========================================================================
 * MAIN FUNCTION
 * ===========================================================================
 */
int main(int argc, char **argv)
{
    simulation_str_time = clock();
    simulation_time_1file = 0.0;
    writefile_time_1file = 0.0;

    // Initialize CFD values from command line
    numericalmainvalues(argv, argc, &cfdbv);

    // Set domain size
    size(cfdbv.domainsize);
    origin(0, 0);  // Axisymmetric: x is axial, y is radial

    // Initialize grid
    int initialgrid = pow(2, LEVELmin);
    init_grid(initialgrid);

    // Create output directory
    char comm[80];
    sprintf(comm, "mkdir -p intermediate");
    system(comm);

    // Set physical properties for two-phase flow
    rho1 = cfdbv.rhoL;
    rho2 = cfdbv.rhoG;
    mu1 = cfdbv.muL;
    mu2 = cfdbv.muG;
    f.sigma = cfdbv.Sigma;

    // Set tolerance for Poisson solver
    TOLERANCE = 1e-6;

    // CFL number
    CFL = CFL_NUMBER;

    run();
    return 1;
}

/**
 * ===========================================================================
 * INITIALIZATION
 * ===========================================================================
 */

event init(i = 0)
{
    if (restore(file = FILENAME_LASTFILE))
    {
        // Restored from file
#if AXI
        boundary((scalar *){fm});
#endif
    }
    else
    {
        // Initialize from scratch
        double x0 = cfdbv.drop_center_x;  // Drop center at x = 1.5
        double R0 = cfdbv.radius;          // Drop radius = 0.5
        double Rinf = cfdbv.temp_radius;   // Temperature smoothing = 0.625

        // Refine around drop interface
        refine(sq(x - x0) + sq(y) < sq(R0 + cfdbv.refinegap) &&
               sq(x - x0) + sq(y) > sq(R0 - cfdbv.refinegap) &&
               level < LEVELmax);

        // Initialize fields
        foreach()
        {
            // Distance from drop center
            double r = sqrt(sq(x - x0) + sq(y));

            // ==============================================================
            // Initialize VOF field (f = 1 in liquid, 0 in gas)
            // ==============================================================
            if (r < R0)
            {
                f[] = 1.0;  // Inside drop: liquid
            }
            else
            {
                f[] = 0.0;  // Outside drop: gas
            }

            // ==============================================================
            // Initialize temperature field (Eq. 53)
            // T* = 0 at saturation, T* = 1 at free stream
            // ==============================================================
            if (r <= R0)
            {
                T[] = 0.0;  // Saturation temperature inside drop
            }
            else if (r >= Rinf)
            {
                T[] = 1.0;  // Free-stream temperature far from drop
            }
            else
            {
                // Linear interpolation in transition region
                T[] = (r - R0) / (Rinf - R0);
            }

            // ==============================================================
            // Initialize velocity field
            // ==============================================================
            u.x[] = cfdbv.vel;  // Free-stream velocity (= 1.0)
            u.y[] = 0.0;        // No radial velocity initially
        }

        // Initialize evaporation mass flux to zero
        foreach()
            m_evap[] = 0.0;

        // ==============================================================
        // Open timing file
        // ==============================================================
        clock_t timestr, timeend;
        timestr = clock();
        FILE *fp;
        char name[100], tmp[50];
        sprintf(name, FILENAME_DURATION);
        sprintf(tmp, "-CPU%02d.plt", pid());
        strcat(name, tmp);
        fp = fopen(name, "w");
        fprintf(fp, "Variables = Iteration DeltaTime PhysicalTime t_star LastDuration FileDuration CellNumber TotalLastDuration TotalFileDuration\r\nzone\r\n");
        fclose(fp);
        timeend = clock();
        writefile_time_1file += (double)(timeend - timestr) / CLOCKS_PER_SEC;

        // ==============================================================
        // Open volume history file
        // ==============================================================
        sprintf(name, FILENAME_VOLUME);
        fp = fopen(name, "w");
        fprintf(fp, "# Nondimensional Drop Vaporization - Volume History\n");
        fprintf(fp, "# Re = %.2f, We = %.2f, St = %.2f, Pr = %.2f\n",
                cfdbv.Reynolds, cfdbv.Weber, cfdbv.Stefan, cfdbv.Prandtl);
        fprintf(fp, "# Columns: t+ | t* | Volume | V/V0\n");
        fclose(fp);
    }
}

/**
 * ===========================================================================
 * THERMAL PROPERTIES EVENT
 * ===========================================================================
 * Set phase-dependent thermal diffusivity for temperature diffusion
 */
event properties(i++)
{
    foreach_face()
    {
        double ff = (f[] + f[-1]) / 2.0;

        // Phase-dependent thermal diffusivity
        // α_l* for liquid, α_g* for gas
        if (ff > 0.5)
            alphav.x[] = cfdbv.alpha_l;
        else
            alphav.x[] = cfdbv.alpha_g;
    }
}

/**
 * ===========================================================================
 * EVAPORATION EVENT
 * ===========================================================================
 * Compute mass flux at interface using Stefan condition
 * ṁ* = -(1/(St Re Pr)) |∇T*|_interface
 */
event evaporation(i++)
{
    foreach()
    {
        m_evap[] = 0.0;

        // Only compute at interface cells
        if (f[] > 0.01 && f[] < 0.99)
        {
            // Compute temperature gradient
            double gradT_x = (T[1,0] - T[-1,0]) / (2.0 * Delta);
            double gradT_y = (T[0,1] - T[0,-1]) / (2.0 * Delta);
            double gradT_mag = sqrt(sq(gradT_x) + sq(gradT_y));

            // Stefan condition: ṁ* = -(1/(St Re Pr)) |∇T*|
            double St_Re_Pr = cfdbv.Stefan * cfdbv.Reynolds * cfdbv.Prandtl;
            m_evap[] = -gradT_mag / St_Re_Pr;

            // Note: Negative sign because heat flows into liquid (evaporation)
            // Positive m_evap means mass flux from liquid to gas
        }
    }

    // Apply evaporation to VOF field
    // ∂f/∂t + u·∇f = ṁ/ρ_l
    foreach()
    {
        if (f[] > 0.01 && f[] < 0.99)
        {
            // VOF source term due to evaporation
            double vof_source = m_evap[] / cfdbv.rhoL;
            f[] -= vof_source * dt;

            // Clamp to [0, 1]
            if (f[] < 0.0) f[] = 0.0;
            if (f[] > 1.0) f[] = 1.0;
        }
    }
}

/**
 * ===========================================================================
 * TEMPERATURE DIFFUSION EVENT
 * ===========================================================================
 * Solve diffusion equation and apply latent heat sink
 */
event tracer_diffusion(i++)
{
    // Solve diffusion equation: ∂T/∂t = ∇·(α ∇T)
    diffusion(T, dt, alphav);

    // Apply latent heat sink at interface
    // Energy balance: ρ c_p dT/dt = -ṁ h_lg
    // In nondimensional form: dT/dt = -(St ṁ)/(ρ c_p)
    foreach()
    {
        if (f[] > 0.01 && f[] < 0.99)
        {
            // Effective properties at interface
            double rho_eff = cfdbv.rhoL * f[] + cfdbv.rhoG * (1.0 - f[]);
            double cp_eff = cfdbv.cp_l * f[] + cfdbv.cp_g * (1.0 - f[]);

            // Temperature change due to latent heat
            // dT = -(St ṁ)/(ρ c_p) dt
            double dT_evap = -(cfdbv.Stefan * m_evap[]) / (rho_eff * cp_eff) * dt;
            T[] += dT_evap;

            // Clamp to physical range
            if (T[] < 0.0) T[] = 0.0;
            if (T[] > 1.0) T[] = 1.0;
        }
    }
}

/**
 * ===========================================================================
 * ADAPTIVE MESH REFINEMENT
 * ===========================================================================
 */
event adapt(i++)
{
    double refine_f = pow(10.0, (double)REFINE_VALUE_F);
    double refine_u = pow(10.0, (double)REFINE_VALUE_U);
    double refine_T = pow(10.0, (double)REFINE_VALUE_T);

    adapt_wavelet({f, u.x, u.y, T},
                  (double[]){refine_f, refine_u, refine_u, refine_T},
                  maxlevel = LEVELmax,
                  minlevel = LEVELmin);
}

/**
 * ===========================================================================
 * SHOW ITERATION EVENT
 * ===========================================================================
 */
event showiteration(i++)
{
    switch (pid())
    {
    case 0:
    {
        char name[500], tmp[100];
        double t_star = t * cfdbv.t_star_factor;
        sprintf(name, "i%05d_dt%.2e_t%.4f_tstar%.4f_P%02d",
                i, dt, t, t_star, (int)(100.0 * t / cfdbv.timeend));
        sprintf(tmp, "_Re%.2f_We%.2f", cfdbv.Reynolds, cfdbv.Weber);
        strcat(name, tmp);
        sprintf(tmp, "_AXI_L%02d%02d", LEVELmin, LEVELmax);
        strcat(name, tmp);
        printf("%s\r\n", name);
    }
    }
}

/**
 * ===========================================================================
 * VOLUME TRACKING EVENT
 * ===========================================================================
 */
event volume_tracking(i++)
{
    // Compute liquid volume
    double volume = 0.0;
    foreach(reduction(+:volume))
    {
        volume += f[] * dv();
    }

    // Initial volume (sphere)
    static double V0 = -1.0;
    if (V0 < 0.0)
        V0 = 4.0/3.0 * R_PI * pow(cfdbv.radius, 3.0);

    double V_ratio = volume / V0;
    double t_star = t * cfdbv.t_star_factor;

    // Write to volume history file
    FILE *fp;
    fp = fopen(FILENAME_VOLUME, "a");
    fprintf(fp, "%.6e %.6e %.6e %.6e\n", t, t_star, volume, V_ratio);
    fclose(fp);
}

/**
 * ===========================================================================
 * OUTPUT FILES EVENT
 * ===========================================================================
 */
event outputfiles(t += cfdbv.timestep)
{
    clock_t timestr, timeend;
    timestr = clock();
    static FILE *fp;
    char name[500], tmp[100];

    // Clamp VOF field
    foreach()
    {
        if (f[] < 0.0)
            f[] = 0.0;
        else if (f[] > 1.0)
            f[] = 1.0;
    }

    // Copy pressure for output
    foreach()
    {
        pressure[] = p[];
    }
    p.nodump = false;

    // Save snapshot
    sprintf(name, "intermediate/snapshot-%5.4f", t);
    dump(file = name);

    // Save restart file
    dump(file = FILENAME_LASTFILE);

    timeend = clock();
    writefile_time_1file += (double)(timeend - timestr) / CLOCKS_PER_SEC;

    // Count cells
    int cellnumber = 0;
    foreach()
        cellnumber++;

    // Update timing
    simulation_end_time = clock();
    simulation_time_1file = (double)(simulation_end_time - simulation_str_time) / CLOCKS_PER_SEC;
    simulation_time_total += simulation_time_1file;
    writefile_time_total += writefile_time_1file;

    // Estimate time left
    double estimatetimeleft;
    char LDc[100], TDc[100], ETLc[100];
    if (t == 0.0)
        estimatetimeleft = 0.0;
    else
        estimatetimeleft = simulation_time_total * cfdbv.timeend / t - simulation_time_total;

    timecalculation(simulation_time_1file, LDc);
    timecalculation(simulation_time_total, TDc);
    timecalculation(estimatetimeleft, ETLc);

    // Write timing data
    sprintf(name, FILENAME_DURATION);
    sprintf(tmp, "-CPU%02d.plt", pid());
    strcat(name, tmp);
    fp = fopen(name, "a");
    double t_star = t * cfdbv.t_star_factor;
    fprintf(fp, "%d %e %e %e %e %e %d %e %e\r\n",
            i, dt, t, t_star, simulation_time_1file, writefile_time_1file,
            cellnumber, simulation_time_total, writefile_time_total);
    fclose(fp);

    // Reset timing counters
    simulation_str_time = clock();
    simulation_time_1file = 0.0;
    writefile_time_1file = 0.0;

    // Print progress
    switch (pid())
    {
    case 0:
    {
        printf("\r\nData Files are Written!\r\n");
        printf("t = %.4f, t* = %.4f\r\n", t, t_star);
        printf("Duration Last: %s\r\nTotal: %s, Time Left: %s\r\n\r\n",
               LDc, TDc, ETLc);
        break;
    }
    }
}

/**
 * ===========================================================================
 * END EVENT
 * ===========================================================================
 */
event end(t = cfdbv.timeend)
{
    FILE *fp;
    char name[500], tmp[100];
    sprintf(name, FILENAME_ENDOFRUN);
    sprintf(tmp, "-CPU%02d.txt", pid());
    strcat(name, tmp);
    fp = fopen(name, "w");
    fprintf(fp, "SimulationTime %e\r\nWriteFileTime %e\r\n",
            simulation_time_total, writefile_time_total);
    fclose(fp);

    double t_star = cfdbv.timeend * cfdbv.t_star_factor;
    fprintf(stderr, "\nSimulation completed!\n");
    fprintf(stderr, "  Final time: t = %.4f (t* = %.4f)\n", cfdbv.timeend, t_star);
    fprintf(stderr, "  Total simulation time: %.2f seconds\n", simulation_time_total);
}
