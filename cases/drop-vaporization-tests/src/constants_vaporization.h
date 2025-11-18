/**
 * @file constants_vaporization.h
 * @brief Constants and configuration for axisymmetric drop vaporization
 * @author Based on ImpactForce-main structure, adapted for vaporization
 *
 * This header provides nondimensional parameters, CFD values structure,
 * and helper functions for the axisymmetric drop vaporization simulation.
 *
 * Reference: Boyd & Ling 2023, Computers and Fluids (Figure 10)
 */

#include "axi.h"                       // axisymmetric geometry
#include "navier-stokes/centered.h"    // solve NS equations
#define FILTERED                       // Smear density and viscosity jumps
#include "two-phase.h"                 // Solve two-phase equation
#include "tension.h"                   // include surface tension between phases
#include "tracer.h"                    // For temperature field
#include "diffusion.h"                 // For heat diffusion

/**
 * NONDIMENSIONALIZATION MODE
 * 'd' = dimensional (not used for this test)
 * 'n' = nondimensional (standard mode)
 */
#define DIM_NONDIM_MODE 'n'

/**
 * ===========================================================================
 * NONDIMENSIONAL FORMULATION (MODE 'n')
 * ===========================================================================
 * Reference scales:
 *   Length:     D₀ (drop diameter) = 1.0
 *   Velocity:   U∞ (free-stream)  = 1.0
 *   Density:    ρ_l (liquid)      = 1.0
 *   Time:       t_ref = D₀/U∞
 *   Pressure:   p_ref = ρ_l U∞²
 *
 * Material ratios (FIXED):
 *   eta = ρ_g/ρ_l = 0.000623
 *   mu_ratio = μ_g/μ_l = 0.045
 *   k_ratio = k_g/k_l = 0.0368
 *   cp_ratio = c_p,g/c_p,l = 0.4815
 *
 * Nondimensional groups (FIXED except Re):
 *   We = 1.5   (Weber number)
 *   St = 0.1   (Stefan number)
 *   Pr = 1.0   (Prandtl number)
 *   Re = variable (swept from 22 to 200)
 */

#if DIM_NONDIM_MODE == 'n' || DIM_NONDIM_MODE == 'N'

// Nondimensional groups
#define WEBER                1.5
#define STEFAN               0.1
#define PRANDTL              1.0
#define REYNOLDS             100.0  // Default, overridden by command line

// Material ratios (FIXED - from dimensional properties)
#define RHO_RATIO            0.000623    // ρ_g/ρ_l = 0.597/958.4
#define MU_RATIO             0.045       // μ_g/μ_l = 1.26e-5/2.8e-4
#define K_RATIO              0.0368      // k_g/k_l = 0.025/0.679
#define CP_RATIO             0.4815      // c_p,g/c_p,l = 2030/4216

// Nondimensional reference values
#define RHO_L_STAR           1.0         // Liquid density (reference)
#define RHO_G_STAR           RHO_RATIO   // Gas density
#define U_INF_STAR           1.0         // Free-stream velocity
#define D_CHAR               1.0         // Drop diameter
#define SIGMA_STAR           (1.0/WEBER) // Surface tension coefficient

// Placeholder dimensional values (not used in nondim mode)
#define VELOCITY             0.0
#define DROP_DIAMETER        0.0
#define RHO_L                0.0
#define MU_L                 0.0
#define SIGMA                0.0
#define RHO_G                0.0
#define MU_G                 0.0

#else
#error "Only nondimensional mode 'n' is supported for vaporization"
#endif

/**
 * ===========================================================================
 * GEOMETRIC PARAMETERS (NONDIMENSIONAL)
 * ===========================================================================
 */
#define DOMAIN_WIDTH         8.0     // Domain size: 8D₀ × 8D₀
#define DROP_RADIUS          0.5     // R₀ = D₀/2
#define DROP_CENTER_X        1.5     // Drop center at x = 1.5D₀
#define DROP_CENTER_R        0.0     // On axis (r = 0)
#define TEMP_SMOOTH_RADIUS   0.625   // R_inf = 1.25*R₀ for temperature profile

/**
 * ===========================================================================
 * MESH PARAMETERS
 * ===========================================================================
 */
#define INITAL_GRID_LEVEL    6       // Base grid: 2^6 = 64
#define MAX_GRID_LEVEL       12      // Max refinement: 2^12 = 4096 cells
#define REFINE_GAP           0.02    // Refinement band width

/**
 * ===========================================================================
 * REFINEMENT CRITERIA
 * ===========================================================================
 */
#define REFINE_VAR           {f, u.x, u.y, T}
#define REFINE_VAR_TEXT      "f, u.x, u.y, T"
#define REFINE_VALUE_F       -2      // VOF refinement: 10^-2 = 0.01
#define REFINE_VALUE_U       -2      // Velocity refinement: 10^-2 = 0.01
#define REFINE_VALUE_T       -2      // Temperature refinement: 10^-2 = 0.01

/**
 * ===========================================================================
 * TIME CONTROL
 * ===========================================================================
 */
#define CFL_NUMBER           0.2     // CFL condition
#define MAX_TIME             6.4     // End time in solver units (t⁺ = t U∞/D₀)
                                     // Corresponds to t* = 0.16 in paper time
#define SAVE_FILE_EVERY      0.05    // Output interval

/**
 * ===========================================================================
 * OUTPUT FILES
 * ===========================================================================
 */
#define FILENAME_DATA        "data"
#define FILENAME_DURATION    "duration"
#define FILENAME_PARAMETERS  "parameters.txt"
#define FILENAME_ENDOFRUN    "endofrun"
#define FILENAME_LASTFILE    "lastfile"
#define FILENAME_VOLUME      "volume_history.txt"
#define FILENAME_NUSSELT     "nusselt_history.txt"

/**
 * ===========================================================================
 * CONSTANTS
 * ===========================================================================
 */
#define R_VOFLIMIT           1.0e-9
#define R_PI                 3.1415926535897932384626433832795

/**
 * ===========================================================================
 * GLOBAL VARIABLES
 * ===========================================================================
 */
int LEVELmin = INITAL_GRID_LEVEL;
int LEVELmax = MAX_GRID_LEVEL;
double maxruntime = HUGE;

// Scalar fields
scalar f[];                // Volume fraction (liquid)
scalar T[];                // Temperature field (nondimensional)
scalar m_evap[];           // Evaporation mass flux
scalar pressure[];         // Pressure field for output

// Face vector for thermal diffusivity
face vector alphav[];

/**
 * ===========================================================================
 * CFD VALUES STRUCTURE
 * ===========================================================================
 * Stores all simulation parameters in nondimensional form
 */
struct CFDValues {
    // Nondimensional groups
    double Reynolds;
    double Weber;
    double Stefan;
    double Prandtl;

    // Material ratios
    double rho_ratio;      // ρ_g/ρ_l
    double mu_ratio;       // μ_g/μ_l
    double k_ratio;        // k_g/k_l
    double cp_ratio;       // c_p,g/c_p,l

    // Nondimensional properties (for Basilisk)
    double rhoL;           // Liquid density (= 1.0)
    double rhoG;           // Gas density (= rho_ratio)
    double muL;            // Liquid viscosity
    double muG;            // Gas viscosity
    double Sigma;          // Surface tension coefficient

    // Thermal properties (nondimensional)
    double alpha_l;        // Liquid thermal diffusivity
    double alpha_g;        // Gas thermal diffusivity
    double cp_l;           // Liquid specific heat
    double cp_g;           // Gas specific heat

    // Geometry (nondimensional)
    double diameter;       // Drop diameter (= 1.0)
    double radius;         // Drop radius (= 0.5)
    double domainsize;     // Domain size (= 8.0)
    double drop_center_x;  // Drop center x-coordinate (= 1.5)
    double temp_radius;    // Temperature smoothing radius (= 0.625)
    double refinegap;      // Refinement gap

    // Velocity (nondimensional)
    double vel;            // Free-stream velocity (= 1.0)

    // Time control
    double timeend;        // End time
    double timestep;       // Output timestep

    // Derived time scales
    double t_star_factor;  // Conversion factor: t* = t⁺ * sqrt(ρ_l/ρ_g)
};

/**
 * ===========================================================================
 * FUNCTION DECLARATIONS
 * ===========================================================================
 */
void readfromarg(char **argv, int argc, struct CFDValues *bvalues);
int timecalculation(double t, char *chartime);

/**
 * ===========================================================================
 * MAIN INITIALIZATION FUNCTION
 * ===========================================================================
 * Sets up all nondimensional parameters
 */
int numericalmainvalues(char **argv, int argc, struct CFDValues *bvalues)
{
    // Set reference scales (nondimensional)
    bvalues->diameter = D_CHAR;
    bvalues->radius = DROP_RADIUS;
    bvalues->vel = U_INF_STAR;
    bvalues->rhoL = RHO_L_STAR;

    // Initialize dimensionless groups
    bvalues->Reynolds = REYNOLDS;
    bvalues->Weber = WEBER;
    bvalues->Stefan = STEFAN;
    bvalues->Prandtl = PRANDTL;

    // Initialize material ratios
    bvalues->rho_ratio = RHO_RATIO;
    bvalues->mu_ratio = MU_RATIO;
    bvalues->k_ratio = K_RATIO;
    bvalues->cp_ratio = CP_RATIO;

    // Default time values
    bvalues->timeend = -1.0;
    bvalues->timestep = -1.0;

    // Read command-line arguments
    readfromarg(argv, argc, bvalues);

    // Compute derived nondimensional properties
    // =========================================

    // Densities (normalized by ρ_l)
    bvalues->rhoG = bvalues->rho_ratio * bvalues->rhoL;

    // Viscosities from Reynolds number
    // Re = ρ_g U∞ D₀ / μ_g
    // μ_g = ρ_g U∞ D₀ / Re = rho_ratio / Re (since ρ_l=1, U∞=1, D₀=1)
    bvalues->muG = bvalues->rho_ratio / bvalues->Reynolds;
    bvalues->muL = bvalues->muG / bvalues->mu_ratio;

    // Surface tension from Weber number
    // We = ρ_g U∞² D₀ / σ
    // σ = ρ_g U∞² D₀ / We = rho_ratio / We
    bvalues->Sigma = bvalues->rho_ratio / bvalues->Weber;

    // Thermal diffusivities
    // α = k / (ρ cp)
    // For nondimensional form: α* = α / (U∞ D₀)
    // From Pr = μ cp / k and Re = ρ U D / μ:
    // α = k/(ρ cp) = μ/(ρ Pr) = U D/(Re Pr)
    bvalues->alpha_g = bvalues->vel * bvalues->diameter /
                       (bvalues->Reynolds * bvalues->Prandtl);
    bvalues->alpha_l = bvalues->alpha_g * bvalues->k_ratio /
                       (bvalues->rho_ratio * bvalues->cp_ratio);

    // Specific heats (normalized by c_p,g)
    bvalues->cp_g = 1.0;
    bvalues->cp_l = 1.0 / bvalues->cp_ratio;

    // Geometry
    bvalues->domainsize = DOMAIN_WIDTH * bvalues->diameter;
    bvalues->drop_center_x = DROP_CENTER_X * bvalues->diameter;
    bvalues->temp_radius = TEMP_SMOOTH_RADIUS * bvalues->diameter;
    bvalues->refinegap = REFINE_GAP * bvalues->diameter;

    // Time parameters
    if (bvalues->timeend < 0.0)
        bvalues->timeend = MAX_TIME;
    if (bvalues->timestep < 0.0)
        bvalues->timestep = SAVE_FILE_EVERY;

    // Time scale conversion factor
    // t* = t⁺ * sqrt(ρ_l/ρ_g) where t⁺ = t U∞/D₀
    bvalues->t_star_factor = sqrt(bvalues->rhoL / bvalues->rhoG);

    // Print summary
    switch (pid())
    {
    case 0:
    {
        printf("========================================\n");
        printf("NONDIMENSIONAL DROP VAPORIZATION\n");
        printf("========================================\n");
        printf("\nNondimensional Groups:\n");
        printf("  Re = %.2f\n", bvalues->Reynolds);
        printf("  We = %.2f\n", bvalues->Weber);
        printf("  St = %.2f\n", bvalues->Stefan);
        printf("  Pr = %.2f\n", bvalues->Prandtl);
        printf("\nMaterial Ratios:\n");
        printf("  ρ_g/ρ_l = %.6f\n", bvalues->rho_ratio);
        printf("  μ_g/μ_l = %.6f\n", bvalues->mu_ratio);
        printf("  k_g/k_l = %.6f\n", bvalues->k_ratio);
        printf("  c_p,g/c_p,l = %.6f\n", bvalues->cp_ratio);
        printf("\nDerived Properties (Nondimensional):\n");
        printf("  ρ_l* = %.6e\n", bvalues->rhoL);
        printf("  ρ_g* = %.6e\n", bvalues->rhoG);
        printf("  μ_l* = %.6e\n", bvalues->muL);
        printf("  μ_g* = %.6e\n", bvalues->muG);
        printf("  σ* = %.6e\n", bvalues->Sigma);
        printf("  α_l* = %.6e\n", bvalues->alpha_l);
        printf("  α_g* = %.6e\n", bvalues->alpha_g);
        printf("\nGeometry:\n");
        printf("  Domain: %.1f × %.1f\n", bvalues->domainsize, bvalues->domainsize);
        printf("  Drop radius: %.2f\n", bvalues->radius);
        printf("  Drop center: (%.2f, 0)\n", bvalues->drop_center_x);
        printf("\nTime:\n");
        printf("  End time (t⁺): %.2f\n", bvalues->timeend);
        printf("  Conversion: t* = %.2f × t⁺\n", bvalues->t_star_factor);
        printf("========================================\n\n");

        // Write to parameters file
        FILE *fp;
        fp = fopen(FILENAME_PARAMETERS, "w");
        fprintf(fp, "Nondimensional Axisymmetric Drop Vaporization\n");
        fprintf(fp, "==============================================\n\n");
        fprintf(fp, "Nondimensional Groups:\n");
        fprintf(fp, "  Reynolds number (Re): %.2f\n", bvalues->Reynolds);
        fprintf(fp, "  Weber number (We): %.2f\n", bvalues->Weber);
        fprintf(fp, "  Stefan number (St): %.2f\n", bvalues->Stefan);
        fprintf(fp, "  Prandtl number (Pr): %.2f\n", bvalues->Prandtl);
        fprintf(fp, "\nMaterial Ratios:\n");
        fprintf(fp, "  Density ratio (ρ_g/ρ_l): %.6f\n", bvalues->rho_ratio);
        fprintf(fp, "  Viscosity ratio (μ_g/μ_l): %.6f\n", bvalues->mu_ratio);
        fprintf(fp, "  Thermal conductivity ratio (k_g/k_l): %.6f\n", bvalues->k_ratio);
        fprintf(fp, "  Heat capacity ratio (c_p,g/c_p,l): %.6f\n", bvalues->cp_ratio);
        fprintf(fp, "\nDerived Nondimensional Properties:\n");
        fprintf(fp, "  Liquid density (ρ_l*): %.6e\n", bvalues->rhoL);
        fprintf(fp, "  Gas density (ρ_g*): %.6e\n", bvalues->rhoG);
        fprintf(fp, "  Liquid viscosity (μ_l*): %.6e\n", bvalues->muL);
        fprintf(fp, "  Gas viscosity (μ_g*): %.6e\n", bvalues->muG);
        fprintf(fp, "  Surface tension (σ*): %.6e\n", bvalues->Sigma);
        fprintf(fp, "  Liquid thermal diffusivity (α_l*): %.6e\n", bvalues->alpha_l);
        fprintf(fp, "  Gas thermal diffusivity (α_g*): %.6e\n", bvalues->alpha_g);
        fclose(fp);
        break;
    }
    }
    return 1;
}

/**
 * ===========================================================================
 * READ COMMAND-LINE ARGUMENTS
 * ===========================================================================
 */
void readfromarg(char **argv, int argc, struct CFDValues *bvalues)
{
    int i, j;
    char tmp[100];
    if (argc < 2)
        return;
    for (i = 1; i < argc; i++)
    {
        switch(argv[i][0])
        {
        case 'r':
        case 'R':
        {
            for(j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];
            tmp[j - 1] = '\0';
            bvalues->Reynolds = atof(tmp);
            break;
        }
        case 'w':
        case 'W':
        {
            for(j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];
            tmp[j - 1] = '\0';
            bvalues->Weber = atof(tmp);
            break;
        }
        case 's':
        case 'S':
        {
            for(j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];
            tmp[j - 1] = '\0';
            bvalues->Stefan = atof(tmp);
            break;
        }
        case 'p':
        case 'P':
        {
            for(j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];
            tmp[j - 1] = '\0';
            bvalues->Prandtl = atof(tmp);
            break;
        }
        case 'x':
        case 'X':
        {
            for(j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];
            tmp[j - 1] = '\0';
            LEVELmax = atoi(tmp);
            break;
        }
        case 'n':
        case 'N':
        {
            for(j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];
            tmp[j - 1] = '\0';
            LEVELmin = atoi(tmp);
            break;
        }
        case 't':
        case 'T':
        {
            switch(argv[i][1])
            {
            case 'e':
            case 'E':
            {
                for(j = 2; j < (int)strlen(argv[i]); j++)
                    tmp[j - 2] = argv[i][j];
                tmp[j - 2] = '\0';
                bvalues->timeend = atof(tmp);
                break;
            }
            case 's':
            case 'S':
            {
                for(j = 2; j < (int)strlen(argv[i]); j++)
                    tmp[j - 2] = argv[i][j];
                tmp[j - 2] = '\0';
                bvalues->timestep = atof(tmp);
                break;
            }
            }
            break;
        }
        }
    }
}

/**
 * ===========================================================================
 * TIME CALCULATION UTILITY
 * ===========================================================================
 * Converts seconds to days:hours:minutes:seconds format
 */
int timecalculation(double t, char *chartime)
{
    int d, h, m, s;
    if(t < 60.0)
    {
        d = 0;
        h = 0;
        m = 0;
        s = (int) t;
    }
    else if(t < 3600.0)
    {
        d = 0;
        h = 0;
        m = (int) (t / 60.0);
        s = (int) (t - m*60.0);
    }
    else if(t < 3600.0*24.0)
    {
        d = 0;
        h = (int) (t / 3600.0);
        m = (int) ((t - h*3600.0) / 60.0);
        s = (int) (t - h*3600.0 - m*60.0);
    }
    else
    {
        d = (int) (t / 3600.0 / 24.0);
        h = (int) ((t - d*3600.0*24.0) / 3600.0);
        m = (int) ((t - d*3600.0*24.0 - h*3600.0) / 60.0);
        s = (int) (t - d*3600.0*24.0 - h*3600.0 - m*60.0);
    }
    sprintf(chartime, "%d:%02d:%02d:%02d", d, h, m, s);
    return 1;
}
