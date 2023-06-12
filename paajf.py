import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from scipy.signal import find_peaks
from decimal import Decimal

# Load the .csv file into a pandas dataframe
df = pd.read_csv('Up1.csv', header=None)

# Extract the columns as separate arrays
x = df.iloc[:, 0].values
y = df.iloc[:, 1].values
x = x / 1e6
formatted_y = np.vectorize(lambda x: format(Decimal(x), '.5f'))(y)
print(formatted_y)

# Find the indices and properties of all peaks in the y array
peaks, properties = find_peaks(y, prominence=10)  # Adjust the prominence threshold as needed

# Get the corresponding peak frequencies from the x array
peak_frequencies = x[peaks]

colors = ['red', 'black', 'green', 'orange']
# Print the peak frequencies
print("Peak frequencies:")
for i, freq in enumerate(peak_frequencies):
    print(freq)

    # Add the peak frequencies as legends and labels on the plot
    plt.text(freq, y[peaks[i]], f'Peak {i + 1}', ha='center', va='bottom', color=colors[i])
    plt.text(7, 12 - i, f'Peak {i + 1}: {freq:.2f} MHz', ha='left', va='top', color=colors[i])

# Plot the data as a line plot
plt.text(1000, -20, 'Peak1 = 100.30MHz', fontsize=10, ha='center', va='bottom',color='red')
plt.text(1000, -25, 'Peak2 = 8467.70MHz', fontsize=10, ha='center', va='bottom',color='black')
plt.text(1000, -30, 'Peak3 = 8567.99MHz', fontsize=10, ha='center', va='bottom',color='green')
plt.text(1000, -35, 'Peak4 = 8668.30MHz', fontsize=10, ha='center', va='bottom',color='orange')
plt.title("Up Conversion Full Span")
plt.ylabel('Amplitude, dBm')
plt.xlabel('Frequency, MHz')
plt.plot(x, y)
plt.show()
