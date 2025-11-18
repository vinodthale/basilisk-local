#!/usr/bin/env python3
"""
Analyze and visualize results from nondimensional drop vaporization tests.

Usage:
    python3 analyze_results.py [OPTIONS] RESULTS_DIR

Options:
    --compare       Compare multiple Re cases
    --plot-volume   Plot volume evolution
    --plot-nusselt  Plot Nusselt number evolution
    --save          Save plots to files
    --format FMT    Output format (png, pdf, svg; default: png)

Examples:
    # Analyze single case
    python3 analyze_results.py ../results/Re_100.00_We_1.50

    # Compare all cases
    python3 analyze_results.py --compare ../results

    # Plot volume evolution and save
    python3 analyze_results.py --plot-volume --save ../results/Re_100.00_We_1.50
"""

import os
import sys
import glob
import argparse
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

# Set matplotlib style
plt.style.use('seaborn-v0_8-paper' if 'seaborn-v0_8-paper' in plt.style.available else 'default')
plt.rcParams['figure.dpi'] = 100
plt.rcParams['font.size'] = 10


def load_volume_history(filepath):
    """Load volume history from file."""
    data = []
    with open(filepath, 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split()
            if len(parts) >= 4:
                try:
                    data.append([float(x) for x in parts[:4]])
                except ValueError:
                    continue

    if not data:
        return None

    data = np.array(data)
    return {
        't_plus': data[:, 0],
        't_star': data[:, 1],
        'volume': data[:, 2],
        'v_ratio': data[:, 3]
    }


def load_nusselt_history(filepath):
    """Load Nusselt number history from file."""
    data = []
    with open(filepath, 'r') as f:
        for line in f:
            if line.startswith('#'):
                continue
            parts = line.strip().split()
            if len(parts) >= 3:
                try:
                    data.append([float(x) for x in parts[:3]])
                except ValueError:
                    continue

    if not data:
        return None

    data = np.array(data)
    return {
        't_plus': data[:, 0],
        't_star': data[:, 1],
        'nusselt': data[:, 2]
    }


def extract_re_we_from_dirname(dirname):
    """Extract Re and We from directory name."""
    # Expected format: Re_XXX.XX_We_Y.YY
    parts = dirname.split('_')
    try:
        re_idx = parts.index('Re')
        we_idx = parts.index('We')
        re_value = float(parts[re_idx + 1])
        we_value = float(parts[we_idx + 1])
        return re_value, we_value
    except (ValueError, IndexError):
        return None, None


def plot_single_case_volume(volume_data, re_value, output_file=None):
    """Plot volume evolution for a single case."""
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 8), sharex=True)

    # Plot V/V0 vs t*
    ax1.plot(volume_data['t_star'], volume_data['v_ratio'], 'b-', linewidth=2)
    ax1.set_ylabel(r'$V/V_0$', fontsize=12)
    ax1.set_title(f'Drop Volume Evolution (Re = {re_value:.1f}, We = 1.5)', fontsize=14)
    ax1.grid(True, alpha=0.3)
    ax1.set_ylim([0, 1.1])

    # Plot vaporization rate: d(V/V0)/dt*
    if len(volume_data['t_star']) > 1:
        dt_star = np.diff(volume_data['t_star'])
        dV = np.diff(volume_data['v_ratio'])
        vap_rate = -dV / dt_star  # Negative because volume decreases
        t_star_mid = 0.5 * (volume_data['t_star'][:-1] + volume_data['t_star'][1:])

        ax2.plot(t_star_mid, vap_rate, 'r-', linewidth=2)
        ax2.set_xlabel(r'$t^*$', fontsize=12)
        ax2.set_ylabel(r'$-d(V/V_0)/dt^*$', fontsize=12)
        ax2.set_title('Vaporization Rate', fontsize=14)
        ax2.grid(True, alpha=0.3)
        ax2.set_ylim(bottom=0)

    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved plot to {output_file}")
    else:
        plt.show()

    plt.close()


def plot_comparison_volume(results_dict, output_file=None):
    """Plot volume evolution comparison for multiple Re values."""
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

    # Sort by Re
    re_values = sorted(results_dict.keys())

    # Color map
    colors = plt.cm.viridis(np.linspace(0, 1, len(re_values)))

    for idx, re_value in enumerate(re_values):
        volume_data = results_dict[re_value]['volume']
        if volume_data is None:
            continue

        label = f'Re = {re_value:.1f}'
        color = colors[idx]

        # Plot V/V0 vs t*
        ax1.plot(volume_data['t_star'], volume_data['v_ratio'],
                color=color, linewidth=1.5, label=label, alpha=0.8)

        # Plot vaporization rate
        if len(volume_data['t_star']) > 1:
            dt_star = np.diff(volume_data['t_star'])
            dV = np.diff(volume_data['v_ratio'])
            vap_rate = -dV / dt_star
            t_star_mid = 0.5 * (volume_data['t_star'][:-1] + volume_data['t_star'][1:])

            ax2.plot(t_star_mid, vap_rate, color=color, linewidth=1.5,
                    label=label, alpha=0.8)

    # Format axes
    ax1.set_xlabel(r'$t^*$', fontsize=12)
    ax1.set_ylabel(r'$V/V_0$', fontsize=12)
    ax1.set_title('Drop Volume Evolution\n(We = 1.5, varying Re)', fontsize=14)
    ax1.grid(True, alpha=0.3)
    ax1.legend(fontsize=8, ncol=2, loc='upper right')
    ax1.set_ylim([0, 1.1])

    ax2.set_xlabel(r'$t^*$', fontsize=12)
    ax2.set_ylabel(r'$-d(V/V_0)/dt^*$', fontsize=12)
    ax2.set_title('Vaporization Rate\n(We = 1.5, varying Re)', fontsize=14)
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=8, ncol=2, loc='upper right')
    ax2.set_ylim(bottom=0)

    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved comparison plot to {output_file}")
    else:
        plt.show()

    plt.close()


def plot_re_scaling(results_dict, output_file=None):
    """Plot scaling with Reynolds number."""
    re_values = []
    vap_rates = []
    final_volumes = []

    for re_value in sorted(results_dict.keys()):
        volume_data = results_dict[re_value]['volume']
        if volume_data is None:
            continue

        re_values.append(re_value)

        # Final volume
        final_volumes.append(volume_data['v_ratio'][-1])

        # Average vaporization rate
        if len(volume_data['t_star']) > 1:
            dt_star = np.diff(volume_data['t_star'])
            dV = np.diff(volume_data['v_ratio'])
            vap_rate = -dV / dt_star
            avg_vap_rate = np.mean(vap_rate)
            vap_rates.append(avg_vap_rate)
        else:
            vap_rates.append(0)

    re_values = np.array(re_values)
    vap_rates = np.array(vap_rates)
    final_volumes = np.array(final_volumes)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Plot average vaporization rate vs Re
    ax1.plot(re_values, vap_rates, 'bo-', markersize=6, linewidth=2)
    ax1.set_xlabel('Reynolds Number', fontsize=12)
    ax1.set_ylabel(r'Average $-d(V/V_0)/dt^*$', fontsize=12)
    ax1.set_title('Vaporization Rate vs Reynolds Number', fontsize=14)
    ax1.grid(True, alpha=0.3)

    # Plot final volume vs Re
    ax2.plot(re_values, final_volumes, 'ro-', markersize=6, linewidth=2)
    ax2.set_xlabel('Reynolds Number', fontsize=12)
    ax2.set_ylabel(r'Final $V/V_0$ at $t^* = 0.16$', fontsize=12)
    ax2.set_title('Final Volume vs Reynolds Number', fontsize=14)
    ax2.grid(True, alpha=0.3)
    ax2.set_ylim([0, 1.0])

    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved Re scaling plot to {output_file}")
    else:
        plt.show()

    plt.close()


def analyze_single_case(case_dir, args):
    """Analyze a single case."""
    case_name = os.path.basename(case_dir)
    re_value, we_value = extract_re_we_from_dirname(case_name)

    print(f"\n{'='*60}")
    print(f"Analyzing: {case_name}")
    print(f"{'='*60}\n")

    if re_value is None:
        print("Warning: Could not extract Re and We from directory name")
        re_value = 0
    else:
        print(f"Parameters: Re = {re_value:.2f}, We = {we_value:.2f}")

    # Load volume history
    volume_file = os.path.join(case_dir, 'liquid_volume_history.txt')
    if os.path.exists(volume_file):
        volume_data = load_volume_history(volume_file)
        if volume_data is not None:
            print(f"\nVolume History:")
            print(f"  Initial V/V0: {volume_data['v_ratio'][0]:.6f}")
            print(f"  Final V/V0:   {volume_data['v_ratio'][-1]:.6f}")
            print(f"  Total loss:   {(1 - volume_data['v_ratio'][-1])*100:.2f}%")
            print(f"  Final t*:     {volume_data['t_star'][-1]:.4f}")

            if args.plot_volume or args.save:
                output_file = None
                if args.save:
                    output_file = os.path.join(case_dir, f'volume_evolution.{args.format}')
                plot_single_case_volume(volume_data, re_value, output_file)
        else:
            print("Warning: Volume history file is empty or invalid")
    else:
        print(f"Warning: Volume history file not found: {volume_file}")

    print()


def analyze_comparison(results_dir, args):
    """Analyze and compare multiple cases."""
    # Find all case directories
    case_dirs = glob.glob(os.path.join(results_dir, 'Re_*_We_*'))

    if not case_dirs:
        print(f"Error: No case directories found in {results_dir}")
        return

    print(f"\n{'='*60}")
    print(f"Comparing {len(case_dirs)} cases from {results_dir}")
    print(f"{'='*60}\n")

    # Load all results
    results_dict = {}

    for case_dir in sorted(case_dirs):
        case_name = os.path.basename(case_dir)
        re_value, we_value = extract_re_we_from_dirname(case_name)

        if re_value is None:
            continue

        print(f"Loading: {case_name} (Re = {re_value:.1f})")

        # Load volume data
        volume_file = os.path.join(case_dir, 'liquid_volume_history.txt')
        volume_data = None
        if os.path.exists(volume_file):
            volume_data = load_volume_history(volume_file)

        # Load Nusselt data
        nusselt_file = os.path.join(case_dir, 'nusselt_history.txt')
        nusselt_data = None
        if os.path.exists(nusselt_file):
            nusselt_data = load_nusselt_history(nusselt_file)

        results_dict[re_value] = {
            'volume': volume_data,
            'nusselt': nusselt_data,
            'we': we_value
        }

    print(f"\nSuccessfully loaded {len(results_dict)} cases")
    print()

    # Generate comparison plots
    if results_dict:
        output_file = None

        # Volume comparison
        if args.save:
            output_file = os.path.join(results_dir, f'volume_comparison.{args.format}')
        plot_comparison_volume(results_dict, output_file)

        # Re scaling
        if args.save:
            output_file = os.path.join(results_dir, f're_scaling.{args.format}')
        plot_re_scaling(results_dict, output_file)


def main():
    parser = argparse.ArgumentParser(
        description='Analyze nondimensional drop vaporization test results',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )

    parser.add_argument('results_dir', help='Results directory')
    parser.add_argument('--compare', action='store_true',
                       help='Compare multiple Re cases')
    parser.add_argument('--plot-volume', action='store_true',
                       help='Plot volume evolution')
    parser.add_argument('--plot-nusselt', action='store_true',
                       help='Plot Nusselt number evolution')
    parser.add_argument('--save', action='store_true',
                       help='Save plots to files')
    parser.add_argument('--format', default='png',
                       choices=['png', 'pdf', 'svg'],
                       help='Output format for saved plots (default: png)')

    args = parser.parse_args()

    # Check if results directory exists
    if not os.path.isdir(args.results_dir):
        print(f"Error: Results directory not found: {args.results_dir}")
        sys.exit(1)

    # Analyze
    if args.compare:
        analyze_comparison(args.results_dir, args)
    else:
        analyze_single_case(args.results_dir, args)


if __name__ == '__main__':
    main()
