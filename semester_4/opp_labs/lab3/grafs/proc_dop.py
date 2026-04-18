import matplotlib.pyplot as plt

# Данные
processes = [1, 4, 9, 16]
times = [34.758478, 9.799416, 8.353113, 4.766929]

# Базовое время (1 процесс)
T1 = times[0]

# Ускорение: S(p) = T1 / Tp
speedup = [T1 / t for t in times]

# Эффективность: E(p) = S(p) / p
efficiency = [s / p for s, p in zip(speedup, processes)]

# ---------------------------
# График ускорения
# ---------------------------
plt.figure(figsize=(8, 5))
plt.plot(processes, speedup, marker='o', linewidth=2, label='Реальное ускорение')
plt.plot(processes, processes, '--', label='Идеальное ускорение')

plt.xlabel('Количество процессов')
plt.ylabel('Ускорение')
plt.title('Зависимость ускорения от количества процессов')
plt.grid(True)
plt.legend()
plt.tight_layout()

plt.savefig('boost.png', dpi=300, bbox_inches='tight')
plt.show()

# ---------------------------
# График эффективности
# ---------------------------
plt.figure(figsize=(8, 5))
plt.plot(processes, efficiency, marker='s', linewidth=2)

plt.xlabel('Количество процессов')
plt.ylabel('Эффективность')
plt.title('Зависимость эффективности от количества процессов')
plt.grid(True)
plt.tight_layout()

plt.savefig('efficiency.png', dpi=300, bbox_inches='tight')
plt.show()
