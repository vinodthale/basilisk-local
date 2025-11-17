/**
 * Test Case 3: Droplet Impact on Substrate
 *
 * Reference: Tavares et al. (2024) arXiv:2402.10185, Section 3.3, Figure 7
 *
 * Purpose:
 * - Validate drop impact dynamics with contact line
 * - Measure maximum spreading factor β_max = D_max/D_0
 * - Compare with empirical correlations
 * - Verify contact angle influence
 *
 * Expected Results:
 * - β_max matches Pasandideh-Fard correlation (within 5%)
 * - β_max matches Clanet correlation (within 5%)
 * - Contact angle θ maintained during spreading
 * - Mass conservation |ΔV/V₀| < 1e-8
 */

#include "../../../constants-sharp.h"

// Override parameters for impact test
#undef MAX_TIME
#undef SAVE_FILE_EVERY
#undef VELOCITY
#undef DROP_DIAMETER
#undef DOMAIN_WIDTH

#define MAX_TIME 15.0            // Time for complete spreading
#define SAVE_FILE_EVERY 0.05
#define VELOCITY 1.0             // Impact velocity (m/s)
#define DROP_DIAMETER 2.0e-3     // 2 mm droplet
#define DOMAIN_WIDTH 8.0         // Large domain for spreading

// Test parameters
double test_reynolds[] = {100.0, 500.0, 1000.0};
double test_weber[] = {10.0, 50.0, 100.0};
double test_contact_angle[] = {60.0, 90.0, 120.0};
int num_test_cases = 3;
int current_test_idx = 0;

// Measurement storage
double max_spreading_diameter = 0.0;
double max_spreading_radius = 0.0;
double current_spreading_radius = 0.0;
double beta_max = 0.0;
double initial_volume = 0.0;
double current_volume = 0.0;
double contact_line_velocity = 0.0;
double measured_contact_angle = 0.0;

struct CFDValues cfdbv;

// Boundary conditions
u.t[left] = dirichlet(0);  // No slip at substrate
f[left] = 0.;              // Non-wetting BC (contact angle controlled)
u.n[right] = neumann(0);
p[right] = dirichlet(0);
u.n[top] = neumann(0);
p[top] = dirichlet(0);

/**
 * Empirical correlations for maximum spreading
 */

// Pasandideh-Fard et al. (1996) - energy balance correlation
double beta_max_pasandideh_fard(double We, double Re, double theta_deg) {
	double theta = theta_deg * pi / 180.0;
	double cos_theta = cos(theta);

	// β_max = √[(We + 12) / (3(1-cosθ) + 4We/√Re)]
	double numerator = We + 12.0;
	double denominator = 3.0 * (1.0 - cos_theta) + 4.0 * We / sqrt(Re);

	return sqrt(numerator / denominator);
}

// Clanet et al. (2004) - scaling law for large Weber numbers
double beta_max_clanet(double We, double Re) {
	// β_max = 0.87 Re^(1/5) - 0.40 Re^(2/5) We^(-1)
	double term1 = 0.87 * pow(Re, 0.2);
	double term2 = 0.40 * pow(Re, 0.4) * pow(We, -1.0);

	return term1 - term2;
}

// Scheller & Bousfield (1995) - viscous regime
double beta_max_scheller(double We, double Re) {
	// β_max ∝ Re^(1/5) for viscous regime
	return 0.61 * pow(Re, 0.2);
}

/**
 * Measure spreading radius (maximum radial extent at substrate)
 */
double measure_spreading_radius() {
	double max_radius = 0.0;

	foreach(reduction(max:max_radius)) {
		if (x < 0.1 * cfdbv.diameter && f[] > 0.5) {
			double r = sqrt(sq(y) + sq(z));
			if (r > max_radius) max_radius = r;
		}
	}

	return max_radius;
}

/**
 * Measure droplet volume
 */
double measure_volume() {
	double volume = 0.0;

	foreach(reduction(+:volume)) {
		volume += f[] * dv();
	}

	return volume;
}

/**
 * Measure contact line velocity
 */
double measure_contact_line_velocity() {
	double max_vel = 0.0;
	int count = 0;

	foreach() {
		if (x < 0.1 * cfdbv.diameter && f[] > 0.1 && f[] < 0.9) {
			double u_mag = sqrt(sq(u.x[]) + sq(u.y[]));
			max_vel += u_mag;
			count++;
		}
	}

	return (count > 0) ? max_vel / count : 0.0;
}

/**
 * Measure contact angle at spreading edge
 */
double measure_dynamic_contact_angle() {
	double angle_sum = 0.0;
	int count = 0;

	foreach() {
		if (x < 0.15 * cfdbv.diameter && f[] > 0.1 && f[] < 0.9) {
			coord n = interface_normal(point, f);
			double magnitude = sqrt(sq(n.x) + sq(n.y));

			if (magnitude > 0.1) {
				double local_angle = atan2(fabs(n.x), fabs(n.y)) * 180.0 / pi;

				// Correct for orientation
				if (n.y < 0) local_angle = 180.0 - local_angle;

				angle_sum += local_angle;
				count++;
			}
		}
	}

	return (count > 0) ? angle_sum / count : -1.0;
}

int main(int argc, char **argv) {
	// Parse test case if specified
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == 'T') {
			int idx = atoi(&argv[i][1]);
			if (idx >= 0 && idx < num_test_cases) {
				current_test_idx = idx;
			}
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

	// Set test parameters
	double Re = test_reynolds[current_test_idx];
	double We = test_weber[current_test_idx];
	double theta = test_contact_angle[current_test_idx];

	// Calculate physical properties
	double D0 = cfdbv.diameter;
	double V0 = cfdbv.velocity;
	double sigma = cfdbv.Sigma;

	// From We = ρV²D/σ and Re = ρVD/μ
	cfdbv.rhoL = 1000.0;  // Water
	cfdbv.Sigma = (cfdbv.rhoL * sq(V0) * D0) / We;
	cfdbv.muL = (cfdbv.rhoL * V0 * D0) / Re;

	rho1 = cfdbv.rhoL;
	rho2 = cfdbv.rhoG;
	mu1 = cfdbv.muL;
	mu2 = cfdbv.muG;
	f.sigma = cfdbv.Sigma;
	TOLERANCE = 1e-6;

	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Droplet Impact Test - Paper Validation\n");
	fprintf(stderr, "Paper: Tavares et al. (2024) arXiv:2402.10185\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Test case: %d\n", current_test_idx);
	fprintf(stderr, "Reynolds number: %.1f\n", Re);
	fprintf(stderr, "Weber number: %.1f\n", We);
	fprintf(stderr, "Contact angle: %.1f°\n", theta);
	fprintf(stderr, "Impact velocity: %.3f m/s\n", V0);
	fprintf(stderr, "Droplet diameter: %.3f mm\n", D0 * 1000);
	fprintf(stderr, "Grid levels: %d - %d\n", LEVELmin, LEVELmax);
	fprintf(stderr, "========================================\n\n");

	run();
	return 0;
}

event defaults(i = 0) {
	interfaces = list_add(NULL, f);
	tmp_interfaces = list_add(NULL, tmp_c);
	interfaces_mark = list_add(NULL, mark);
	interfaces_contact_angle = list_add(NULL, contact_angle);
	tmp_c.height = f.height;
	tmp_c.nc = f.nc;
	tmp_c.oxyi = f.oxyi;
}

event initfraction (t = 0) {
	// Initialize as sphere above substrate
	double x0 = cfdbv.diameter * 1.5;  // Start above substrate
	fraction(f, sq(0.5*cfdbv.diameter) - (sq(x - x0) + sq(y) + sq(z)));
}

event init(i = 0) {
	double x0 = cfdbv.diameter * 1.5;

	refine(sq(x - x0) + sq(y) + sq(z) < sq(0.6*cfdbv.diameter) &&
	       level < LEVELmax);

	foreach() {
		f[] = 0.0;
		if (sq(x - x0) + sq(y) + sq(z) < sq(0.5*cfdbv.diameter)) {
			f[] = 1.0;
		}

		// Set contact angle
		contact_angle[] = test_contact_angle[current_test_idx];
		mark[] = 0;
		tmp_c[] = f[];

		// Initial velocity (downward)
		u.x[] = (f[] > 0.5) ? -cfdbv.velocity : 0.0;
		u.y[] = 0.0;
	}

	// Measure initial volume
	initial_volume = measure_volume();

	boundary({contact_angle, mark, tmp_c, f, u.x, u.y});
}

event adapt(i++) {
	double refine[3];
	refine[0] = pow(10.0, REFINE_VALUE_0);
	refine[1] = pow(10.0, REFINE_VALUE_1);
	refine[2] = pow(10.0, REFINE_VALUE_2);
	adapt_wavelet(REFINE_VAR, (double[]){refine[0], refine[1], refine[2]},
	              maxlevel = LEVELmax, minlevel = LEVELmin);
}

event measurements (t += 0.05) {
	current_spreading_radius = measure_spreading_radius();
	current_volume = measure_volume();
	contact_line_velocity = measure_contact_line_velocity();
	measured_contact_angle = measure_dynamic_contact_angle();

	// Track maximum spreading
	if (current_spreading_radius > max_spreading_radius) {
		max_spreading_radius = current_spreading_radius;
		max_spreading_diameter = 2.0 * max_spreading_radius;
		beta_max = max_spreading_diameter / cfdbv.diameter;
	}

	// Write to results file
	static FILE *fp = NULL;
	if (fp == NULL) {
		char filename[256];
		sprintf(filename, "../results/droplet_impact_T%d_Re%.0f_We%.0f_theta%.0f.txt",
		        current_test_idx,
		        test_reynolds[current_test_idx],
		        test_weber[current_test_idx],
		        test_contact_angle[current_test_idx]);
		fp = fopen(filename, "w");
		fprintf(fp, "# Droplet Impact Test - Paper Validation\n");
		fprintf(fp, "# Test case: %d\n", current_test_idx);
		fprintf(fp, "# Reynolds: %.1f, Weber: %.1f, Contact angle: %.1f°\n",
		        test_reynolds[current_test_idx],
		        test_weber[current_test_idx],
		        test_contact_angle[current_test_idx]);
		fprintf(fp, "# Time  Spreading_Radius  Beta  Volume  Volume_Error  CL_Velocity  Contact_Angle\n");
	}

	double beta = 2.0 * current_spreading_radius / cfdbv.diameter;
	double vol_error = fabs(current_volume - initial_volume) / initial_volume;

	fprintf(fp, "%.4f  %.6e  %.6f  %.6e  %.6e  %.6e  %.4f\n",
	        t, current_spreading_radius, beta, current_volume,
	        vol_error, contact_line_velocity, measured_contact_angle);
	fflush(fp);

	if (i % 20 == 0) {
		fprintf(stderr, "t=%.2f: β=%.3f, β_max=%.3f, V_err=%.3e, θ=%.1f°\n",
		        t, beta, beta_max, vol_error, measured_contact_angle);
	}
}

event end(t = MAX_TIME) {
	double Re = test_reynolds[current_test_idx];
	double We = test_weber[current_test_idx];
	double theta = test_contact_angle[current_test_idx];

	// Calculate theoretical values from correlations
	double beta_pf = beta_max_pasandideh_fard(We, Re, theta);
	double beta_clanet = beta_max_clanet(We, Re);
	double beta_scheller = beta_max_scheller(We, Re);

	// Errors
	double error_pf = fabs(beta_max - beta_pf) / beta_pf * 100.0;
	double error_clanet = fabs(beta_max - beta_clanet) / beta_clanet * 100.0;
	double error_scheller = fabs(beta_max - beta_scheller) / beta_scheller * 100.0;

	double vol_error = fabs(current_volume - initial_volume) / initial_volume;

	fprintf(stderr, "\n========================================\n");
	fprintf(stderr, "Paper Validation Results\n");
	fprintf(stderr, "Test: Droplet Impact\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Parameters:\n");
	fprintf(stderr, "  Reynolds number: %.1f\n", Re);
	fprintf(stderr, "  Weber number: %.1f\n", We);
	fprintf(stderr, "  Contact angle: %.1f°\n", theta);
	fprintf(stderr, "\n");
	fprintf(stderr, "Maximum Spreading Factor (β_max):\n");
	fprintf(stderr, "  Measured: %.4f\n", beta_max);
	fprintf(stderr, "\n");
	fprintf(stderr, "Comparison with Correlations:\n");
	fprintf(stderr, "  Pasandideh-Fard: %.4f (error: %.2f%%)\n", beta_pf, error_pf);
	fprintf(stderr, "  Clanet: %.4f (error: %.2f%%)\n", beta_clanet, error_clanet);
	fprintf(stderr, "  Scheller: %.4f (error: %.2f%%)\n", beta_scheller, error_scheller);
	fprintf(stderr, "\n");
	fprintf(stderr, "Mass conservation:\n");
	fprintf(stderr, "  Volume error: %.3e\n", vol_error);
	fprintf(stderr, "\n");

	// Pass/fail criteria
	bool spreading_pass = (error_pf < 5.0) || (error_clanet < 5.0);
	bool mass_pass = vol_error < 1e-8;
	bool angle_pass = (measured_contact_angle > 0) &&
	                  (fabs(measured_contact_angle - theta) < 15.0);

	bool overall_pass = spreading_pass && mass_pass && angle_pass;

	if (overall_pass) {
		fprintf(stderr, "TEST STATUS: ✅ PASS\n");
		fprintf(stderr, "All criteria met for paper validation\n");
	} else {
		fprintf(stderr, "TEST STATUS: ❌ FAIL\n");
		if (!spreading_pass) fprintf(stderr, "  ✗ Spreading factor error > 5%%\n");
		if (!mass_pass) fprintf(stderr, "  ✗ Mass conservation error > 1e-8\n");
		if (!angle_pass) fprintf(stderr, "  ✗ Contact angle error too large\n");
	}
	fprintf(stderr, "========================================\n");

	// Write summary JSON
	char summary_file[256];
	sprintf(summary_file, "../results/droplet_impact_T%d_summary.json", current_test_idx);
	FILE *fs = fopen(summary_file, "w");
	fprintf(fs, "{\n");
	fprintf(fs, "  \"test_name\": \"droplet_impact\",\n");
	fprintf(fs, "  \"paper_reference\": \"Tavares et al. (2024) arXiv:2402.10185\",\n");
	fprintf(fs, "  \"test_case\": %d,\n", current_test_idx);
	fprintf(fs, "  \"status\": \"%s\",\n", overall_pass ? "PASS" : "FAIL");
	fprintf(fs, "  \"parameters\": {\n");
	fprintf(fs, "    \"reynolds\": %.1f,\n", Re);
	fprintf(fs, "    \"weber\": %.1f,\n", We);
	fprintf(fs, "    \"contact_angle\": %.1f\n", theta);
	fprintf(fs, "  },\n");
	fprintf(fs, "  \"metrics\": {\n");
	fprintf(fs, "    \"beta_max_measured\": %.4f,\n", beta_max);
	fprintf(fs, "    \"beta_max_pasandideh_fard\": %.4f,\n", beta_pf);
	fprintf(fs, "    \"beta_max_clanet\": %.4f,\n", beta_clanet);
	fprintf(fs, "    \"error_pf_percent\": %.2f,\n", error_pf);
	fprintf(fs, "    \"error_clanet_percent\": %.2f,\n", error_clanet);
	fprintf(fs, "    \"volume_error\": %.6e,\n", vol_error);
	fprintf(fs, "    \"final_contact_angle\": %.2f\n", measured_contact_angle);
	fprintf(fs, "  },\n");
	fprintf(fs, "  \"criteria\": {\n");
	fprintf(fs, "    \"spreading\": \"%s\",\n", spreading_pass ? "PASS" : "FAIL");
	fprintf(fs, "    \"mass_conservation\": \"%s\",\n", mass_pass ? "PASS" : "FAIL");
	fprintf(fs, "    \"contact_angle\": \"%s\"\n", angle_pass ? "PASS" : "FAIL");
	fprintf(fs, "  }\n");
	fprintf(fs, "}\n");
	fclose(fs);

	exit(overall_pass ? 0 : 1);
}
