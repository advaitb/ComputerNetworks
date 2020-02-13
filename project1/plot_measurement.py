import numpy as np
import matplotlib.pyplot as plt

def read_file(filename):
    with open(filename, 'r') as file:
        timings = []
        for line in file:
            time = line.split(",")
            time = time[:-1]
            print(len(time))
            timings.append([float(t) for t in time])
    # return np.array([float(t) for t in time])
    return np.average(np.array(timings), axis=0)

timings = read_file("./test.txt")
# print(timings[490:491])


# plt.figure()
# x = np.linspace(1, len(timings), len(timings))*10 + 10
# # print(x)
# print(np.shape(x), np.shape(timings))
# print(timings)
# plt.plot(x, timings)
# plt.axis([0, len(timings)*10 + 20, 0, 10])
# plt.show()
plt.figure()
x = np.linspace(10, 65535, len(timings))
# print(x)
print(np.shape(x), np.shape(timings))
# for t in timings:
#     if t > 6000 or t < 3000:
#         print(t)
plt.plot(x, timings)
plt.axis([0, 65535, 0, 0.7])
plt.show()
