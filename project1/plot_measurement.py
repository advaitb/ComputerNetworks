import numpy as np
import matplotlib.pyplot as plt

def read_file(filename):
    with open(filename, 'r') as file:
        timings = []
        for line in file:
            time = line.split(",")
            time = time[:-1]
            timings.append([float(t) for t in time])
    # return np.array([float(t) for t in time])
    return np.average(np.array(timings), axis=0)

timings = read_file("./test.txt")
# print(timings)
# print(len(timings))

plt.figure()
x = np.linspace(1, len(timings), len(timings)) + 10
print(x)
print(np.shape(x), np.shape(timings))
plt.plot(x, timings)
plt.axis([0, len(timings) + 20, 0, 400])
plt.show()