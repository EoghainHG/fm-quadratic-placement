import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LinearSegmentedColormap
import seaborn as sns

# Read the CSV file
df = pd.read_csv('placement_result1.csv')

# Convert the data types if needed
df['X'] = pd.to_numeric(df['X'], errors='coerce')
df['Y'] = pd.to_numeric(df['Y'], errors='coerce')
df['Width'] = pd.to_numeric(df['Width'], errors='coerce')
df['Height'] = pd.to_numeric(df['Height'], errors='coerce')
df['IsFixed'] = pd.to_numeric(df['IsFixed'], errors='coerce')

# Create a figure with two subplots
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 10))

# SUBPLOT 1: SCATTER PLOT
# -----------------------
# Calculate point size based on Width and Height
size_factor = 500  # Adjust based on your data scale
df['PointSize'] = df['Width'] * df['Height'] * size_factor

# Plot fixed points (IsFixed = 1) in red
fixed = df[df['IsFixed'] == 1]
ax1.scatter(fixed['X'], fixed['Y'],
            s=fixed['PointSize'],
            color='red',
            alpha=0.6,
            label='Fixed')

# Plot non-fixed points (IsFixed = 0) in blue
non_fixed = df[df['IsFixed'] == 0]
ax1.scatter(non_fixed['X'], non_fixed['Y'],
            s=non_fixed['PointSize'],
            color='blue',
            alpha=0.6,
            label='Non-Fixed')

# Add labels and title for scatter plot
ax1.set_xlabel('X Position')
ax1.set_ylabel('Y Position')
ax1.set_title('Cell Placement Scatter Plot')
ax1.legend()
ax1.grid(True, linestyle='--', alpha=0.7)

# SUBPLOT 2: HEATMAP / DENSITY PLOT
# ---------------------------------
# Calculate the bounds of the area
x_min, x_max = df['X'].min(), df['X'].max()
y_min, y_max = df['Y'].min(), df['Y'].max()

# Add a small margin to the bounds
margin = 0.05
x_range = x_max - x_min
y_range = y_max - y_min
x_min -= margin * x_range
x_max += margin * x_range
y_min -= margin * y_range
y_max += margin * y_range

# Create a grid for the heatmap
grid_size = 100  # Adjust for more/less granularity
x_grid = np.linspace(x_min, x_max, grid_size)
y_grid = np.linspace(y_min, y_max, grid_size)
grid_x, grid_y = np.meshgrid(x_grid, y_grid)

# Calculate cell area for each point
df['Area'] = df['Width'] * df['Height']

# Initialize an empty grid for the heatmap
heatmap_grid = np.zeros((grid_size, grid_size))

# For each cell, add its area contribution to nearby grid points
# This is a simplified approach - for more accuracy you might use scipy.interpolate
for _, cell in df.iterrows():
    # Find the nearest grid point
    i = np.argmin(np.abs(x_grid - cell['X']))
    j = np.argmin(np.abs(y_grid - cell['Y']))

    # Add the cell's area to the grid
    # Using a "spread" to simulate cell dimensions
    spread = max(1, int(min(grid_size / 20, max(cell['Width'], cell['Height']) * grid_size * 5)))
    for di in range(-spread, spread + 1):
        for dj in range(-spread, spread + 1):
            if 0 <= i + di < grid_size and 0 <= j + dj < grid_size:
                # Weight by distance from center and cell area
                weight = cell['Area'] * np.exp(-(di ** 2 + dj ** 2) / (2 * (spread / 2) ** 2))
                heatmap_grid[j + dj, i + di] += weight

# Create a custom colormap from white to dark blue
colors = [(1, 1, 1), (0, 0, 0.8)]  # White to dark blue
cmap = LinearSegmentedColormap.from_list('white_to_blue', colors, N=100)

# Plot the heatmap
im = ax2.imshow(heatmap_grid, extent=[x_min, x_max, y_min, y_max],
                origin='lower', cmap=cmap, aspect='auto')

# Add colorbar
cbar = fig.colorbar(im, ax=ax2)
cbar.set_label('Utilization Density')

# Add labels and title for heatmap
ax2.set_xlabel('X Position')
ax2.set_ylabel('Y Position')
ax2.set_title('Area Utilization Heatmap')

# Overall figure adjustments
plt.tight_layout()
plt.savefig('cell_placement_analysis.png', dpi=300)
plt.show()

# Print some statistics
print(f"Total cells: {len(df)}")
print(f"Fixed cells: {len(fixed)} ({len(fixed) / len(df) * 100:.1f}%)")
print(f"Non-fixed cells: {len(non_fixed)} ({len(non_fixed) / len(df) * 100:.1f}%)")
print(f"Total area utilized: {df['Area'].sum()}")
print(f"Average cell area: {df['Area'].mean()}")
