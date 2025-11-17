/**
 * Mass Conservation Validation Test for Sharp VOF
 *
 * Purpose: Verify that the VOF field is strictly conserved over time
 *
 * Metrics:
 * - Volume ratio: V(t)/V(0)
 * - Absolute error: |V(t) - V(0)|
 * - Relative error: |V(t) - V(0)|/V(0)
 *
 * Pass Criteria:
 * - Sharp VOF: |ΔV/V₀| < 1e-10
 * - Standard VOF: |ΔV/V₀| < 1e-8
 */

#include "../../constants-sharp.h"

// Test parameters
#undef MAX_TIME
#undef SAVE_FILE_EVERY
#define MAX_TIME 2.0
#define SAVE_FILE_EVERY 0.01

// Storage for initial volume
double initial_volume = 0.0;
double initial_bubble_volume = 0.0;

// Statistics
double max_volume_error = 0.0;
double max_bubble_error = 0.0;
double mean_volume_error = 0.0;
int sample_count = 0;

struct CFDValues cfdbv;

// Boundary conditions
u.t[left] = dirichlet(0);
f[left] = 0.;
u.n[right] = neumann(0);
p[right] = dirichlet(0);
u.n[top] = neumann(0);
p[top] = dirichlet(0);

int main(int argc, char **argv)
{
	numericalmainvalues(argv, argc, &cfdbv);
	size(cfdbv.domainsize);
#if AXI
	;
#else
	origin(0, -cfdbv.domainsize / 2., -cfdbv.domainsize / 2.);
#endif
	int initialgrid = pow(2, LEVELmin);
	init_grid(initialgrid);

	// Create output directory
	system("mkdir -p ../results");

	rho1 = cfdbv.rhoL;
	rho2 = cfdbv.rhoG;
	mu1 = cfdbv.muL;
	mu2 = cfdbv.muG;
	f.sigma = cfdbv.Sigma;
	TOLERANCE = 1e-6;

	// Print test header
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Mass Conservation Test - Sharp VOF\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Re = %.2f, We = %.2f\n", cfdbv.Reynolds, cfdbv.Weber);
	fprintf(stderr, "Grid levels: %d - %d\n", LEVELmin, LEVELmax);
	fprintf(stderr, "Test duration: %.2f\n", MAX_TIME);
	fprintf(stderr, "========================================\n\n");

	run();
	return 0;
}

event defaults(i = 0)
{
	interfaces = list_add(NULL, f);
	interfaces = list_add(interfaces, fb);
	tmp_interfaces = list_add(NULL, tmp_c);
	interfaces_mark = list_add(NULL, mark);
	interfaces_contact_angle = list_add(NULL, contact_angle);
	tmp_c.height = f.height;
	tmp_c.nc = f.nc;
	tmp_c.oxyi = f.oxyi;
}

event initfraction (t = 0)
{
	double x0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;
	double Bubtx0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;
	fraction(f,(min(sq(0.50*cfdbv.diameter)-(sq(x - x0) + sq(y) + sq(z)),-(sq(0.50*(cfdbv.bubblediameter*cfdbv.diameter))-(sq(x - Bubtx0) + sq(y) + sq(z))))));
}

event init(i = 0)
{
	double x0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;
	double Bubtx0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;

	refine(sq(x - x0) + sq(y) + sq(z) < sq(0.50*cfdbv.diameter + cfdbv.refinegap) &&
	       sq(x - x0) + sq(y) + sq(z) > sq(0.50*cfdbv.diameter - cfdbv.refinegap) &&
	       level < LEVELmax);
	refine(sq(x - Bubtx0) + sq(y) + sq(z) < sq(0.50*(cfdbv.bubblediameter * cfdbv.diameter) + cfdbv.refinegap) &&
	       sq(x - Bubtx0) + sq(y) + sq(z) > sq(0.50*(cfdbv.bubblediameter*cfdbv.diameter) - cfdbv.refinegap) &&
	       level < LEVELmax);

	foreach ()
	{
		f[] = 0.0;
		if(sq(x - x0) + sq(y) + sq(z) < sq(0.50*cfdbv.diameter))
		{
			f[] = 1.0;
			u.x[] = -cfdbv.vel;
			u.y[] = 0.0;
		}
		if(sq(x - Bubtx0) + sq(y) + sq(z) < sq(0.50*(cfdbv.bubblediameter * cfdbv.diameter)))
		{
			f[] = 0.0;
			u.x[] = -cfdbv.vel;
			u.y[] = 0.0;
		}
		contact_angle[] = CONTACT_ANGLE_SUBSTRATE;
		mark[] = 0;
		tmp_c[] = f[];
	}

	boundary({contact_angle, mark, tmp_c});

	// Calculate initial volume
	initial_volume = 0.0;
	initial_bubble_volume = 0.0;
	foreach(reduction(+:initial_volume) reduction(+:initial_bubble_volume))
	{
		double dv = dv();
		initial_volume += f[] * dv;
		if (fb[] > 0.5) initial_bubble_volume += fb[] * dv;
	}

	fprintf(stderr, "Initial drop volume: %.12e\n", initial_volume);
	fprintf(stderr, "Initial bubble volume: %.12e\n\n", initial_bubble_volume);
}

event adapt(i++)
{
	double refine[3];
	refine[0] = pow(10.0, REFINE_VALUE_0);
	refine[1] = pow(10.0, REFINE_VALUE_1);
	refine[2] = pow(10.0, REFINE_VALUE_2);
	adapt_wavelet(REFINE_VAR, (double[]){refine[0], refine[1], refine[2]},
	              maxlevel = LEVELmax, minlevel = LEVELmin);
}

event measure_volume (i++)
{
	// Calculate current volume
	double current_volume = 0.0;
	double current_bubble_volume = 0.0;

	foreach(reduction(+:current_volume) reduction(+:current_bubble_volume))
	{
		double dv = dv();
		current_volume += f[] * dv;
		if (fb[] > 0.5) current_bubble_volume += fb[] * dv;
	}

	// Calculate errors
	double volume_error = fabs(current_volume - initial_volume);
	double volume_ratio = current_volume / initial_volume;
	double relative_error = volume_error / initial_volume;

	double bubble_error = fabs(current_bubble_volume - initial_bubble_volume);
	double bubble_ratio = (initial_bubble_volume > 0) ? current_bubble_volume / initial_bubble_volume : 0.0;

	// Update statistics
	if (volume_error > max_volume_error) max_volume_error = volume_error;
	if (bubble_error > max_bubble_error) max_bubble_error = bubble_error;
	mean_volume_error += relative_error;
	sample_count++;

	// Output to stderr (for monitoring)
	if (i % 10 == 0) {
		fprintf(stderr, "t=%6.4f: V/V0=%.12f, err=%.3e, rel_err=%.3e\n",
		        t, volume_ratio, volume_error, relative_error);
	}

	// Write to results file
	static FILE *fp = NULL;
	if (fp == NULL) {
		char filename[256];
		sprintf(filename, "../results/mass_conservation_L%d-%d.txt", LEVELmin, LEVELmax);
		fp = fopen(filename, "w");
		fprintf(fp, "# Mass Conservation Test Results\n");
		fprintf(fp, "# Re=%.2f, We=%.2f, Levels=%d-%d\n",
		        cfdbv.Reynolds, cfdbv.Weber, LEVELmin, LEVELmax);
		fprintf(fp, "# Time  Volume  Volume_Ratio  Abs_Error  Rel_Error  Bubble_Vol  Bubble_Ratio\n");
	}

	fprintf(fp, "%.6f  %.12e  %.12f  %.6e  %.6e  %.12e  %.12f\n",
	        t, current_volume, volume_ratio, volume_error, relative_error,
	        current_bubble_volume, bubble_ratio);
	fflush(fp);
}

event end(t = MAX_TIME)
{
	mean_volume_error /= sample_count;

	fprintf(stderr, "\n========================================\n");
	fprintf(stderr, "Mass Conservation Test - RESULTS\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Max volume error: %.6e\n", max_volume_error);
	fprintf(stderr, "Mean relative error: %.6e\n", mean_volume_error);
	fprintf(stderr, "Max bubble error: %.6e\n", max_bubble_error);
	fprintf(stderr, "\n");

	// Determine pass/fail
	double tolerance_sharp = 1e-10;
	double tolerance_standard = 1e-8;
	double tolerance = tolerance_sharp;  // Using Sharp VOF

	bool passed = (max_volume_error / initial_volume) < tolerance;

	if (passed) {
		fprintf(stderr, "TEST STATUS: ✅ PASS\n");
		fprintf(stderr, "Mass is conserved within tolerance (%.1e)\n", tolerance);
	} else {
		fprintf(stderr, "TEST STATUS: ❌ FAIL\n");
		fprintf(stderr, "Mass conservation violated!\n");
		fprintf(stderr, "Error %.6e exceeds tolerance %.6e\n",
		        max_volume_error / initial_volume, tolerance);
	}
	fprintf(stderr, "========================================\n");

	// Write summary JSON
	char summary_file[256];
	sprintf(summary_file, "../results/mass_conservation_summary.json");
	FILE *fs = fopen(summary_file, "w");
	fprintf(fs, "{\n");
	fprintf(fs, "  \"test_name\": \"mass_conservation\",\n");
	fprintf(fs, "  \"status\": \"%s\",\n", passed ? "PASS" : "FAIL");
	fprintf(fs, "  \"metrics\": {\n");
	fprintf(fs, "    \"max_volume_error\": %.6e,\n", max_volume_error);
	fprintf(fs, "    \"mean_relative_error\": %.6e,\n", mean_volume_error);
	fprintf(fs, "    \"max_bubble_error\": %.6e,\n", max_bubble_error);
	fprintf(fs, "    \"initial_volume\": %.12e\n", initial_volume);
	fprintf(fs, "  },\n");
	fprintf(fs, "  \"parameters\": {\n");
	fprintf(fs, "    \"Re\": %.2f,\n", cfdbv.Reynolds);
	fprintf(fs, "    \"We\": %.2f,\n", cfdbv.Weber);
	fprintf(fs, "    \"levels\": \"%d-%d\",\n", LEVELmin, LEVELmax);
	fprintf(fs, "    \"duration\": %.2f\n", MAX_TIME);
	fprintf(fs, "  }\n");
	fprintf(fs, "}\n");
	fclose(fs);

	exit(passed ? 0 : 1);
}
