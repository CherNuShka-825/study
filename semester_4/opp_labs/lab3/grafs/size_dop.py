import matplotlib.pyplot as plt

# Данные
sizes = [2880, 3024, 3168, 3312, 3456]
processes = [1, 4, 9, 16]

times = {
    1:  [26.133099, 30.183149, 34.706878, 39.603751, 45.048641],
    4:  [7.400071, 8.560792, 9.803039, 11.178689, 12.661698],
    9:  [5.229786, 6.056277, 6.948816, 7.939239, 9.029349],
    16: [3.087555, 3.565730, 4.085924, 4.659578, 5.299062]
}

# -----------------------------------
# УСКОРЕНИЕ
# S(p) = T1 / Tp
# -----------------------------------
speedup = {}

for p in processes:
    speedup[p] = []
    for i in range(len(sizes)):
        s = times[1][i] / times[p][i]
        speedup[p].append(s)

# -----------------------------------
# ЭФФЕКТИВНОСТЬ
# E(p) = S(p) / p
# -----------------------------------
efficiency = {}

for p in processes:
    efficiency[p] = []
    for s in speedup[p]:
        efficiency[p].append(s / p)

# -----------------------------------
# График ускорения
# -----------------------------------
plt.figure(figsize=(10,6))

for p in processes:
    plt.plot(sizes, speedup[p], marker='o', label=f'{p} процессов')

plt.xlabel('Размер матрицы N')
plt.ylabel('Ускорение')
plt.title('Зависимость ускорения от размера матрицы')
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig('boost2.png', dpi=300, bbox_inches='tight')
plt.show()

# -----------------------------------
# График эффективности
# -----------------------------------
plt.figure(figsize=(10,6))

for p in processes:
    plt.plot(sizes, efficiency[p], marker='s', label=f'{p} процессов')

plt.xlabel('Размер матрицы N')
plt.ylabel('Эффективность')
plt.title('Зависимость эффективности от размера матрицы')
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig('efficiency2.png', dpi=300, bbox_inches='tight')
plt.show()
