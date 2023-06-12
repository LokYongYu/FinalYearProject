import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

df = pd.read_csv('PLotthisshit.csv', header=None)
x = df.iloc[:, 0].values
y = df.iloc[:, 1].values
y1 = df.iloc[:, 2].values
y2 = df.iloc[:, 3].values
uncetain = df.iloc[:, 4].values
mean_uncertain = np.mean(uncetain)
std_dev_un = np.std(uncetain)
min_uncertainty = np.min(uncetain)
max_uncertainty = np.max(uncetain)
print(mean_uncertain)
print(std_dev_un)
print(min_uncertainty)
print(max_uncertainty)
plt.plot(x, y1, color='blue', label='RF')
plt.plot(x, x,'r--', label='LO')

plt.errorbar(x, y1, yerr=uncetain, fmt='o', color='black', ecolor='gray', capsize=3, label='Uncertainty')

# Set axis labels and legend
plt.title("Uncertainty error bar of RF vs LO")
plt.ylabel('Up-converted Frequency, RF(Hz)')
plt.xlabel('Local Oscillator, LO(Hz)')
plt.legend()

# Show the plot
plt.show()