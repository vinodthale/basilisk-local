#include "axi.h"                       // Header for axisymmetric geometry, handles cylindrical or radial symmetry
#include "navier-stokes/centered.h"    // Header to solve Navier-Stokes equations using a centered finite difference method
#define FILTERED          1            // Define FILTERED as 1 to enable filtering of density and viscosity jumps between phases
#include "two-phase.h"                 // Header for two-phase flow simulation, modeling interactions between two distinct fluids
#include "navier-stokes/conserving.h"  // http://basilisk.fr/sandbox/kaitaotang/dw14l11.c
#include "tension.h"                   // Header to include surface tension effects between the two phases
#include "tag.h"                       // Header for tracking droplet properties and tagging specific regions or particles
#include "curvature.h"                 // Header to compute curvature of the interface between the two phases, important for surface tension forces

// --- Define the type of simulation setup: dimensional, nondimensional, or experimentalized ---
#define DIM_NONDIM_EXP			'd'  // 'd' = Dimensional; 'n' = Nondimensional; 'e' = Experimentalized

// --- Dimensional Setup: Real-world physical quantities ---
#if DIM_NONDIM_EXP == 'd' || DIM_NONDIM_EXP == 'D'

#define VELOCITY            17.881         // m/s - Gas stream velocity
#define DROP_DIAMETER       0.0019       // m   - Water droplet diameter
#define RHO_L               1000.0      // kg/m^3 - Density of water
#define RHO_G               1.2         // kg/m^3 - Density of air
#define MU_L                0.001       // Pa·s - Dynamic viscosity of water
#define MU_G                0.000018    // Pa·s - Dynamic viscosity of air
#define SIGMA               0.0729      // N/m - Surface tension between water and air
#define GRAVITY             9.81        // m/s^2 - Gravitational acceleration

// --- Unused or unnecessary parameters in dimensional setup ---
#define RHO_GL				0.0          // Placeholder value, unused in dimensional setup
#define MU_GL				0.0          // Placeholder value, unused in dimensional setup
#define REYNOLDS			0.0          // Placeholder for Reynolds number, unused
#define WEBER				0.0          // Placeholder for Weber number, unused
#define FROUDE              0.0          // Placeholder for Froude number, unused

// --- Nondimensional Setup: Non-dimensionalized numbers (Reynolds, Weber, Froude) ---
#elif DIM_NONDIM_EXP == 'n' || DIM_NONDIM_EXP == 'N'

#define WEBER				100.0        // Nondimensional Weber number
#define REYNOLDS			100.0        // Nondimensional Reynolds number
#define FROUDE              70.0         // Nondimensional Froude number
#define RHO_GL				0.0012       // Nondimensionalized air-water density
#define MU_GL				0.0210       // Nondimensionalized air-water viscosity

// --- Unused parameters in nondimensional setup ---
#define VELOCITY			0.0          // No dimensional velocity in nondimensional setup
#define DROP_DIAMETER		0.0          // No dimensional droplet size in nondimensional setup
#define RHO_L				0.0          // No dimensional fluid density
#define MU_L				0.0          // No dimensional viscosity
#define SIGMA				0.0          // No dimensional surface tension
#define RHO_G				0.0          // No dimensional air density
#define MU_G				0.0          // No dimensional air viscosity
#define GRAVITY				0.0          // No gravitational acceleration

// --- Experimentalized Setup: Experimental data values ---
#elif DIM_NONDIM_EXP == 'e' || DIM_NONDIM_EXP == 'E'

#define WEBER				300.0        // Experimental Weber number
#define REYNOLDS			1000.0       // Experimental Reynolds number
#define FROUDE              70.0         // Experimental Froude number
#define DROP_DIAMETER		2.0e-3       // Experimental droplet diameter in meters
#define SIGMA				17.6e-3      // Experimental surface tension (water-air)
#define RHO_L				816.0        // Experimental water density
#define RHO_G				1.2041       // Experimental air density
#define MU_G				1.94e-5      // Experimental air viscosity

// --- Unused parameters in experimentalized setup ---
#define VELOCITY			0.0          // No dimensional velocity in experimentalized setup
#define RHO_GL				0.0          // No placeholder for gas-liquid density in experimental setup
#define MU_GL				0.0          // No placeholder for gas-liquid viscosity in experimental setup
#define MU_L				0.0          // No placeholder for liquid viscosity in experimental setup
#define GRAVITY				0.0          // No placeholder for gravity in experimental setup

#endif

// --- Grid Setup Parameters (using Basliks Units) ---
#define INITAL_GRID_LEVEL		10         // Initial grid refinement level (Basliks Units)
#define MAX_GRID_LEVEL			13        // Maximum allowed grid refinement level (Basliks Units)
//#define MAX_GRID_LEVEL 12  // for ~256 cells across the initial drop diameter
//#define MAX_GRID_LEVEL 13  // for ~512 cells across the initial drop diameter
//#define MAX_GRID_LEVEL 14  // for ~1024 cells across the initial drop diameter
//#define MAX_GRID_LEVEL 15  // for ~2048 cells across the initial drop diameter
#define DOMAIN_WIDTH			16.0      // Width of the simulation domain (Basliks Units)
#define POOL_DEPTH				0.00      // Depth of the fluid pool (Basliks Units)
#define INITIAL_DISTANCE		5.00      // Initial distance from left Boundary up to drop outside radius (Basliks Units)
#define ASPECT_RATIO            1.25      // AR = Dh / Dv
#define BUBBLE_DIAMETER		    0.00      // Initial bubble diameter (Basliks Units)
#define DBDELTA       		    0.00      // Additional parameter for droplet-bubble interaction (Basliks Units)
#define REFINE_GAP				0.02      // Gap for grid refinement (Basliks Units)
#define MAX_TIME				250.0     // Maximum simulation time (Basliks Units)
#define SAVE_FILE_EVERY			1.00      // Time interval (Basliks Units) to save simulation data

// --- Refinement Settings (using Basliks Units) ---
#define REFINE_VAR				{f, u.x, u.y} // Variables used for refinement (e.g., phase field and velocity components)
#define REFINE_VAR_TEXT			"f, u.x, u.y" // Textual representation of variables for clarity
#define REFINE_VALUE_0			-6            // Refinement level for variable 0 (Basliks Units)
#define REFINE_VALUE_1			-5            // Refinement level for variable 1 (Basliks Units)
#define REFINE_VALUE_2			-5            // Refinement level for variable 2 (Basliks Units)

// --- Droplet and Bubble Removal Settings (using Basliks Units) ---
#define REMOVE_DROP_YESNO		'n'          // Flag for removing droplets (set to 'y' or 'n')
#define REMOVE_DROP_SIZE		4.0          // Minimum droplet size for removal (Basliks Units)
#define REMOVE_DROP_PERIOD		4            // Period of droplet removal (Basliks Units)
#define REMOVE_BUBBLE_YESNO		'n'          // Flag for removing bubbles (set to 'y' or 'n')
#define REMOVE_BUBBLE_SIZE		4.0          // Minimum bubble size for removal (Basliks Units)
#define REMOVE_BUBBLE_PERIOD	4            // Period of bubble removal (Basliks Units)


// --- File Output Settings ---
#define FILENAME_DATA			"data"           // Filename for saving simulation data
#define FILENAME_DURATION		"duration"       // Filename for saving simulation duration data
#define FILENAME_PARAMETERS		"parameters.txt" // Filename for saving simulation parameters
#define FILENAME_ENDOFRUN		"endofrun"       // Filename for saving end-of-run data
#define FILENAME_LASTFILE		"lastfile"       // Filename for saving the last simulation state

// --- Constants ---
#define R_VOFLIMIT				1.0e-9                            // Minimum volume fraction limit (for numerical stability)
#define R_PI					3.1415926535897932384626433832795 // Pi constant (for geometry calculations)

// --- Grid Setup and Simulation Parameters ---
int LEVELmin = INITAL_GRID_LEVEL, LEVELmax = MAX_GRID_LEVEL;
double maxruntime = HUGE;    
scalar fdrop[];              // Scalar field for tracking the drop (VOF tracer)
scalar fb[];                 // Optional scalar for another tracer (e.g., air bubble indicator)
scalar pressure[];           // Scalar to store the hydrodynamic (gauge) pressure field at each point
scalar pressureGas[];        // Stores pressure in gas regions (where f ~ 0)
scalar pressureLiquid[];     // Stores pressure in liquid regions (where f ~ 1)
// Structure to hold CFD simulation values
struct CFDValues {
    // --- Physical properties ---
    double rhoL;        // Density of the liquid phase
    double rhoG;        // Density of the gas phase
    double muL;         // Dynamic viscosity of the liquid
    double muG;         // Dynamic viscosity of the gas
    double Sigma;       // Surface tension coefficient
    // --- Non-dimensional quantities ---
    double velocity;    // Characteristic velocity scale (U₀)
    double diameter;    // Characteristic length scale (e.g., drop diameter, d₀)
    double Reynolds;    // Reynolds number
    double Weber;       // Weber number
    double Oh;          // Ohnesorge number
    // --- Simulation time parameters ---
    double timeend;     // End time of simulation
    double timestep;    // Time interval for data output
    double pooldepth;   // Depth of liquid pool (non-dimensionalized)
    // --- Derived or optional values ---
    double vel;             // Normalized velocity (often set to 1)
    double GXnormlised;     // Normalized gravity in x-direction
    double Froude;          // Froude number
    double Bond;            // Bond number
    double domainsize;      // Domain width (based on drop diameter)
    double refinegap;       // Refinement distance around the interface
    double initialdis;      // Initial distance between drop and surface
    double timecontact;     // Estimated time of first contact with the pool
    double bubblediameter;  // Diameter of the preexisting bubble (if any)
    double dbdelta;         // Distance between drop and bubble center
    // --- Geometric variation ---
    double Dh;               // Horizontal diameter of ellipsoidal drop
    double Dv;               // Vertical diameter of the drop (Dv = Dh * Dh / 1 = Dh²)
};


void readfromarg(char **argv, int argc, struct CFDValues *bvalues);

int numericalmainvalues(char **argv, int argc, struct CFDValues *bvalues) {
    // Default initializations
    double velocity = VELOCITY, mu_l = MU_L;
    bvalues->rhoG = 1.0;  // Gas density (rhoG)
    bvalues->vel = 1.0;  // Characteristic velocity scale (U_0)
    bvalues->diameter = 1.0;  // Characteristic length scale (diameter d_0)
    
    bvalues->Reynolds = -1.0;  // Reynolds number (dimensionless)
    bvalues->Weber = -1.0;  // Weber number (dimensionless)
    bvalues->pooldepth = -1.0;  // Pool depth
    bvalues->timeend = -1.0;  // End time for simulation
    bvalues->timestep = -1.0;  // Time step for simulation

    // Read values from command-line arguments
    readfromarg(argv, argc, bvalues);

    // Handle different dimension/nondimensionalization cases
    switch(DIM_NONDIM_EXP) {
        case 'd':
        case 'D': {
            // Dimensional case - based on experimental values
            bvalues->Reynolds = (RHO_G * VELOCITY * DROP_DIAMETER / MU_G);  // Reynolds number = (rhoG * velocity * diameter) / muG
            bvalues->Weber = (RHO_G * VELOCITY * VELOCITY * DROP_DIAMETER / SIGMA);  // Weber number = (rhoG * velocity^2 * diameter) / sigma
            bvalues->Oh = (mu_l / sqrt(RHO_L * SIGMA * DROP_DIAMETER));  // Ohnesorge number = muL / sqrt(rhoL * sigma * diameter)
            // Calculate physical properties based on these numbers
            bvalues->muG = (bvalues->rhoG * bvalues->vel * bvalues->diameter / bvalues->Reynolds);  // muG = (rhoG * velocity * diameter) / Reynolds
            bvalues->Sigma = (bvalues->rhoG * bvalues->vel * bvalues->vel * bvalues->diameter / bvalues->Weber);  // Sigma = (rhoG * velocity^2 * diameter) / Weber
            bvalues->rhoL = (RHO_L / RHO_G) * bvalues->rhoG;  // rhoL = (RHO_L / RHO_G) * rhoG
            bvalues->muL = (MU_L / MU_G ) * bvalues->muG;  // muL = (MU_L / MU_G) * muG
            // Bubble properties
            bvalues->bubblediameter = BUBBLE_DIAMETER * bvalues->diameter;  // Bubble diameter = BUBBLE_DIAMETER * diameter
            bvalues->dbdelta = DBDELTA * bvalues->diameter;  // Distance between drop and bubble (dbdelta) = DBDELTA * diameter  
            // Compute Dh and Dv from aspect ratio
            bvalues->Dh = bvalues->diameter * pow(ASPECT_RATIO, 1.0 / 3.0);    
            bvalues->Dv = bvalues->diameter * pow(ASPECT_RATIO, -2.0 / 3.0); 
            break;
        }
        
        case 'n':
        case 'N': {
            // Non-dimensional case - using predefined values
            if (bvalues->Reynolds < 0.0)
                bvalues->Reynolds = REYNOLDS;
            if (bvalues->Weber < 0.0)
                bvalues->Weber = WEBER;
            if (bvalues->Froude < 0.0)
                bvalues->Froude = FROUDE;
            // Calculate physical properties based on non-dimensionalization
            bvalues->muL = (bvalues->rhoL * bvalues->vel * bvalues->diameter / bvalues->Reynolds);  // muL = (rhoL * velocity * diameter) / Reynolds
            bvalues->Sigma = (bvalues->rhoL * bvalues->vel * bvalues->vel * bvalues->diameter / bvalues->Weber);  // Sigma = (rhoL * velocity^2 * diameter) / Weber
            bvalues->rhoG = RHO_GL * bvalues->rhoL;  // rhoG = RHO_GL * rhoL
            bvalues->muG = MU_GL * bvalues->muL;  // muG = MU_GL * muL
            break;
        }
        
        case 'e':
        case 'E': {
            // Experimental case - using experimental values
            if (bvalues->Reynolds < 0.0)
                bvalues->Reynolds = REYNOLDS;
            if (bvalues->Weber < 0.0)
                bvalues->Weber = WEBER;
            // Calculate physical properties based on experimental values
            bvalues->muL = (bvalues->rhoL * bvalues->vel * bvalues->diameter / bvalues->Reynolds);  // muL = (rhoL * velocity * diameter) / Reynolds
            bvalues->Sigma = (bvalues->rhoL * bvalues->vel * bvalues->vel * bvalues->diameter / bvalues->Weber);  // Sigma = (rhoL * velocity^2 * diameter) / Weber
            // Adjust velocity and viscosity based on experimental data
            velocity = sqrt(bvalues->Weber * SIGMA / (DROP_DIAMETER * RHO_L));  // velocity = sqrt(Weber * sigma / (diameter * rhoL))
            mu_l = (RHO_L * velocity * DROP_DIAMETER / bvalues->Reynolds);  // muL = (rhoL * velocity * diameter) / Reynolds
            bvalues->muG = (MU_G / mu_l) * bvalues->muL;  // muG = (MU_G / muL) * muL
            bvalues->rhoG = (RHO_G / RHO_L) * bvalues->rhoL;  // rhoG = (RHO_G / RHO_L) * rhoL
            break;
        }
    }

    // Calculate derived simulation parameters
    bvalues->domainsize = DOMAIN_WIDTH * bvalues->diameter;  // domain size = DOMAIN_WIDTH * diameter
    if (bvalues->pooldepth < 0.0)
        bvalues->pooldepth = POOL_DEPTH * bvalues->diameter;  // pool depth = POOL_DEPTH * diameter
    bvalues->initialdis = INITIAL_DISTANCE * bvalues->diameter;  // initial distance = INITIAL_DISTANCE * diameter
    bvalues->refinegap = REFINE_GAP * bvalues->diameter;  // refine gap = REFINE_GAP * diameter
    bvalues->timecontact = bvalues->initialdis / bvalues->vel;  // contact time = initial distance / velocity
    if (bvalues->timeend < 0.0)
        bvalues->timeend = MAX_TIME;  // end time = MAX_TIME if not defined
    if (bvalues->timestep < 0.0)
        bvalues->timestep = SAVE_FILE_EVERY;  // time step = SAVE_FILE_EVERY if not defined

    // Output parameters for debugging/verification (for process ID 0)
    switch (pid()) {
        case 0: {
            printf("R: %f --- W: %f --- H: %f\r\n", bvalues->Reynolds, bvalues->Weber, bvalues->pooldepth);
            // Compute number of cells per drop diameter at max refinement
            int cells_per_drop_diameter = (int)(pow(2, MAX_GRID_LEVEL) * (bvalues->diameter / bvalues->domainsize));
            FILE *fp = fopen(FILENAME_PARAMETERS, "w");
            fprintf(fp, "Name of Liquid : Water drop  \r\n");
            fprintf(fp, "Experimental parameters / Numerical simulation parameters in Basilisk unit\r\n");
            fprintf(fp, "Diameter_Experimental: %.3e / Normalized diameter of drop (D): %.3e\r\n", DROP_DIAMETER, bvalues->diameter);
            fprintf(fp, "Velocity_Experimental: %.3e / Normalized Velocity of liquid (V) : %.3e\r\n", velocity, bvalues->vel);
            fprintf(fp, "Rho(L)_Experimental: %.3e / Normalized density of liquid (rho1): %.3e\r\n", RHO_L, bvalues->rhoL);
            fprintf(fp, "Rho(G)_Experimental: %.3e / Normalized density of Air (rho2) : %.3e\r\n", RHO_G, bvalues->rhoG);
            fprintf(fp, "Mu(L)_Experimental: %.3e / Normalized viscosity of liquid (mu1): %.3e\r\n", mu_l, bvalues->muL);
            fprintf(fp, "Mu(G)_Experimental: %.3e / Normalized viscosity of Air (mu2): %.3e\r\n", MU_G, bvalues->muG);
            fprintf(fp, "Sigma(L-G)_Experimental: %.3e / Normalized surface tension (f.sigma): %.3e\r\n", SIGMA, bvalues->Sigma);
            fprintf(fp, "Maximum Simulation Time (Basliks Units): %.2f\r\n", MAX_TIME);
            fprintf(fp, "Data Save Interval (Basliks Units): %.2f\r\n", SAVE_FILE_EVERY);
            fprintf(fp, "\r\n");
            fprintf(fp, "Reynolds: %.10f\r\n", bvalues->Reynolds);
            fprintf(fp, "Weber: %.10f\r\n", bvalues->Weber);
            fprintf(fp, "Ohsorge: %.10f\r\n", bvalues->Oh);
            fprintf(fp, "\r\n");
            fprintf(fp, "Grid Parameters:\r\n");
            fprintf(fp, "Level Max: %d\r\n", LEVELmax);
            fprintf(fp, "Level Min: %d\r\n", LEVELmin);
            fprintf(fp, "\r\n");
            fprintf(fp, "Cells per Drop Diameter (2^max_level * D / Domain Size): %d\r\n", cells_per_drop_diameter);
            fprintf(fp, "Domain Size: %.2f\r\n", bvalues->domainsize);
            fprintf(fp, "Pool Depth: %.2f\r\n", bvalues->pooldepth);
            fprintf(fp, "Initial Distance: %.2f\r\n", bvalues->initialdis);
            fprintf(fp, "Refine Gap: %.2f\r\n", bvalues->refinegap);
            fprintf(fp, "Contact Time: %.2f\r\n", bvalues->timecontact);
            fprintf(fp, "Bubble Diameter: %.2f\r\n", bvalues->bubblediameter);     
            fprintf(fp, "Dbdelta (distance between drop and bubble) : %.6f\r\n", bvalues->dbdelta); 
            fprintf(fp, "\r\n");  
            fprintf(fp, "HORIZONTAL_DIAMETER : %.3e\r\n", bvalues->Dh);
            fprintf(fp, "VERTICAL_DIAMETER   : %.3e\r\n", bvalues->Dv);
            fprintf(fp, "ASPECT_RATIO (Dh/Dv): %.3e\r\n", bvalues->Dh / bvalues->Dv);
            fprintf(fp, "Refine Variables: %s\r\n", REFINE_VAR_TEXT);
            fprintf(fp, "Refine Variables powers: %d, %d, %d\r\n", REFINE_VALUE_0, REFINE_VALUE_1, REFINE_VALUE_2);
            fprintf(fp, "\r\n");
            fprintf(fp, "Remove Drop YesNo: %c\r\n", REMOVE_DROP_YESNO);
            fprintf(fp, "Remove Drop Size: %f\r\n", REMOVE_DROP_SIZE);
            fprintf(fp, "Remove Drop Period: %d\r\n", REMOVE_DROP_PERIOD);
            fprintf(fp, "\r\n");
            fprintf(fp, "Remove Bubble YesNo: %c\r\n", REMOVE_BUBBLE_YESNO);
            fprintf(fp, "Remove Bubble Size: %f\r\n", REMOVE_BUBBLE_SIZE);
            fprintf(fp, "Remove Bubble Period: %d\r\n", REMOVE_BUBBLE_PERIOD);
            fclose(fp);
            break;
        }
    }
    return 1;
}


void readfromarg(char **argv, int argc, struct CFDValues *bvalues)
{
    int i, j;
    char tmp[100];
    
    // If there are less than 2 arguments, no values to read, so exit the function
    if (argc < 2)
        return;

    // Iterate through command-line arguments (starting from index 1)
    for (i = 1; i < argc; i++)
    {
        // Check the first character of the argument
        switch(argv[i][0])
        {
        case 'r':
        case 'R':
            // Read Reynolds number
            for (j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];  // Extract numerical part from the argument
            tmp[j - 1] = '\0';  // Null terminate the string
            bvalues->Reynolds = atof(tmp);  // Convert string to float and assign it to Reynolds number
            break;

        case 'w':
        case 'W':
            // Read Weber number
            for (j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];  // Extract numerical part
            tmp[j - 1] = '\0';  // Null terminate the string
            bvalues->Weber = atof(tmp);  // Convert string to float and assign it to Weber number
            break;

        case 'F':
            // Read Froude number
            for (j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];  // Extract numerical part
            tmp[j - 1] = '\0';  // Null terminate the string
            bvalues->Froude = atof(tmp);  // Convert string to float and assign it to Froude number
            break;

        case 'h':
        case 'H':
            // Read pool depth
            for (j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];  // Extract numerical part
            tmp[j - 1] = '\0';  // Null terminate the string
            bvalues->pooldepth = atof(tmp);  // Convert string to float and assign it to pool depth
            break;

        case 'x':
        case 'X':
            // Read LEVELmax (maximum refinement level)
            for (j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];  // Extract numerical part
            tmp[j - 1] = '\0';  // Null terminate the string
            LEVELmax = atoi(tmp);  // Convert string to integer and assign it to LEVELmax
            break;

        case 'n':
        case 'N':
            // Read LEVELmin (minimum refinement level)
            for (j = 1; j < (int)strlen(argv[i]); j++)
                tmp[j - 1] = argv[i][j];  // Extract numerical part
            tmp[j - 1] = '\0';  // Null terminate the string
            LEVELmin = atoi(tmp);  // Convert string to integer and assign it to LEVELmin
            break;

        case 't':
        case 'T':
            // Handle time-related parameters
            switch(argv[i][1])
            {
            case 'e':
            case 'E':
                // Read end time
                for (j = 2; j < (int)strlen(argv[i]); j++)
                    tmp[j - 2] = argv[i][j];  // Extract numerical part
                tmp[j - 2] = '\0';  // Null terminate the string
                bvalues->timeend = atof(tmp);  // Convert string to float and assign it to timeend
                break;

            case 's':
            case 'S':
                // Read timestep
                for (j = 2; j < (int)strlen(argv[i]); j++)
                    tmp[j - 2] = argv[i][j];  // Extract numerical part
                tmp[j - 2] = '\0';  // Null terminate the string
                bvalues->timestep = atof(tmp);  // Convert string to float and assign it to timestep
                break;
            }
            break;
        }
    }
}

int timecalculation(double t, char *chartime)
{
    int d, h, m, s;
    
    // Case 1: If time is less than 60 seconds (no need for hours, minutes, or days)
    if (t < 60.0)
    {
        d = 0;  // Days = 0
        h = 0;  // Hours = 0
        m = 0;  // Minutes = 0
        s = (int)t;  // Seconds = the whole time value
    }
    // Case 2: If time is less than 3600 seconds (1 hour)
    else if (t < 3600.0)
    {
        d = 0;  // Days = 0
        h = 0;  // Hours = 0
        m = (int)(t / 60.0);  // Minutes = total minutes in the time
        s = (int)(t - m * 60.0);  // Seconds = remaining time after extracting minutes
    }
    // Case 3: If time is less than 24 hours (3600 seconds * 24 hours)
    else if (t < 3600.0 * 24.0)
    {
        d = 0;  // Days = 0
        h = (int)(t / 3600.0);  // Hours = total hours in the time
        m = (int)((t - h * 3600.0) / 60.0);  // Minutes = remaining time after extracting hours
        s = (int)(t - h * 3600.0 - m * 60.0);  // Seconds = remaining time after extracting hours and minutes
    }
    // Case 4: If time exceeds 24 hours
    else
    {
        d = (int)(t / 3600.0 / 24.0);  // Days = total days in the time
        h = (int)((t - d * 3600.0 * 24.0) / 3600.0);  // Hours = remaining time after extracting days
        m = (int)((t - d * 3600.0 * 24.0 - h * 3600.0) / 60.0);  // Minutes = remaining time after extracting days and hours
        s = (int)(t - d * 3600.0 * 24.0 - h * 3600.0 - m * 60.0);  // Seconds = remaining time after extracting days, hours, and minutes
    }

    // Format the time as "days:hours:minutes:seconds" and store in chartime string
    sprintf(chartime, "%d:%02d:%02d:%02d", d, h, m, s);
    
    // Return success (can be modified as needed, such as returning a status code)
    return 1;
}








