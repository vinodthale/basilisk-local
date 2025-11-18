# Reference Data for Validation

This directory contains reference data for validating the nondimensional drop vaporization test results.

## Expected Reference Data

### From Boyd & Ling 2023 (Figure 10)

The following reference data should be extracted from the paper:

1. **Volume evolution**: V/V₀ vs t* for various Re
2. **Drop shape**: Interface position at different times
3. **Temperature contours**: T* field visualization
4. **Nusselt number**: Nu vs Re correlation

### Data Format

Reference data files should be in plain text format:

```
# Re_XXX_volume_reference.txt
# Columns: t* | V/V0
0.00  1.000
0.02  0.990
0.04  0.975
...
```

### Usage

Place reference data files in this directory to enable comparison in the analysis scripts:

```bash
python3 ../utilities/analyze_results.py --compare-reference ../reference_data
```

## Data Sources

If you have access to the original paper data, please add reference files here with appropriate citations.

### Citation

> Boyd, B., & Ling, Y. (2023). Direct numerical simulation of evaporating droplets based on a sharp-interface algebraic VOF approach. *Computers & Fluids*, Figure 10, Sections 5.1 and Table 2.
