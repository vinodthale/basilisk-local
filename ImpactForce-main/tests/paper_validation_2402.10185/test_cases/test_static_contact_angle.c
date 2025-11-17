/**
 * Test Case 1: Static Contact Angle Validation
 *
 * Reference: Tavares et al. (2024) arXiv:2402.10185, Section 3.1, Figure 3
 *
 * Purpose:
 * - Validate equilibrium contact angle implementation
 * - Verify spherical cap geometry
 * - Check Laplace pressure
 * - Measure spurious currents
 *
 * Expected Results:
 * - Contact angle matches specified within 2°
 * - Drop height/radius ratio matches spherical cap theory
 * - Laplace pressure Δp = σ/R within 5%
 * - Spurious currents |u_max| < 1e-6
 */

#include "../../../constants-sharp.h"

// Override parameters for static test
#undef MAX_TIME
#undef SAVE_FILE_EVERY
#undef VELOCITY
#undef DROP_DIAMETER
#undef DOMAIN_WIDTH

#define MAX_TIME 5.0             // Long time to reach equilibrium
#define SAVE_FILE_EVERY 0.1
#define VELOCITY 0.0             // No initial velocity
#define DROP_DIAMETER 2.0e-3     // 2 mm droplet
#define DOMAIN_WIDTH 6.0         // Large domain

// Test contact angles (degrees)
double test_angles[] = {30.0, 60.0, 90.0, 120.0, 150.0};
int num_test_angles = 5;
int current_test_idx = 0;

// Measurement storage
double droplet_height = 0.0;
double droplet_radius = 0.0;
double contact_radius = 0.0;
double measured_angle = 0.0;
double laplace_pressure = 0.0;
double max_velocity = 0.0;

struct CFDValues cfdbv;

// Boundary conditions
u.t[left] = dirichlet(0);  // No slip at substrate
f[left] = 0.;              // Non-wetting BC (will be overridden by contact angle)
u.n[right] = neumann(0);
p[right] = dirichlet(0);
u.n[top] = neumann(0);
p[top] = dirichlet(0);

/**
 * Theoretical spherical cap geometry
 */
double spherical_cap_height(double R, double theta_deg) {
	double theta = theta_deg * pi / 180.0;
	return R * (1.0 - cos(theta));
}

double spherical_cap_radius(double R, double theta_deg) {
	double theta = theta_deg * pi / 180.0;
	return R * sin(theta);
}

/**
 * Measure droplet geometry
 */
void measure_droplet_shape() {
	// Find maximum height
	droplet_height = 0.0;
	foreach(reduction(max:droplet_height)) {
		if (f[] > 0.5) {
			if (x > droplet_height) droplet_height = x;
		}
	}

	// Find contact radius (maximum radial extent at substrate)
	contact_radius = 0.0;
	foreach(reduction(max:contact_radius)) {
		if (x < 0.1 * cfdbv.diameter && f[] > 0.5) {
			if (fabs(y) > contact_radius) contact_radius = fabs(y);
		}
	}

	// Estimate droplet radius from volume conservation
	double volume = 0.0;
	foreach(reduction(+:volume)) {
		volume += f[] * dv();
	}
	// V = πR³(2/3 - cosθ + cos³θ/3) for spherical cap
	// Approximate as 4πR³/3 for full sphere
	droplet_radius = pow(3.0 * volume / (4.0 * pi), 1.0/3.0);
}

/**
 * Measure contact angle from interface shape near substrate
 */
double measure_contact_angle_shape() {
	double angle_sum = 0.0;
	int count = 0;

	// Find interface cells near substrate
	foreach() {
		if (x < 0.1 && f[] > 0.1 && f[] < 0.9) {
			// Calculate interface normal
			coord n = interface_normal(point, f);
			double magnitude = sqrt(sq(n.x) + sq(n.y));

			if (magnitude > 0.1) {
				// Angle from horizontal
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

/**
 * Measure maximum spurious current
 */
double measure_spurious_currents() {
	double max_u = 0.0;

	foreach(reduction(max:max_u)) {
		if (f[] > 0.5 && f[] < 0.99) {  // Near interface
			double u_mag = sqrt(sq(u.x[]) + sq(u.y[]));
			if (u_mag > max_u) max_u = u_mag;
		}
	}

	return max_u;
}

/**
 * Measure Laplace pressure
 */
double measure_pressure_jump() {
	double p_inside = 0.0, p_outside = 0.0;
	int n_in = 0, n_out = 0;

	foreach() {
		if (f[] > 0.9) {  // Inside droplet
			p_inside += p[];
			n_in++;
		} else if (f[] < 0.1) {  // Outside droplet
			p_outside += p[];
			n_out++;
		}
	}

	if (n_in > 0) p_inside /= n_in;
	if (n_out > 0) p_outside /= n_out;

	return p_inside - p_outside;
}

int main(int argc, char **argv) {
	// Parse test angle if specified
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == 'A') {
			double angle = atof(&argv[i][1]);
			test_angles[0] = angle;
			num_test_angles = 1;
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
	fprintf(stderr, "Static Contact Angle Test\n");
	fprintf(stderr, "Paper: Tavares et al. (2024) arXiv:2402.10185\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Testing contact angle: %.1f°\n", test_angles[current_test_idx]);
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
	// Initialize as sphere in center of domain
	double x0 = cfdbv.diameter;  // Center above substrate
	fraction(f, sq(0.5*cfdbv.diameter) - (sq(x - x0) + sq(y) + sq(z)));
}

event init(i = 0) {
	double x0 = cfdbv.diameter;

	refine(sq(x - x0) + sq(y) + sq(z) < sq(0.6*cfdbv.diameter) &&
	       level < LEVELmax);

	foreach() {
		f[] = 0.0;
		if (sq(x - x0) + sq(y) + sq(z) < sq(0.5*cfdbv.diameter)) {
			f[] = 1.0;
		}

		// Set contact angle
		contact_angle[] = test_angles[current_test_idx];
		mark[] = 0;
		tmp_c[] = f[];

		// No initial velocity (static case)
		u.x[] = 0.0;
		u.y[] = 0.0;
	}

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

event measurements (t += 0.1) {
	measure_droplet_shape();
	measured_angle = measure_contact_angle_shape();
	max_velocity = measure_spurious_currents();
	laplace_pressure = measure_pressure_jump();

	// Write to results file
	static FILE *fp = NULL;
	if (fp == NULL) {
		char filename[256];
		sprintf(filename, "../results/static_contact_angle_%.0f_deg.txt",
		        test_angles[current_test_idx]);
		fp = fopen(filename, "w");
		fprintf(fp, "# Static Contact Angle Test - Paper Validation\n");
		fprintf(fp, "# Specified angle: %.1f°\n", test_angles[current_test_idx]);
		fprintf(fp, "# Time  Height  Radius  Contact_Radius  Measured_Angle  Pressure_Jump  Max_Velocity\n");
	}

	fprintf(fp, "%.4f  %.6e  %.6e  %.6e  %.4f  %.6e  %.6e\n",
	        t, droplet_height, droplet_radius, contact_radius,
	        measured_angle, laplace_pressure, max_velocity);
	fflush(fp);

	if (i % 10 == 0 && measured_angle > 0) {
		fprintf(stderr, "t=%.2f: θ=%.2f°, H=%.3f, R=%.3f, Δp=%.3e, |u|=%.3e\n",
		        t, measured_angle, droplet_height/cfdbv.diameter,
		        contact_radius/cfdbv.diameter, laplace_pressure, max_velocity);
	}
}

event end(t = MAX_TIME) {
	double specified_angle = test_angles[current_test_idx];

	// Theoretical values
	double h_theory = spherical_cap_height(droplet_radius, specified_angle);
	double r_theory = spherical_cap_radius(droplet_radius, specified_angle);
	double dp_theory = cfdbv.Sigma / droplet_radius;  // Laplace pressure

	// Errors
	double angle_error = fabs(measured_angle - specified_angle);
	double height_error = fabs(droplet_height - h_theory) / h_theory * 100.0;
	double radius_error = fabs(contact_radius - r_theory) / r_theory * 100.0;
	double pressure_error = fabs(laplace_pressure - dp_theory) / dp_theory * 100.0;

	fprintf(stderr, "\n========================================\n");
	fprintf(stderr, "Paper Validation Results\n");
	fprintf(stderr, "Test: Static Contact Angle\n");
	fprintf(stderr, "========================================\n");
	fprintf(stderr, "Specified angle: %.1f°\n", specified_angle);
	fprintf(stderr, "Measured angle: %.2f° (error: %.2f°)\n", measured_angle, angle_error);
	fprintf(stderr, "\n");
	fprintf(stderr, "Droplet height:\n");
	fprintf(stderr, "  Theory: %.4f mm\n", h_theory * 1000);
	fprintf(stderr, "  Measured: %.4f mm\n", droplet_height * 1000);
	fprintf(stderr, "  Error: %.2f%%\n", height_error);
	fprintf(stderr, "\n");
	fprintf(stderr, "Contact radius:\n");
	fprintf(stderr, "  Theory: %.4f mm\n", r_theory * 1000);
	fprintf(stderr, "  Measured: %.4f mm\n", contact_radius * 1000);
	fprintf(stderr, "  Error: %.2f%%\n", radius_error);
	fprintf(stderr, "\n");
	fprintf(stderr, "Laplace pressure:\n");
	fprintf(stderr, "  Theory: %.4f Pa\n", dp_theory);
	fprintf(stderr, "  Measured: %.4f Pa\n", laplace_pressure);
	fprintf(stderr, "  Error: %.2f%%\n", pressure_error);
	fprintf(stderr, "\n");
	fprintf(stderr, "Spurious currents: %.3e m/s\n", max_velocity);
	fprintf(stderr, "\n");

	// Pass/fail criteria
	bool angle_pass = angle_error < 2.0;  // Paper criterion
	bool geometry_pass = (height_error < 10.0) && (radius_error < 10.0);
	bool pressure_pass = pressure_error < 5.0;
	bool spurious_pass = max_velocity < 1e-6;

	bool overall_pass = angle_pass && geometry_pass && pressure_pass && spurious_pass;

	if (overall_pass) {
		fprintf(stderr, "TEST STATUS: ✅ PASS\n");
		fprintf(stderr, "All criteria met for paper validation\n");
	} else {
		fprintf(stderr, "TEST STATUS: ❌ FAIL\n");
		if (!angle_pass) fprintf(stderr, "  ✗ Contact angle error > 2°\n");
		if (!geometry_pass) fprintf(stderr, "  ✗ Geometry error > 10%%\n");
		if (!pressure_pass) fprintf(stderr, "  ✗ Pressure error > 5%%\n");
		if (!spurious_pass) fprintf(stderr, "  ✗ Spurious currents too large\n");
	}
	fprintf(stderr, "========================================\n");

	// Write summary JSON
	char summary_file[256];
	sprintf(summary_file, "../results/static_contact_angle_%.0f_summary.json", specified_angle);
	FILE *fs = fopen(summary_file, "w");
	fprintf(fs, "{\n");
	fprintf(fs, "  \"test_name\": \"static_contact_angle\",\n");
	fprintf(fs, "  \"paper_reference\": \"Tavares et al. (2024) arXiv:2402.10185\",\n");
	fprintf(fs, "  \"status\": \"%s\",\n", overall_pass ? "PASS" : "FAIL");
	fprintf(fs, "  \"metrics\": {\n");
	fprintf(fs, "    \"specified_angle\": %.1f,\n", specified_angle);
	fprintf(fs, "    \"measured_angle\": %.2f,\n", measured_angle);
	fprintf(fs, "    \"angle_error\": %.2f,\n", angle_error);
	fprintf(fs, "    \"height_error_percent\": %.2f,\n", height_error);
	fprintf(fs, "    \"radius_error_percent\": %.2f,\n", radius_error);
	fprintf(fs, "    \"pressure_error_percent\": %.2f,\n", pressure_error);
	fprintf(fs, "    \"max_spurious_velocity\": %.6e\n", max_velocity);
	fprintf(fs, "  },\n");
	fprintf(fs, "  \"criteria\": {\n");
	fprintf(fs, "    \"angle\": \"%s\",\n", angle_pass ? "PASS" : "FAIL");
	fprintf(fs, "    \"geometry\": \"%s\",\n", geometry_pass ? "PASS" : "FAIL");
	fprintf(fs, "    \"pressure\": \"%s\",\n", pressure_pass ? "PASS" : "FAIL");
	fprintf(fs, "    \"spurious\": \"%s\"\n", spurious_pass ? "PASS" : "FAIL");
	fprintf(fs, "  }\n");
	fprintf(fs, "}\n");
	fclose(fs);

	exit(overall_pass ? 0 : 1);
}
