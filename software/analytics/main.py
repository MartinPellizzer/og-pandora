import csv
import numpy as np
from matplotlib import pyplot as plt 

with open('HISTORY.csv', newline='') as f:
    reader = csv.reader(f)
    data = list(reader)

# x = []
y = []
for i, d in enumerate(data):
    y.append(d[6])
    # x.append(d[6])

    # if i > 10: break

# x = [1,2,3,4,5,6,7,8,9]
x = np.arange(len(y))

# print(x)

# plt.yticks([1000, 2000, 3000])

plt.plot(x, y)
plt.show()