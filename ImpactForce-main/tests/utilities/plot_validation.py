#!/usr/bin/env python3
"""
Visualization Tools for Sharp VOF Test Suite
Creates plots for validation metrics
"""

import numpy as np
import matplotlib.pyplot as plt
import glob
import os
import sys
from pathlib import Path

def plot_mass_conservation(results_dir="../results", save=False):
    """Plot mass conservation results"""
    files = glob.glob(f"{results_dir}/mass_conservation_*.txt")

    if not files:
        print("No mass conservation results found")
        return

    fig, axes = plt.subplots(2, 1, figsize=(10, 8))

    for file in files:
        try:
            data = np.loadtxt(file, comments='#')
            if data.shape[1] < 5:
                continue

            time = data[:, 0]
            volume_ratio = data[:, 2]
            rel_error = data[:, 4]

            label = Path(file).stem.replace('mass_conservation_', '')

            # Plot volume ratio
            axes[0].plot(time, volume_ratio, label=label, linewidth=2)

            # Plot relative error
            axes[1].semilogy(time, np.abs(rel_error), label=label, linewidth=2)

        except Exception as e:
            print(f"Error plotting {file}: {e}")

    # Volume ratio plot
    axes[0].axhline(y=1.0, color='k', linestyle='--', alpha=0.5, label='Perfect conservation')
    axes[0].set_xlabel('Time', fontsize=12)
    axes[0].set_ylabel('Volume Ratio V(t)/V(0)', fontsize=12)
    axes[0].set_title('Mass Conservation: Volume Evolution', fontsize=14, fontweight='bold')
    axes[0].grid(True, alpha=0.3)
    axes[0].legend(fontsize=10)

    # Error plot
    axes[1].axhline(y=1e-10, color='g', linestyle='--', alpha=0.5, label='Sharp VOF tolerance')
    axes[1].axhline(y=1e-8, color='orange', linestyle='--', alpha=0.5, label='Standard VOF tolerance')
    axes[1].set_xlabel('Time', fontsize=12)
    axes[1].set_ylabel('Relative Error |ΔV/V₀|', fontsize=12)
    axes[1].set_title('Mass Conservation: Relative Error', fontsize=14, fontweight='bold')
    axes[1].grid(True, alpha=0.3, which='both')
    axes[1].legend(fontsize=10)

    plt.tight_layout()

    if save:
        output_file = f"{results_dir}/mass_conservation_plot.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved: {output_file}")
    else:
        plt.show()

def plot_contact_angle(results_dir="../results", save=False):
    """Plot contact angle results"""
    files = glob.glob(f"{results_dir}/contact_angle_*_deg.txt")

    if not files:
        print("No contact angle results found")
        return

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    for file in files:
        try:
            data = np.loadtxt(file, comments='#')
            if data.shape[1] < 2:
                continue

            time = data[:, 0]
            measured_angle = data[:, 1]

            # Extract specified angle from filename
            import re
            match = re.search(r'(\d+)_deg', file)
            if match:
                specified_angle = float(match.group(1))
            else:
                specified_angle = 90.0

            label = f"θ = {specified_angle:.0f}°"

            # Plot angle evolution
            axes[0].plot(time, measured_angle, label=label, linewidth=2)
            axes[0].axhline(y=specified_angle, color='gray',
                          linestyle='--', alpha=0.3)

            # Plot error
            error = measured_angle - specified_angle
            axes[1].plot(time, error, label=label, linewidth=2)

        except Exception as e:
            print(f"Error plotting {file}: {e}")

    # Angle evolution plot
    axes[0].set_xlabel('Time', fontsize=12)
    axes[0].set_ylabel('Contact Angle (degrees)', fontsize=12)
    axes[0].set_title('Contact Angle Evolution', fontsize=14, fontweight='bold')
    axes[0].grid(True, alpha=0.3)
    axes[0].legend(fontsize=10)

    # Error plot
    axes[1].axhline(y=5, color='g', linestyle='--', alpha=0.5, label='Pass tolerance')
    axes[1].axhline(y=-5, color='g', linestyle='--', alpha=0.5)
    axes[1].axhline(y=0, color='k', linestyle='-', alpha=0.3)
    axes[1].set_xlabel('Time', fontsize=12)
    axes[1].set_ylabel('Error (degrees)', fontsize=12)
    axes[1].set_title('Contact Angle Error', fontsize=14, fontweight='bold')
    axes[1].grid(True, alpha=0.3)
    axes[1].legend(fontsize=10)

    plt.tight_layout()

    if save:
        output_file = f"{results_dir}/contact_angle_plot.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved: {output_file}")
    else:
        plt.show()

def plot_summary(results_dir="../results", save=False):
    """Create summary dashboard"""
    import json

    # Load all summary JSONs
    json_files = glob.glob(f"{results_dir}/*_summary.json")

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

    # Create summary plots
    fig = plt.figure(figsize=(14, 10))
    gs = fig.add_gridspec(3, 2, hspace=0.3, wspace=0.3)

    # Test status pie chart
    ax1 = fig.add_subplot(gs[0, 0])
    statuses = [s.get('status', 'Unknown') for s in summaries]
    status_counts = {}
    for status in statuses:
        status_counts[status] = status_counts.get(status, 0) + 1

    colors = {'PASS': 'green', 'FAIL': 'red', 'Unknown': 'gray'}
    ax1.pie(status_counts.values(), labels=status_counts.keys(),
           colors=[colors.get(k, 'gray') for k in status_counts.keys()],
           autopct='%1.1f%%', startangle=90)
    ax1.set_title('Test Status', fontsize=14, fontweight='bold')

    # Test types bar chart
    ax2 = fig.add_subplot(gs[0, 1])
    test_types = [s.get('test_name', 'Unknown') for s in summaries]
    type_counts = {}
    for test_type in test_types:
        type_counts[test_type] = type_counts.get(test_type, 0) + 1

    ax2.bar(range(len(type_counts)), list(type_counts.values()))
    ax2.set_xticks(range(len(type_counts)))
    ax2.set_xticklabels(list(type_counts.keys()), rotation=45, ha='right')
    ax2.set_ylabel('Count', fontsize=12)
    ax2.set_title('Tests by Type', fontsize=14, fontweight='bold')
    ax2.grid(True, alpha=0.3, axis='y')

    # Add more plots as needed...

    plt.suptitle('Sharp VOF Validation Summary', fontsize=16, fontweight='bold')

    if save:
        output_file = f"{results_dir}/validation_summary.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"Saved: {output_file}")
    else:
        plt.show()

def main():
    import argparse

    parser = argparse.ArgumentParser(description='Plot Sharp VOF validation results')
    parser.add_argument('--results-dir', default='../results',
                       help='Results directory')
    parser.add_argument('--mass', action='store_true',
                       help='Plot mass conservation')
    parser.add_argument('--angle', action='store_true',
                       help='Plot contact angle')
    parser.add_argument('--summary', action='store_true',
                       help='Plot summary dashboard')
    parser.add_argument('--all', action='store_true',
                       help='Plot all')
    parser.add_argument('--save', action='store_true',
                       help='Save plots to files')

    args = parser.parse_args()

    if args.all or args.mass:
        plot_mass_conservation(results_dir=args.results_dir, save=args.save)

    if args.all or args.angle:
        plot_contact_angle(results_dir=args.results_dir, save=args.save)

    if args.all or args.summary:
        plot_summary(results_dir=args.results_dir, save=args.save)

    if not (args.all or args.mass or args.angle or args.summary):
        # Default: plot all
        plot_mass_conservation(results_dir=args.results_dir, save=args.save)
        plot_contact_angle(results_dir=args.results_dir, save=args.save)

if __name__ == '__main__':
    main()
