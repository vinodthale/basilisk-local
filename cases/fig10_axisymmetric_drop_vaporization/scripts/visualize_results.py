#!/usr/bin/env python3
"""
Visualization script for Figure 10 axisymmetric drop vaporization results

This script creates various plots to analyze the simulation results:
1. Drop shape evolution (similar to Figure 10)
2. Temperature contours around the drop
3. Volume history over time
4. Vaporization rate analysis
5. Comparison across different Reynolds numbers
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import patches
import os
import sys
import glob

# Configure matplotlib for publication-quality figures
plt.rcParams['font.size'] = 12
plt.rcParams['axes.labelsize'] = 14
plt.rcParams['axes.titlesize'] = 14
plt.rcParams['legend.fontsize'] = 11
plt.rcParams['figure.dpi'] = 150


class DropVaporizationAnalyzer:
    """Analyzer for drop vaporization simulation results"""

    def __init__(self, results_dir):
        """
        Initialize analyzer

        Parameters
        ----------
        results_dir : str
            Directory containing simulation results
        """
        self.results_dir = results_dir
        self.Re = None
        self.We = None
        self.volume_data = None

        # Extract Re and We from directory name
        self._extract_parameters()

        # Load data
        self._load_volume_history()

    def _extract_parameters(self):
        """Extract Reynolds and Weber numbers from directory name"""
        dirname = os.path.basename(self.results_dir)

        # Format: Re_100.00_We_1.50
        parts = dirname.split('_')
        if len(parts) >= 4:
            try:
                self.Re = float(parts[1])
                self.We = float(parts[3])
            except:
                print(f"Warning: Could not extract parameters from {dirname}")

    def _load_volume_history(self):
        """Load volume history data"""
        volume_file = os.path.join(self.results_dir, 'volume_history.txt')

        if not os.path.exists(volume_file):
            print(f"Warning: Volume history file not found: {volume_file}")
            return

        try:
            self.volume_data = np.loadtxt(volume_file, skiprows=1)
        except Exception as e:
            print(f"Error loading volume history: {e}")

    def plot_volume_history(self, ax=None, label=None):
        """
        Plot volume history

        Parameters
        ----------
        ax : matplotlib.axes.Axes, optional
            Axes to plot on. If None, creates new figure.
        label : str, optional
            Label for the plot
        """
        if self.volume_data is None:
            print("No volume data available")
            return

        if ax is None:
            fig, ax = plt.subplots(figsize=(8, 6))
            standalone = True
        else:
            standalone = False

        # Extract data: t [s], t* [-], Volume [D0^3], V/V0 [-]
        t_star = self.volume_data[:, 1]
        V_ratio = self.volume_data[:, 3]

        if label is None:
            label = f'Re = {self.Re:.1f}'

        ax.plot(t_star, V_ratio, '-', label=label, linewidth=2)

        if standalone:
            ax.set_xlabel('Dimensionless time t* = t·U∞/D₀·√(ρₗ/ρₐ)')
            ax.set_ylabel('Volume ratio V/V₀')
            ax.set_title(f'Drop Volume Evolution (Re = {self.Re:.1f}, We = {self.We:.1f})')
            ax.grid(True, alpha=0.3)
            ax.legend()
            plt.tight_layout()
            plt.show()

    def compute_vaporization_rate(self):
        """
        Compute vaporization rate from volume history

        Returns
        -------
        t_star : ndarray
            Dimensionless time
        vap_rate : ndarray
            Vaporization rate dV/dt*
        """
        if self.volume_data is None:
            return None, None

        t_star = self.volume_data[:, 1]
        V = self.volume_data[:, 2]

        # Compute derivative dV/dt*
        vap_rate = -np.gradient(V, t_star)  # Negative because volume decreases

        return t_star, vap_rate

    def plot_vaporization_rate(self, ax=None, label=None):
        """Plot vaporization rate"""
        t_star, vap_rate = self.compute_vaporization_rate()

        if t_star is None:
            print("No data available")
            return

        if ax is None:
            fig, ax = plt.subplots(figsize=(8, 6))
            standalone = True
        else:
            standalone = False

        if label is None:
            label = f'Re = {self.Re:.1f}'

        ax.plot(t_star, vap_rate, '-', label=label, linewidth=2)

        if standalone:
            ax.set_xlabel('Dimensionless time t*')
            ax.set_ylabel('Vaporization rate -dV/dt* [D₀³/t*]')
            ax.set_title(f'Vaporization Rate (Re = {self.Re:.1f}, We = {self.We:.1f})')
            ax.grid(True, alpha=0.3)
            ax.legend()
            plt.tight_layout()
            plt.show()


def compare_reynolds_sweep(results_base_dir, We=1.5):
    """
    Compare results across different Reynolds numbers

    Parameters
    ----------
    results_base_dir : str
        Base directory containing results for different Re
    We : float
        Weber number to filter results
    """
    # Find all result directories
    pattern = os.path.join(results_base_dir, f'Re_*_We_{We:.2f}')
    result_dirs = sorted(glob.glob(pattern))

    if not result_dirs:
        print(f"No results found matching pattern: {pattern}")
        return

    print(f"Found {len(result_dirs)} result directories")

    analyzers = []
    for result_dir in result_dirs:
        analyzer = DropVaporizationAnalyzer(result_dir)
        if analyzer.volume_data is not None:
            analyzers.append(analyzer)

    if not analyzers:
        print("No valid data found")
        return

    # Create comparison plots
    fig, axes = plt.subplots(2, 1, figsize=(10, 10))

    # Sort by Reynolds number
    analyzers.sort(key=lambda a: a.Re)

    # Plot volume history
    ax = axes[0]
    for analyzer in analyzers:
        analyzer.plot_volume_history(ax=ax)

    ax.set_xlabel('Dimensionless time t*')
    ax.set_ylabel('Volume ratio V/V₀')
    ax.set_title(f'Drop Volume Evolution (We = {We})')
    ax.grid(True, alpha=0.3)
    ax.legend(ncol=2)

    # Plot vaporization rate
    ax = axes[1]
    for analyzer in analyzers:
        analyzer.plot_vaporization_rate(ax=ax)

    ax.set_xlabel('Dimensionless time t*')
    ax.set_ylabel('Vaporization rate -dV/dt*')
    ax.set_title(f'Vaporization Rate (We = {We})')
    ax.grid(True, alpha=0.3)
    ax.legend(ncol=2)

    plt.tight_layout()

    # Save figure
    output_file = os.path.join(results_base_dir, f'comparison_We_{We:.2f}.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Saved comparison plot: {output_file}")

    plt.show()


def plot_single_case(results_dir):
    """
    Create detailed plots for a single case

    Parameters
    ----------
    results_dir : str
        Directory containing results
    """
    analyzer = DropVaporizationAnalyzer(results_dir)

    if analyzer.volume_data is None:
        print("No data available")
        return

    fig, axes = plt.subplots(2, 1, figsize=(10, 10))

    # Volume history
    analyzer.plot_volume_history(ax=axes[0])
    axes[0].set_xlabel('Dimensionless time t*')
    axes[0].set_ylabel('Volume ratio V/V₀')
    axes[0].set_title(f'Drop Volume Evolution (Re = {analyzer.Re:.1f}, We = {analyzer.We:.1f})')
    axes[0].grid(True, alpha=0.3)

    # Vaporization rate
    analyzer.plot_vaporization_rate(ax=axes[1])
    axes[1].set_xlabel('Dimensionless time t*')
    axes[1].set_ylabel('Vaporization rate -dV/dt*')
    axes[1].set_title(f'Vaporization Rate (Re = {analyzer.Re:.1f}, We = {analyzer.We:.1f})')
    axes[1].grid(True, alpha=0.3)

    plt.tight_layout()

    # Save
    output_file = os.path.join(results_dir, 'analysis.png')
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Saved analysis plot: {output_file}")

    plt.show()


def main():
    """Main function"""
    import argparse

    parser = argparse.ArgumentParser(
        description='Visualize axisymmetric drop vaporization results'
    )
    parser.add_argument(
        'results_dir',
        help='Results directory (single case) or base directory (sweep comparison)'
    )
    parser.add_argument(
        '--compare',
        action='store_true',
        help='Compare multiple Reynolds numbers'
    )
    parser.add_argument(
        '--We',
        type=float,
        default=1.5,
        help='Weber number for comparison (default: 1.5)'
    )

    args = parser.parse_args()

    if not os.path.exists(args.results_dir):
        print(f"Error: Directory not found: {args.results_dir}")
        sys.exit(1)

    if args.compare:
        compare_reynolds_sweep(args.results_dir, We=args.We)
    else:
        plot_single_case(args.results_dir)


if __name__ == '__main__':
    main()
