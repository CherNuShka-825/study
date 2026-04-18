import matplotlib.pyplot as plt

# Размеры матриц
sizes = [2880, 3024, 3168, 3312, 3456]

# Время выполнения для разного числа процессов
time_1  = [26.133099, 30.183149, 34.706878, 39.603751, 45.048641]
time_4  = [7.400071, 8.560792, 9.803039, 11.178689, 12.661698]
time_9  = [5.229786, 6.056277, 6.948816, 7.939239, 9.029349]
time_16 = [3.087555, 3.565730, 4.085924, 4.659578, 5.299062]

plt.figure(figsize=(10, 6))

plt.plot(sizes, time_1,  marker='o', label='1 процесс')
plt.plot(sizes, time_4,  marker='s', label='4 процесса')
plt.plot(sizes, time_9,  marker='^', label='9 процессов')
plt.plot(sizes, time_16, marker='d', label='16 процессов')

plt.xlabel('Размер матрицы N')
plt.ylabel('Время выполнения, сек')
plt.title('Зависимость времени выполнения от размера матрицы')
plt.grid(True)
plt.legend()

plt.savefig('size.png', dpi=300, bbox_inches='tight')
plt.tight_layout()
plt.show()
