
// Author: Vinod Thale  15 sep 2023
// Modified for Sharp VOF method with contact line dynamics
// Basic framework  is developed by Hossain Chizari
// Later functionalities added by Vinod Ashok Thale and Professor Marie-Jean THORAVAL

// IMPORTANT: Include sharp VOF headers in correct order
#include "../2D-sharp-and-conservative-VOF-method-Basiliks-main/axi.h"  // Custom axisymmetric with embed support
#include "navier-stokes/centered.h"    // solve NS equations
#define FILTERED                       // Smear density and viscosity jumps

// Sharp VOF method headers (MUST be in this order)
#include "../2D-sharp-and-conservative-VOF-method-Basiliks-main/myembed.h"              // Custom embed boundaries
#include "../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_contact.h"       // Contact line dynamics
#include "../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_two-phase.h"     // Two-phase flow
#include "../2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_tension.h"       // Surface tension

#include "tag.h"                       // help to count small droplets we can count its geometric mass, volume, area ...... etc
#include "curvature.h"


#define DIM_NONDIM_EXP			'd' // d: dimension; n: nondimension; e: experimentalization

#if DIM_NONDIM_EXP == 'd' || DIM_NONDIM_EXP == 'D'

#define VELOCITY			5.00                    // Velocity of Water   m/s  Si unit  for
#define DROP_DIAMETER		        2.050e-03               // Diameter of Water  drop  meter Si unit
#define RHO_L				998.0                   // Density of Water drop     25 degree kg/m^3  Si unit
#define RHO_G				1.21                    // Density of air at 25 degree kg/m^3  Si unit
#define MU_L				0.001                   // Dynamic Viscosity of Water    at 25 degree Pa s in Si unit
#define MU_G				1.81e-5                 // Dynamic Viscosity of air at 25 degree
#define SIGMA				0.073                   // Surface tension of Water  drop  at 25 degree   N/m  Si unit
#define GRAVITY				9.81
//
#define RHO_GL				0.0
#define MU_GL				0.0
#define REYNOLDS			0.0
#define WEBER				0.0
#define FROUDE                          0.0

#elif DIM_NONDIM_EXP == 'n' || DIM_NONDIM_EXP == 'N'

#define WEBER				100.0
#define REYNOLDS			100.0
#define FROUDE                          70.0
#define RHO_GL				(0.0012) // air-water at 25C: 0.001187503
#define MU_GL				(0.0210) // air-water at 25C: 0.020898876
//
#define VELOCITY			0.0
#define DROP_DIAMETER		        0.0
#define RHO_L				0.0
#define MU_L				0.0
#define SIGMA				0.0
#define RHO_G				0.0
#define MU_G				0.0
#define GRAVITY				0.0

#elif DIM_NONDIM_EXP == 'e' || DIM_NONDIM_EXP == 'E'

#define WEBER				300.0
#define REYNOLDS			1000.0
#define FROUDE                          70.0
#define DROP_DIAMETER		        2.0e-3
#define SIGMA				17.6e-3
#define RHO_L				816.0
#define RHO_G				1.2041
#define MU_G				1.94e-5
//
#define VELOCITY			0.0
#define RHO_GL				0.0
#define MU_GL				0.0
#define MU_L				0.0
#define GRAVITY				0.0

#endif

#define INITAL_GRID_LEVEL		9
#define MAX_GRID_LEVEL			12
#define DOMAIN_WIDTH			4.00
#define POOL_DEPTH		        0.00
#define INITIAL_DISTANCE		0.04
#define BUBBLE_DIAMETER		        0.80
#define DBDELTA       		        0.00
#define REFINE_GAP		        0.02
#define MAX_TIME			2.00
#define SAVE_FILE_EVERY			0.01

#define REFINE_VAR		        {f, u.x, u.y}
#define REFINE_VAR_TEXT			"f, u.x, u.y"
#define REFINE_VALUE_0			-6
#define REFINE_VALUE_1			-3
#define REFINE_VALUE_2			-3

// Contact angle configuration (degrees)
// 0° = perfect wetting, 90° = neutral, 180° = non-wetting
#define CONTACT_ANGLE_SUBSTRATE		90.0    // Contact angle at left boundary (substrate)

#define FILENAME_DATA			"data"
#define FILENAME_DURATION		"duration"
#define FILENAME_PARAMETERS		"parameters.txt"
#define FILENAME_ENDOFRUN		"endofrun"
#define FILENAME_LASTFILE		"lastfile"

#define R_VOFLIMIT		         1.0e-9
#define R_PI			         3.1415926535897932384626433832795

int LEVELmin = INITAL_GRID_LEVEL, LEVELmax = MAX_GRID_LEVEL ;
double maxruntime = HUGE;
scalar fdrop[], pressure[];
scalar fb[];

struct CFDValues {
	double rhoL, rhoG, muL, muG, Sigma;
	double vel, Reynolds, Weber, Froude,Bond, Oh,GXnormlised;
	double diameter, domainsize, refinegap, pooldepth, initialdis;
	double timecontact, timeend, timestep;
	double bubblediameter;
	double dbdelta;
};

void readfromarg(char **argv, int argc, struct CFDValues *bvalues);

int numericalmainvalues(char **argv, int argc, struct CFDValues *bvalues)
{
	double velocity = VELOCITY, mu_l = MU_L;
	bvalues->rhoL = 1.0;
	bvalues->vel = 1.0;
	bvalues->diameter = 1.0;
	;
	bvalues->Reynolds = -1.0;
	bvalues->Weber = -1.0;
	bvalues->pooldepth = -1.0;
	bvalues->timeend = -1.0;
	bvalues->timestep = -1.0;
	;
	readfromarg(argv, argc, bvalues);
	switch(DIM_NONDIM_EXP)
	{
	case 'd':
	case 'D':
	{
		bvalues->Reynolds = (RHO_L * VELOCITY * DROP_DIAMETER / mu_l);
		bvalues->Weber = (RHO_L * VELOCITY * VELOCITY * DROP_DIAMETER / SIGMA);
		bvalues->Froude = (VELOCITY / sqrt (GRAVITY * DROP_DIAMETER));
		;
		bvalues->Bond = (RHO_L * GRAVITY * DROP_DIAMETER  * DROP_DIAMETER / SIGMA);
		bvalues->Oh = (mu_l/ sqrt(RHO_L * SIGMA * DROP_DIAMETER));
		;
		bvalues->muL = (bvalues->rhoL * bvalues->vel * bvalues->diameter / bvalues->Reynolds);
		bvalues->Sigma = (bvalues->rhoL * bvalues->vel * bvalues->vel * bvalues->diameter / bvalues->Weber);
		bvalues->rhoG = (RHO_G / RHO_L) * bvalues->rhoL;
		bvalues->muG = (MU_G / mu_l)  * bvalues->muL;
		bvalues->GXnormlised = (GRAVITY * DROP_DIAMETER / (VELOCITY*VELOCITY));
		;
	    bvalues->bubblediameter = BUBBLE_DIAMETER * bvalues->diameter;
	    bvalues->dbdelta = DBDELTA * bvalues->diameter;
		break;
	}
	}
	bvalues->domainsize = DOMAIN_WIDTH * bvalues->diameter;
	if (bvalues->pooldepth < 0.0)
		bvalues->pooldepth = POOL_DEPTH * bvalues->diameter;
	bvalues->initialdis = INITIAL_DISTANCE * bvalues->diameter;
	bvalues->refinegap = REFINE_GAP * bvalues->diameter;
	;
	bvalues->timecontact = bvalues->initialdis / bvalues->vel;
	if (bvalues->timeend < 0.0)
		bvalues->timeend = MAX_TIME;
	if (bvalues->timestep < 0.0)
		bvalues->timestep = SAVE_FILE_EVERY;
	;
	switch (pid())
	{
	case 0:
	{
	    printf("R: %f --- W: %f --- H: %f\r\n", bvalues->Reynolds, bvalues->Weber, bvalues->pooldepth);
	    FILE *fp;
	    fp = fopen (FILENAME_PARAMETERS, "w");
	    fprintf (fp, "Name of Liquid : Water drop  \r\n");
	    fprintf (fp, "Experimental parameters  / Numerical simulation \r\n");
	    fprintf (fp, "Diameter_Experimental: %.3e / Normalized diameter  of drop (D): %.3e\r\n", DROP_DIAMETER, bvalues->diameter);
	    fprintf (fp, "Velocity_Experimental: %.3e / Normalized Velocity  of liquid (V) : %.3e\r\n", velocity, bvalues->vel);
	    fprintf (fp, "Rho(L)_Experimental: %.3e /  Normalized density of liquid (rho1): %.3e\r\n", RHO_L, bvalues->rhoL);
	    fprintf (fp, "Contact_Angle_Substrate: %.1f degrees\r\n", CONTACT_ANGLE_SUBSTRATE);
	    fprintf (fp, "Sharp_VOF_Method: Enabled\r\n");
	    fclose (fp);
	    break;
	}
	}
	return 1;
}

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
		case 'F':
		{
			for(j = 1; j < (int)strlen(argv[i]); j++)
				tmp[j - 1] = argv[i][j];
			tmp[j - 1] = '\0';
			bvalues->Froude = atof(tmp);
			break;
		}
		case 'h':
		case 'H':
		{
			for(j = 1; j < (int)strlen(argv[i]); j++)
				tmp[j - 1] = argv[i][j];
			tmp[j - 1] = '\0';
			bvalues->pooldepth = atof(tmp);
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
