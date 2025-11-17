#!/usr/bin/env python3
"""
Analysis Tool: Compare Results with Paper Data
Tavares et al. (2024) arXiv:2402.10185

Loads simulation results and compares with reference data from the paper.
Generates comparison reports and validation metrics.
"""

import numpy as np
import json
import glob
import sys
from pathlib import Path

class PaperComparison:
    def __init__(self, results_dir="../results", reference_dir="../reference_data"):
        self.results_dir = Path(results_dir)
        self.reference_dir = Path(reference_dir)
        self.tests = {}
        self.references = {}

    def load_results(self):
        """Load all test result JSON summaries"""
        json_files = glob.glob(str(self.results_dir / "*_summary.json"))

        print(f"Loading results from: {self.results_dir}")
        print(f"Found {len(json_files)} result files")

        for file in json_files:
            try:
                with open(file, 'r') as f:
                    data = json.load(f)
                    test_name = data.get('test_name', Path(file).stem)
                    self.tests[test_name] = data
                    print(f"  ✓ Loaded: {test_name}")
            except Exception as e:
                print(f"  ✗ Error loading {file}: {e}")

        return self.tests

    def load_reference_data(self):
        """Load reference data from paper"""
        # Load reference JSON files if they exist
        json_files = glob.glob(str(self.reference_dir / "*.json"))

        if not json_files:
            print(f"Warning: No reference data found in {self.reference_dir}")
            print("Reference data should be extracted from paper figures")
            return {}

        for file in json_files:
            try:
                with open(file, 'r') as f:
                    data = json.load(f)
                    test_name = Path(file).stem.replace('_reference', '')
                    self.references[test_name] = data
                    print(f"  ✓ Loaded reference: {test_name}")
            except Exception as e:
                print(f"  ✗ Error loading {file}: {e}")

        return self.references

    def compare_static_contact_angle(self):
        """Compare static contact angle test with paper"""
        print("\n" + "="*60)
        print("Test 1: Static Contact Angle")
        print("Paper Reference: Section 3.1, Figure 3")
        print("="*60)

        # Find all static contact angle tests
        sca_tests = {k: v for k, v in self.tests.items()
                     if 'static_contact_angle' in k}

        if not sca_tests:
            print("No static contact angle results found")
            return

        for test_name, test_data in sca_tests.items():
            status = test_data.get('status', 'UNKNOWN')
            metrics = test_data.get('metrics', {})

            specified_angle = metrics.get('specified_angle', 0)
            measured_angle = metrics.get('measured_angle', 0)
            angle_error = metrics.get('angle_error', 0)

            print(f"\nTest: {test_name}")
            print(f"  Specified angle: {specified_angle:.1f}°")
            print(f"  Measured angle: {measured_angle:.2f}°")
            print(f"  Error: {angle_error:.2f}°")
            print(f"  Status: {status}")

            # Paper criterion: error < 2°
            if angle_error < 2.0:
                print("  ✓ Meets paper criterion (< 2°)")
            else:
                print("  ✗ Does not meet paper criterion (< 2°)")

            # Check other metrics
            height_error = metrics.get('height_error_percent', 0)
            radius_error = metrics.get('radius_error_percent', 0)
            pressure_error = metrics.get('pressure_error_percent', 0)
            spurious = metrics.get('max_spurious_velocity', 0)

            print(f"  Height error: {height_error:.2f}%")
            print(f"  Radius error: {radius_error:.2f}%")
            print(f"  Pressure error: {pressure_error:.2f}%")
            print(f"  Spurious currents: {spurious:.3e} m/s")

    def compare_droplet_impact(self):
        """Compare droplet impact test with paper and correlations"""
        print("\n" + "="*60)
        print("Test 3: Droplet Impact")
        print("Paper Reference: Section 3.3, Figure 7")
        print("="*60)

        # Find all droplet impact tests
        impact_tests = {k: v for k, v in self.tests.items()
                       if 'droplet_impact' in k}

        if not impact_tests:
            print("No droplet impact results found")
            return

        for test_name, test_data in impact_tests.items():
            status = test_data.get('status', 'UNKNOWN')
            params = test_data.get('parameters', {})
            metrics = test_data.get('metrics', {})

            Re = params.get('reynolds', 0)
            We = params.get('weber', 0)
            theta = params.get('contact_angle', 0)

            beta_measured = metrics.get('beta_max_measured', 0)
            beta_pf = metrics.get('beta_max_pasandideh_fard', 0)
            beta_clanet = metrics.get('beta_max_clanet', 0)

            error_pf = metrics.get('error_pf_percent', 0)
            error_clanet = metrics.get('error_clanet_percent', 0)

            print(f"\nTest: {test_name}")
            print(f"  Re = {Re:.0f}, We = {We:.0f}, θ = {theta:.0f}°")
            print(f"\n  Maximum Spreading Factor:")
            print(f"    Measured:         β_max = {beta_measured:.4f}")
            print(f"    Pasandideh-Fard:  β_max = {beta_pf:.4f} (error: {error_pf:.2f}%)")
            print(f"    Clanet:           β_max = {beta_clanet:.4f} (error: {error_clanet:.2f}%)")

            # Paper criterion: error < 5%
            if error_pf < 5.0 or error_clanet < 5.0:
                print(f"  ✓ Meets paper criterion (< 5% error)")
            else:
                print(f"  ✗ Does not meet paper criterion (< 5% error)")

            # Mass conservation
            vol_error = metrics.get('volume_error', 0)
            print(f"\n  Mass conservation:")
            print(f"    Volume error: {vol_error:.3e}")

            if vol_error < 1e-8:
                print(f"    ✓ Good (< 1e-8)")
            else:
                print(f"    ⚠ Exceeds tolerance")

            print(f"\n  Status: {status}")

    def generate_comparison_report(self, output_file="paper_comparison_report.txt"):
        """Generate comprehensive comparison report"""
        print(f"\n{'='*60}")
        print("Generating Comparison Report")
        print(f"{'='*60}")

        output_path = self.results_dir / output_file

        with open(output_path, 'w') as f:
            f.write("="*70 + "\n")
            f.write("Paper Validation Comparison Report\n")
            f.write("="*70 + "\n\n")

            f.write("Reference:\n")
            f.write("  Title: A coupled VOF/embedded boundary method to model\n")
            f.write("         two-phase flows on arbitrary solid surfaces\n")
            f.write("  Authors: Mathilde Tavares, Christophe Josserand,\n")
            f.write("           Alexandre Limare, José María López-Herrera,\n")
            f.write("           Stéphane Popinet\n")
            f.write("  arXiv: 2402.10185v1\n")
            f.write("  Year: 2024\n\n")

            f.write("="*70 + "\n")
            f.write("Test Results Summary\n")
            f.write("="*70 + "\n\n")

            total = len(self.tests)
            passed = sum(1 for t in self.tests.values() if t.get('status') == 'PASS')
            failed = total - passed

            f.write(f"Total tests run: {total}\n")
            f.write(f"Passed: {passed}\n")
            f.write(f"Failed: {failed}\n\n")

            # Detailed results
            f.write("="*70 + "\n")
            f.write("Detailed Comparison\n")
            f.write("="*70 + "\n\n")

            for test_name, test_data in self.tests.items():
                f.write(f"Test: {test_name}\n")
                f.write("-"*70 + "\n")
                f.write(f"Status: {test_data.get('status', 'UNKNOWN')}\n")

                if 'parameters' in test_data:
                    f.write("Parameters:\n")
                    for key, value in test_data['parameters'].items():
                        f.write(f"  {key}: {value}\n")

                if 'metrics' in test_data:
                    f.write("Metrics:\n")
                    for key, value in test_data['metrics'].items():
                        if isinstance(value, float):
                            f.write(f"  {key}: {value:.6e}\n")
                        else:
                            f.write(f"  {key}: {value}\n")

                if 'criteria' in test_data:
                    f.write("Pass Criteria:\n")
                    for key, value in test_data['criteria'].items():
                        f.write(f"  {key}: {value}\n")

                f.write("\n")

            # Paper-specific comparisons
            f.write("="*70 + "\n")
            f.write("Paper-Specific Validation\n")
            f.write("="*70 + "\n\n")

            f.write("Test 1: Static Contact Angle (Section 3.1)\n")
            f.write("  Criterion: |θ_measured - θ_specified| < 2°\n")
            sca_tests = [t for k, t in self.tests.items() if 'static_contact_angle' in k]
            sca_pass = sum(1 for t in sca_tests if t.get('status') == 'PASS')
            f.write(f"  Results: {sca_pass}/{len(sca_tests)} passed\n\n")

            f.write("Test 3: Droplet Impact (Section 3.3)\n")
            f.write("  Criterion: |β_max - β_correlation| < 5%\n")
            impact_tests = [t for k, t in self.tests.items() if 'droplet_impact' in k]
            impact_pass = sum(1 for t in impact_tests if t.get('status') == 'PASS')
            f.write(f"  Results: {impact_pass}/{len(impact_tests)} passed\n\n")

        print(f"Report saved to: {output_path}")

    def print_summary(self):
        """Print quick summary to console"""
        print("\n" + "="*60)
        print("Paper Validation Summary")
        print("="*60)

        total = len(self.tests)
        passed = sum(1 for t in self.tests.values() if t.get('status') == 'PASS')
        failed = total - passed

        print(f"Total tests: {total}")
        print(f"Passed: {passed} ✓")
        print(f"Failed: {failed} ✗")

        if failed == 0 and total > 0:
            print("\n✓ All tests passed!")
            print("Results are consistent with paper validation criteria")
        elif passed > 0:
            print("\n⚠ Some tests failed")
            print("Review detailed comparison for issues")
        else:
            print("\n✗ All tests failed")
            print("Significant issues with validation")


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description='Compare simulation results with paper data (arXiv:2402.10185)'
    )
    parser.add_argument('--results-dir', default='../results',
                       help='Results directory')
    parser.add_argument('--reference-dir', default='../reference_data',
                       help='Reference data directory')
    parser.add_argument('--report', action='store_true',
                       help='Generate detailed report')
    parser.add_argument('--output', default='paper_comparison_report.txt',
                       help='Output report filename')

    args = parser.parse_args()

    # Create comparison object
    comparator = PaperComparison(
        results_dir=args.results_dir,
        reference_dir=args.reference_dir
    )

    # Load results
    print("="*60)
    print("Paper Validation Comparison Tool")
    print("Reference: Tavares et al. (2024) arXiv:2402.10185")
    print("="*60)

    comparator.load_results()
    comparator.load_reference_data()

    # Run comparisons
    comparator.compare_static_contact_angle()
    comparator.compare_droplet_impact()

    # Generate report if requested
    if args.report:
        comparator.generate_comparison_report(output_file=args.output)

    # Print summary
    comparator.print_summary()


if __name__ == '__main__':
    main()
