import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from scipy.signal import find_peaks
from decimal import Decimal
# Load the .csv file into a pandas dataframe
df = pd.read_csv('9.0GHz.csv', header=None)

# Extract the columns as separate arrays
x = df.iloc[:, 0].values
y = df.iloc[:, 1].values
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
    plt.text(freq, y[peaks[i]], f'Peak {i + 1}:{freq:.2f}', ha='center', va='bottom', color=colors[i])


# # Plot the data as a line plot
plt.title("Up Conversion Full Span")
plt.ylabel('Amplitude, dBm')
plt.xlabel('Frequency, MHz')
plt.plot(x, y)
plt.show()