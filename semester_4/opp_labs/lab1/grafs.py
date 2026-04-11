import matplotlib.pyplot as plt

# данные
processes = [1, 2, 4, 8, 16]

type1 = [44.47848105, 29.36687613, 14.99037218, 11.15377808, 5.648139]
type2 = [45.1665551662, 27.1356611252, 13.0948638916, 10.0927309990, 5.3601350784]

# ускорение
speedup1 = [type1[0] / t for t in type1]
speedup2 = [type2[0] / t for t in type2]

# эффективность
eff1 = [s / p for s, p in zip(speedup1, processes)]
eff2 = [s / p for s, p in zip(speedup2, processes)]


# -------- график времени --------
plt.figure()
plt.plot(processes, type1, marker='o', label='type1')
plt.plot(processes, type2, marker='o', label='type2')
plt.xlabel("Processes")
plt.ylabel("Time")
plt.title("Execution Time")
plt.legend()
plt.grid(True)
plt.savefig("time.png")

# -------- график ускорения --------
plt.figure()
plt.plot(processes, speedup1, marker='o', label='type1')
plt.plot(processes, speedup2, marker='o', label='type2')
plt.plot(processes, processes, linestyle='--', label='ideal')
plt.xlabel("Processes")
plt.ylabel("Speedup")
plt.title("Speedup")
plt.legend()
plt.grid(True)
plt.savefig("speedup.png")


# -------- график эффективности --------
plt.figure()
plt.plot(processes, eff1, marker='o', label='type1')
plt.plot(processes, eff2, marker='o', label='type2')
plt.xlabel("Processes")
plt.ylabel("Efficiency")
plt.title("Efficiency")
plt.legend()
plt.grid(True)
plt.savefig("efficiency.png")
