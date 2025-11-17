#!/usr/bin/env python3
"""
Analysis Tools for Sharp VOF Test Suite
Processes test results and generates validation reports
"""

import numpy as np
import matplotlib.pyplot as plt
import json
import glob
import os
import sys
from pathlib import Path

class TestAnalyzer:
    def __init__(self, results_dir="../results"):
        self.results_dir = Path(results_dir)
        self.tests = {}

    def load_test_results(self, pattern="*.txt"):
        """Load all test result files"""
        files = glob.glob(str(self.results_dir / pattern))
        print(f"Found {len(files)} result files")

        for file in files:
            try:
                data = np.loadtxt(file, comments='#')
                test_name = Path(file).stem
                self.tests[test_name] = {
                    'file': file,
                    'data': data,
                    'metadata': self._load_metadata(file)
                }
                print(f"  Loaded: {test_name}")
            except Exception as e:
                print(f"  Error loading {file}: {e}")

    def _load_metadata(self, file):
        """Extract metadata from file header"""
        metadata = {}
        with open(file, 'r') as f:
            for line in f:
                if not line.startswith('#'):
                    break
                if '=' in line:
                    parts = line[1:].split('=')
                    if len(parts) == 2:
                        key = parts[0].strip()
                        value = parts[1].strip()
                        metadata[key] = value
        return metadata

    def analyze_mass_conservation(self):
        """Analyze mass conservation test results"""
        print("\n" + "="*50)
        print("Mass Conservation Analysis")
        print("="*50)

        mass_tests = {k: v for k, v in self.tests.items()
                     if 'mass_conservation' in k}

        if not mass_tests:
            print("No mass conservation tests found")
            return

        for test_name, test_data in mass_tests.items():
            data = test_data['data']
            if data.shape[1] < 4:
                continue

            time = data[:, 0]
            volume_ratio = data[:, 2]
            rel_error = data[:, 4]

            max_error = np.max(np.abs(rel_error))
            mean_error = np.mean(np.abs(rel_error))
            final_error = np.abs(rel_error[-1])

            print(f"\nTest: {test_name}")
            print(f"  Max relative error: {max_error:.6e}")
            print(f"  Mean relative error: {mean_error:.6e}")
            print(f"  Final relative error: {final_error:.6e}")

            # Pass criteria
            tolerance = 1e-10
            if max_error < tolerance:
                print(f"  Status: ✓ PASS (error < {tolerance:.1e})")
            else:
                print(f"  Status: ✗ FAIL (error > {tolerance:.1e})")

        return mass_tests

    def analyze_contact_angle(self):
        """Analyze contact angle test results"""
        print("\n" + "="*50)
        print("Contact Angle Analysis")
        print("="*50)

        angle_tests = {k: v for k, v in self.tests.items()
                      if 'contact_angle' in k}

        if not angle_tests:
            print("No contact angle tests found")
            return

        for test_name, test_data in angle_tests.items():
            data = test_data['data']
            if data.shape[1] < 2:
                continue

            time = data[:, 0]
            measured_angle = data[:, 1]

            # Extract specified angle from filename
            import re
            match = re.search(r'(\d+)_deg', test_name)
            if match:
                specified_angle = float(match.group(1))
            else:
                specified_angle = 90.0  # Default

            # Equilibrium angle (mean of last 20%)
            start_idx = int(len(measured_angle) * 0.8)
            equil_angle = np.mean(measured_angle[start_idx:])
            final_angle = measured_angle[-1]
            angle_error = abs(equil_angle - specified_angle)

            print(f"\nTest: {test_name}")
            print(f"  Specified angle: {specified_angle:.1f}°")
            print(f"  Measured angle (equilibrium): {equil_angle:.2f}°")
            print(f"  Final angle: {final_angle:.2f}°")
            print(f"  Error: {angle_error:.2f}°")

            # Pass criteria
            tolerance = 5.0  # degrees
            if angle_error < tolerance:
                print(f"  Status: ✓ PASS (error < {tolerance}°)")
            else:
                print(f"  Status: ✗ FAIL (error > {tolerance}°)")

        return angle_tests

    def load_summary_json(self):
        """Load all summary JSON files"""
        summaries = []
        json_files = glob.glob(str(self.results_dir / "*_summary.json"))

        for file in json_files:
            try:
                with open(file, 'r') as f:
                    summary = json.load(f)
                    summary['file'] = file
                    summaries.append(summary)
            except Exception as e:
                print(f"Error loading {file}: {e}")

        return summaries

    def generate_report(self, output="validation_report.txt"):
        """Generate comprehensive validation report"""
        print(f"\nGenerating report: {output}")

        with open(output, 'w') as f:
            f.write("="*70 + "\n")
            f.write("Sharp VOF Validation Report\n")
            f.write("="*70 + "\n\n")

            f.write(f"Generated: {os.popen('date').read()}\n")
            f.write(f"Results directory: {self.results_dir}\n")
            f.write(f"Total tests analyzed: {len(self.tests)}\n\n")

            # Load summaries
            summaries = self.load_summary_json()
            passed = sum(1 for s in summaries if s.get('status') == 'PASS')
            failed = sum(1 for s in summaries if s.get('status') == 'FAIL')

            f.write("="*70 + "\n")
            f.write("Summary\n")
            f.write("="*70 + "\n")
            f.write(f"Total tests: {len(summaries)}\n")
            f.write(f"Passed: {passed}\n")
            f.write(f"Failed: {failed}\n\n")

            # Detailed results
            f.write("="*70 + "\n")
            f.write("Detailed Results\n")
            f.write("="*70 + "\n\n")

            for summary in summaries:
                f.write(f"Test: {summary.get('test_name', 'Unknown')}\n")
                f.write(f"Status: {summary.get('status', 'Unknown')}\n")

                if 'metrics' in summary:
                    f.write("Metrics:\n")
                    for key, value in summary['metrics'].items():
                        if isinstance(value, float):
                            f.write(f"  {key}: {value:.6e}\n")
                        else:
                            f.write(f"  {key}: {value}\n")

                if 'parameters' in summary:
                    f.write("Parameters:\n")
                    for key, value in summary['parameters'].items():
                        f.write(f"  {key}: {value}\n")

                f.write("\n")

        print(f"Report saved to: {output}")

def main():
    import argparse

    parser = argparse.ArgumentParser(description='Analyze Sharp VOF test results')
    parser.add_argument('--results-dir', default='../results',
                       help='Results directory')
    parser.add_argument('--all', action='store_true',
                       help='Run all analyses')
    parser.add_argument('--mass', action='store_true',
                       help='Analyze mass conservation')
    parser.add_argument('--angle', action='store_true',
                       help='Analyze contact angle')
    parser.add_argument('--report', action='store_true',
                       help='Generate text report')
    parser.add_argument('--output', default='validation_report.txt',
                       help='Output report filename')

    args = parser.parse_args()

    analyzer = TestAnalyzer(results_dir=args.results_dir)
    analyzer.load_test_results()

    if args.all or args.mass:
        analyzer.analyze_mass_conservation()

    if args.all or args.angle:
        analyzer.analyze_contact_angle()

    if args.all or args.report:
        analyzer.generate_report(output=args.output)

    if not (args.all or args.mass or args.angle or args.report):
        # Default: show summary
        summaries = analyzer.load_summary_json()
        print(f"\nFound {len(summaries)} test summaries")
        passed = sum(1 for s in summaries if s.get('status') == 'PASS')
        failed = sum(1 for s in summaries if s.get('status') == 'FAIL')
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")

if __name__ == '__main__':
    main()
