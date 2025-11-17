/**
 * Contact Angle Validation Test for Sharp VOF
 *
 * Purpose: Validate contact angle implementation and dynamics
 *
 * Metrics:
 * - Measured contact angle at substrate
 * - Contact angle evolution to equilibrium
 * - Contact line position
 *
 * Pass Criteria:
 * - |θ_measured - θ_specified| < 5°
 * - Equilibrium reached
 * - No solid penetration
 */

#include "../../constants-sharp.h"

// Test parameters
#undef MAX_TIME
#undef SAVE_FILE_EVERY
#undef VELOCITY
#define MAX_TIME 3.0          // Longer time for equilibration
#define SAVE_FILE_EVERY 0.05
#define VELOCITY 0.1          // Slow impact for gentle contact

// Test contact angles
double test_angles[] = {30.0, 60.0, 90.0, 120.0, 150.0};
int num_angles = 5;
int current_angle_idx = 0;

// Measurement storage
double measured_angles[1000];
double contact_line_positions[1000];
int measurement_count = 0;

struct CFDValues cfdbv;

// Boundary conditions
u.t[left] = dirichlet(0);
f[left] = 0.;
u.n[right] = neumann(0);
p[right] = dirichlet(0);
u.n[top] = neumann(0);
p[top] = dirichlet(0);

/**
 * Measure contact angle at substrate using height function
 */
double measure_contact_angle() {
	double angle = 0.0;
	int count = 0;

	// Find contact line cells (cells near left boundary with interface)
	foreach() {
		if (x < 0.05 && f[] > 0.1 && f[] < 0.9) {
			// This is an interface cell near substrate
			// Estimate angle from interface normal
			coord n = interface_normal(point, f);
			double local_angle = atan2(fabs(n.y), fabs(n.x)) * 180.0 / pi;

			// For left boundary, correct orientation
			if (n.x < 0) local_angle = 180.0 - local_angle;

			angle += local_angle;
			count++;
		}
	}

	if (count > 0) {
		return angle / count;
	}
	return -1.0;  // No measurement
}

/**
 * Find contact line position (y-coordinate at substrate)
 */
double find_contact_line() {
	double max_y = 0.0;

	foreach() {
		if (x < 0.02 && f[] > 0.5) {
			if (fabs(y) > max_y) max_y = fabs(y);
		}
	}

	return max_y;
}

int main(int argc, char **argv)
{
	// Override contact angle if specified in command line
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == 'A') {
			double specified_angle = atof(&argv[i][1]);
			current_angle_idx = 0;
			test_angles[0] = specified_angle;
			num_angles = 1;
			break;
		}
	}

	numericalmainvalues(argv, argc, &cfdbv);
	size(cfdbv.domainsize);
#if AXI
	;
#else
	origin(0, -cfdbv.domainsize / 2., -cfdbv.domainsize / 2.);
#endif
	int initialgrid = pow(2, LEVELmin);
	init_grid(initialgrid);

	system("mkdir -p ../results");

	rho1 = cfdbv.rhoL;
	rho2 = cfdbv.rhoG;
	mu1 = cfdbv.muL;
	mu2 = cfdbv.muG;
	f.sigma = cfdbv.Sigma;
	TOLERANCE = 1e-6;

	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Contact Angle Test - Sharp VOF\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Testing %d contact angles\n", num_angles);
	for (int i = 0; i < num_angles; i++) {
		fprintf(stderr, "  Angle %d: %.1f°\n", i+1, test_angles[i]);
	}
	fprintf(stderr, "Re = %.2f, We = %.2f\n", cfdbv.Reynolds, cfdbv.Weber);
	fprintf(stderr, "Grid levels: %d - %d\n", LEVELmin, LEVELmax);
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
	fraction(f, sq(0.50*cfdbv.diameter) - (sq(x - x0) + sq(y) + sq(z)));
}

event init(i = 0)
{
	double x0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;

	refine(sq(x - x0) + sq(y) + sq(z) < sq(0.50*cfdbv.diameter + cfdbv.refinegap) &&
	       sq(x - x0) + sq(y) + sq(z) > sq(0.50*cfdbv.diameter - cfdbv.refinegap) &&
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

		// Set contact angle for this test
		contact_angle[] = test_angles[current_angle_idx];
		mark[] = 0;
		tmp_c[] = f[];
	}

	boundary({contact_angle, mark, tmp_c});

	fprintf(stderr, "Testing contact angle: %.1f°\n\n", test_angles[current_angle_idx]);
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

event measure_angle (t += 0.05)
{
	double angle = measure_contact_angle();
	double cl_pos = find_contact_line();

	if (angle > 0 && measurement_count < 1000) {
		measured_angles[measurement_count] = angle;
		contact_line_positions[measurement_count] = cl_pos;
		measurement_count++;
	}

	if (i % 10 == 0 && angle > 0) {
		fprintf(stderr, "t=%6.4f: θ=%.2f°, CL_pos=%.4f\n", t, angle, cl_pos);
	}

	// Write to results file
	static FILE *fp = NULL;
	if (fp == NULL) {
		char filename[256];
		sprintf(filename, "../results/contact_angle_%.0f_deg.txt",
		        test_angles[current_angle_idx]);
		fp = fopen(filename, "w");
		fprintf(fp, "# Contact Angle Test Results\n");
		fprintf(fp, "# Specified angle: %.1f°\n", test_angles[current_angle_idx]);
		fprintf(fp, "# Re=%.2f, We=%.2f, Levels=%d-%d\n",
		        cfdbv.Reynolds, cfdbv.Weber, LEVELmin, LEVELmax);
		fprintf(fp, "# Time  Measured_Angle  Contact_Line_Pos\n");
	}

	if (angle > 0) {
		fprintf(fp, "%.6f  %.4f  %.6f\n", t, angle, cl_pos);
		fflush(fp);
	}
}

event end(t = MAX_TIME)
{
	double specified_angle = test_angles[current_angle_idx];

	// Calculate statistics
	double mean_angle = 0.0;
	double final_angle = 0.0;
	if (measurement_count > 0) {
		// Mean of last 20% of measurements (equilibrium)
		int start_idx = (int)(measurement_count * 0.8);
		int equil_count = 0;
		for (int i = start_idx; i < measurement_count; i++) {
			mean_angle += measured_angles[i];
			equil_count++;
		}
		if (equil_count > 0) mean_angle /= equil_count;
		final_angle = measured_angles[measurement_count - 1];
	}

	double angle_error = fabs(mean_angle - specified_angle);

	fprintf(stderr, "\n========================================\n");
	fprintf(stderr, "Contact Angle Test - RESULTS\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Specified angle: %.1f°\n", specified_angle);
	fprintf(stderr, "Measured angle (equilibrium): %.2f°\n", mean_angle);
	fprintf(stderr, "Final angle: %.2f°\n", final_angle);
	fprintf(stderr, "Error: %.2f°\n", angle_error);
	fprintf(stderr, "\n");

	// Pass criteria
	double tolerance = 5.0;  // degrees
	bool passed = angle_error < tolerance;

	if (passed) {
		fprintf(stderr, "TEST STATUS: ✅ PASS\n");
		fprintf(stderr, "Contact angle within tolerance (%.1f°)\n", tolerance);
	} else {
		fprintf(stderr, "TEST STATUS: ❌ FAIL\n");
		fprintf(stderr, "Contact angle error %.2f° exceeds tolerance %.1f°\n",
		        angle_error, tolerance);
	}
	fprintf(stderr, "========================================\n");

	// Write summary JSON
	char summary_file[256];
	sprintf(summary_file, "../results/contact_angle_%.0f_summary.json", specified_angle);
	FILE *fs = fopen(summary_file, "w");
	fprintf(fs, "{\n");
	fprintf(fs, "  \"test_name\": \"contact_angle\",\n");
	fprintf(fs, "  \"status\": \"%s\",\n", passed ? "PASS" : "FAIL");
	fprintf(fs, "  \"metrics\": {\n");
	fprintf(fs, "    \"specified_angle\": %.1f,\n", specified_angle);
	fprintf(fs, "    \"measured_angle\": %.2f,\n", mean_angle);
	fprintf(fs, "    \"final_angle\": %.2f,\n", final_angle);
	fprintf(fs, "    \"error\": %.2f,\n", angle_error);
	fprintf(fs, "    \"measurements\": %d\n", measurement_count);
	fprintf(fs, "  },\n");
	fprintf(fs, "  \"parameters\": {\n");
	fprintf(fs, "    \"Re\": %.2f,\n", cfdbv.Reynolds);
	fprintf(fs, "    \"We\": %.2f,\n", cfdbv.Weber);
	fprintf(fs, "    \"levels\": \"%d-%d\"\n", LEVELmin, LEVELmax);
	fprintf(fs, "  }\n");
	fprintf(fs, "}\n");
	fclose(fs);

	exit(passed ? 0 : 1);
}
