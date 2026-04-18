import matplotlib.pyplot as plt

processes = [1, 4, 9, 16]
times = [34.758478, 9.799416, 8.353113, 4.766929]

plt.figure(figsize=(8, 5))
plt.plot(processes, times, marker='o')
plt.xlabel('Количество процессов')
plt.ylabel('Время выполнения, сек')
plt.title('Зависимость времени выполнения от количества процессов')
plt.grid(True)

plt.savefig('proc.png', dpi=300, bbox_inches='tight')
plt.show()
