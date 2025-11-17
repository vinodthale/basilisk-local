#!/usr/bin/env python3
"""
Visualization Tool: Plot Comparisons with Paper
Tavares et al. (2024) arXiv:2402.10185

Recreates paper figures and overlays simulation results for comparison.
"""

import numpy as np
import matplotlib.pyplot as plt
import glob
import json
import sys
from pathlib import Path

class PaperPlotter:
    def __init__(self, results_dir="../results", reference_dir="../reference_data"):
        self.results_dir = Path(results_dir)
        self.reference_dir = Path(reference_dir)

    def plot_static_contact_angle(self, save=False):
        """
        Plot static contact angle results
        Recreates Paper Figure 3
        """
        # Find result files
        files = glob.glob(str(self.results_dir / "static_contact_angle_*_deg.txt"))

        if not files:
            print("No static contact angle results found")
            return

        fig, axes = plt.subplots(2, 2, figsize=(12, 10))

        # Data containers
        angles_specified = []
        angles_measured = []
        angle_errors = []
        spurious_currents = []

        for file in files:
            try:
                data = np.loadtxt(file, comments='#')
                if data.shape[1] < 7:
                    continue

                time = data[:, 0]
                height = data[:, 1]
                contact_radius = data[:, 3]
                measured_angle = data[:, 4]
                pressure_jump = data[:, 5]
                max_velocity = data[:, 6]

                # Extract specified angle from filename
                import re
                match = re.search(r'(\d+)_deg', file)
                if match:
                    specified_angle = float(match.group(1))
                else:
                    specified_angle = 90.0

                # Store for summary plot
                angles_specified.append(specified_angle)
                angles_measured.append(measured_angle[-1])
                angle_errors.append(abs(measured_angle[-1] - specified_angle))
                spurious_currents.append(np.max(np.abs(max_velocity)))

                label = f"θ = {specified_angle:.0f}°"

                # Plot 1: Height evolution
                axes[0, 0].plot(time, height * 1000, label=label, linewidth=2)

                # Plot 2: Contact radius evolution
                axes[0, 1].plot(time, contact_radius * 1000, label=label, linewidth=2)

                # Plot 3: Measured angle vs time
                axes[1, 0].plot(time, measured_angle, label=label, linewidth=2)
                axes[1, 0].axhline(y=specified_angle, color='gray',
                                  linestyle='--', alpha=0.3)

                # Plot 4: Spurious currents
                axes[1, 1].semilogy(time, np.abs(max_velocity), label=label, linewidth=2)

            except Exception as e:
                print(f"Error plotting {file}: {e}")

        # Configure axes
        # Plot 1: Height
        axes[0, 0].set_xlabel('Time (s)', fontsize=11)
        axes[0, 0].set_ylabel('Droplet Height (mm)', fontsize=11)
        axes[0, 0].set_title('(a) Height Evolution', fontsize=12, fontweight='bold')
        axes[0, 0].grid(True, alpha=0.3)
        axes[0, 0].legend(fontsize=9)

        # Plot 2: Contact radius
        axes[0, 1].set_xlabel('Time (s)', fontsize=11)
        axes[0, 1].set_ylabel('Contact Radius (mm)', fontsize=11)
        axes[0, 1].set_title('(b) Contact Radius Evolution', fontsize=12, fontweight='bold')
        axes[0, 1].grid(True, alpha=0.3)
        axes[0, 1].legend(fontsize=9)

        # Plot 3: Contact angle
        axes[1, 0].set_xlabel('Time (s)', fontsize=11)
        axes[1, 0].set_ylabel('Contact Angle (degrees)', fontsize=11)
        axes[1, 0].set_title('(c) Contact Angle Evolution', fontsize=12, fontweight='bold')
        axes[1, 0].axhline(y=5, color='r', linestyle='--', alpha=0.3, label='±5° tolerance')
        axes[1, 0].axhline(y=-5, color='r', linestyle='--', alpha=0.3)
        axes[1, 0].grid(True, alpha=0.3)
        axes[1, 0].legend(fontsize=9)

        # Plot 4: Spurious currents
        axes[1, 1].axhline(y=1e-6, color='r', linestyle='--', alpha=0.5, label='Target < 1e-6')
        axes[1, 1].set_xlabel('Time (s)', fontsize=11)
        axes[1, 1].set_ylabel('|u| (m/s)', fontsize=11)
        axes[1, 1].set_title('(d) Spurious Currents', fontsize=12, fontweight='bold')
        axes[1, 1].grid(True, alpha=0.3, which='both')
        axes[1, 1].legend(fontsize=9)

        plt.suptitle('Static Contact Angle Validation\n(Paper Section 3.1, Figure 3)',
                    fontsize=14, fontweight='bold', y=0.995)
        plt.tight_layout()

        if save:
            output_file = self.results_dir / "fig_static_contact_angle.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"Saved: {output_file}")
        else:
            plt.show()

    def plot_droplet_impact(self, save=False):
        """
        Plot droplet impact results
        Recreates Paper Figure 7
        """
        # Find result files
        files = glob.glob(str(self.results_dir / "droplet_impact_*.txt"))

        if not files:
            print("No droplet impact results found")
            return

        fig, axes = plt.subplots(2, 2, figsize=(12, 10))

        for file in files:
            try:
                data = np.loadtxt(file, comments='#')
                if data.shape[1] < 7:
                    continue

                time = data[:, 0]
                spreading_radius = data[:, 1]
                beta = data[:, 2]
                volume = data[:, 3]
                vol_error = data[:, 4]
                cl_velocity = data[:, 5]
                contact_angle = data[:, 6]

                # Extract parameters from filename
                import re
                match = re.search(r'Re(\d+)_We(\d+)_theta(\d+)', file)
                if match:
                    Re = float(match.group(1))
                    We = float(match.group(2))
                    theta = float(match.group(3))
                    label = f"Re={Re:.0f}, We={We:.0f}"
                else:
                    label = Path(file).stem

                # Plot 1: Spreading factor β(t)
                axes[0, 0].plot(time, beta, label=label, linewidth=2)

                # Plot 2: Contact line velocity
                axes[0, 1].plot(time, cl_velocity, label=label, linewidth=2)

                # Plot 3: Mass conservation
                axes[1, 0].semilogy(time, vol_error, label=label, linewidth=2)

                # Plot 4: Dynamic contact angle
                axes[1, 1].plot(time, contact_angle, label=label, linewidth=2)

            except Exception as e:
                print(f"Error plotting {file}: {e}")

        # Configure axes
        # Plot 1: Spreading factor
        axes[0, 0].set_xlabel('Time (s)', fontsize=11)
        axes[0, 0].set_ylabel('β = D/D₀', fontsize=11)
        axes[0, 0].set_title('(a) Spreading Factor Evolution', fontsize=12, fontweight='bold')
        axes[0, 0].grid(True, alpha=0.3)
        axes[0, 0].legend(fontsize=9)

        # Plot 2: Contact line velocity
        axes[0, 1].set_xlabel('Time (s)', fontsize=11)
        axes[0, 1].set_ylabel('u_cl (m/s)', fontsize=11)
        axes[0, 1].set_title('(b) Contact Line Velocity', fontsize=12, fontweight='bold')
        axes[0, 1].grid(True, alpha=0.3)
        axes[0, 1].legend(fontsize=9)

        # Plot 3: Mass conservation
        axes[1, 0].axhline(y=1e-8, color='r', linestyle='--', alpha=0.5, label='Target < 1e-8')
        axes[1, 0].set_xlabel('Time (s)', fontsize=11)
        axes[1, 0].set_ylabel('|ΔV/V₀|', fontsize=11)
        axes[1, 0].set_title('(c) Mass Conservation', fontsize=12, fontweight='bold')
        axes[1, 0].grid(True, alpha=0.3, which='both')
        axes[1, 0].legend(fontsize=9)

        # Plot 4: Dynamic contact angle
        axes[1, 1].set_xlabel('Time (s)', fontsize=11)
        axes[1, 1].set_ylabel('Contact Angle (degrees)', fontsize=11)
        axes[1, 1].set_title('(d) Dynamic Contact Angle', fontsize=12, fontweight='bold')
        axes[1, 1].grid(True, alpha=0.3)
        axes[1, 1].legend(fontsize=9)

        plt.suptitle('Droplet Impact Validation\n(Paper Section 3.3, Figure 7)',
                    fontsize=14, fontweight='bold', y=0.995)
        plt.tight_layout()

        if save:
            output_file = self.results_dir / "fig_droplet_impact.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"Saved: {output_file}")
        else:
            plt.show()

    def plot_beta_max_comparison(self, save=False):
        """
        Plot maximum spreading comparison with correlations
        Compare measured β_max with empirical correlations
        """
        # Load JSON summaries
        json_files = glob.glob(str(self.results_dir / "droplet_impact_*_summary.json"))

        if not json_files:
            print("No droplet impact summaries found")
            return

        # Data containers
        Re_values = []
        We_values = []
        theta_values = []
        beta_measured = []
        beta_pf = []
        beta_clanet = []

        for file in json_files:
            try:
                with open(file, 'r') as f:
                    data = json.load(f)
                    params = data.get('parameters', {})
                    metrics = data.get('metrics', {})

                    Re_values.append(params.get('reynolds', 0))
                    We_values.append(params.get('weber', 0))
                    theta_values.append(params.get('contact_angle', 0))
                    beta_measured.append(metrics.get('beta_max_measured', 0))
                    beta_pf.append(metrics.get('beta_max_pasandideh_fard', 0))
                    beta_clanet.append(metrics.get('beta_max_clanet', 0))
            except Exception as e:
                print(f"Error loading {file}: {e}")

        if not Re_values:
            print("No data loaded")
            return

        fig, axes = plt.subplots(1, 2, figsize=(14, 6))

        # Plot 1: Measured vs Pasandideh-Fard
        axes[0].scatter(beta_pf, beta_measured, s=100, alpha=0.7, edgecolors='k')
        for i, (Re, We) in enumerate(zip(Re_values, We_values)):
            axes[0].annotate(f"Re={Re:.0f}\nWe={We:.0f}",
                           (beta_pf[i], beta_measured[i]),
                           fontsize=8, ha='center')

        # Perfect agreement line
        beta_range = [min(min(beta_pf), min(beta_measured)),
                     max(max(beta_pf), max(beta_measured))]
        axes[0].plot(beta_range, beta_range, 'k--', alpha=0.5, label='Perfect agreement')

        # ±5% error bands
        axes[0].fill_between(beta_range,
                           [b*0.95 for b in beta_range],
                           [b*1.05 for b in beta_range],
                           alpha=0.2, color='green', label='±5% error')

        axes[0].set_xlabel('β_max (Pasandideh-Fard)', fontsize=12)
        axes[0].set_ylabel('β_max (Measured)', fontsize=12)
        axes[0].set_title('(a) Comparison with Pasandideh-Fard', fontsize=13, fontweight='bold')
        axes[0].grid(True, alpha=0.3)
        axes[0].legend(fontsize=10)
        axes[0].set_aspect('equal')

        # Plot 2: Measured vs Clanet
        axes[1].scatter(beta_clanet, beta_measured, s=100, alpha=0.7, edgecolors='k')
        for i, (Re, We) in enumerate(zip(Re_values, We_values)):
            axes[1].annotate(f"Re={Re:.0f}\nWe={We:.0f}",
                           (beta_clanet[i], beta_measured[i]),
                           fontsize=8, ha='center')

        beta_range2 = [min(min(beta_clanet), min(beta_measured)),
                      max(max(beta_clanet), max(beta_measured))]
        axes[1].plot(beta_range2, beta_range2, 'k--', alpha=0.5, label='Perfect agreement')
        axes[1].fill_between(beta_range2,
                           [b*0.95 for b in beta_range2],
                           [b*1.05 for b in beta_range2],
                           alpha=0.2, color='green', label='±5% error')

        axes[1].set_xlabel('β_max (Clanet)', fontsize=12)
        axes[1].set_ylabel('β_max (Measured)', fontsize=12)
        axes[1].set_title('(b) Comparison with Clanet', fontsize=13, fontweight='bold')
        axes[1].grid(True, alpha=0.3)
        axes[1].legend(fontsize=10)
        axes[1].set_aspect('equal')

        plt.suptitle('Maximum Spreading Factor Comparison\n(Paper Section 3.3)',
                    fontsize=14, fontweight='bold')
        plt.tight_layout()

        if save:
            output_file = self.results_dir / "fig_beta_max_comparison.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"Saved: {output_file}")
        else:
            plt.show()

    def plot_validation_summary(self, save=False):
        """Create validation summary dashboard"""
        # Load all summaries
        json_files = glob.glob(str(self.results_dir / "*_summary.json"))

        if not json_files:
            print("No summary files found")
            return

        summaries = []
        for file in json_files:
            try:
                with open(file, 'r') as f:
                    summary = json.load(f)
                    summaries.append(summary)
            except Exception as e:
                print(f"Error loading {file}: {e}")

        if not summaries:
            return

        fig = plt.figure(figsize=(14, 10))
        gs = fig.add_gridspec(2, 3, hspace=0.3, wspace=0.3)

        # Plot 1: Test status pie chart
        ax1 = fig.add_subplot(gs[0, 0])
        statuses = [s.get('status', 'Unknown') for s in summaries]
        status_counts = {}
        for status in statuses:
            status_counts[status] = status_counts.get(status, 0) + 1

        colors_map = {'PASS': '#2ecc71', 'FAIL': '#e74c3c', 'Unknown': '#95a5a6'}
        ax1.pie(status_counts.values(), labels=status_counts.keys(),
               colors=[colors_map.get(k, '#95a5a6') for k in status_counts.keys()],
               autopct='%1.1f%%', startangle=90, textprops={'fontsize': 11})
        ax1.set_title('Test Status', fontsize=13, fontweight='bold')

        # Plot 2: Test types
        ax2 = fig.add_subplot(gs[0, 1])
        test_types = [s.get('test_name', 'Unknown') for s in summaries]
        type_counts = {}
        for test_type in test_types:
            type_counts[test_type] = type_counts.get(test_type, 0) + 1

        ax2.bar(range(len(type_counts)), list(type_counts.values()), color='#3498db')
        ax2.set_xticks(range(len(type_counts)))
        ax2.set_xticklabels([t.replace('_', '\n') for t in type_counts.keys()],
                           rotation=0, ha='center', fontsize=9)
        ax2.set_ylabel('Count', fontsize=11)
        ax2.set_title('Tests by Type', fontsize=13, fontweight='bold')
        ax2.grid(True, alpha=0.3, axis='y')

        plt.suptitle('Paper Validation Summary\nTavares et al. (2024) arXiv:2402.10185',
                    fontsize=15, fontweight='bold')

        if save:
            output_file = self.results_dir / "fig_validation_summary.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"Saved: {output_file}")
        else:
            plt.show()


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description='Plot comparisons with paper data (arXiv:2402.10185)'
    )
    parser.add_argument('--results-dir', default='../results',
                       help='Results directory')
    parser.add_argument('--reference-dir', default='../reference_data',
                       help='Reference data directory')
    parser.add_argument('--static', action='store_true',
                       help='Plot static contact angle')
    parser.add_argument('--impact', action='store_true',
                       help='Plot droplet impact')
    parser.add_argument('--beta', action='store_true',
                       help='Plot beta_max comparison')
    parser.add_argument('--summary', action='store_true',
                       help='Plot validation summary')
    parser.add_argument('--all', action='store_true',
                       help='Plot all')
    parser.add_argument('--save', action='store_true',
                       help='Save plots to files')

    args = parser.parse_args()

    plotter = PaperPlotter(
        results_dir=args.results_dir,
        reference_dir=args.reference_dir
    )

    if args.all or args.static:
        plotter.plot_static_contact_angle(save=args.save)

    if args.all or args.impact:
        plotter.plot_droplet_impact(save=args.save)

    if args.all or args.beta:
        plotter.plot_beta_max_comparison(save=args.save)

    if args.all or args.summary:
        plotter.plot_validation_summary(save=args.save)

    if not (args.all or args.static or args.impact or args.beta or args.summary):
        # Default: plot all
        plotter.plot_static_contact_angle(save=args.save)
        plotter.plot_droplet_impact(save=args.save)
        plotter.plot_beta_max_comparison(save=args.save)
        plotter.plot_validation_summary(save=args.save)


if __name__ == '__main__':
    main()
