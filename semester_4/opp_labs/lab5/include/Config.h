#pragma once

#include <string>

struct Config {
    int iterations = 20; // глобальные итерации
    int tasksPerIteration = 1000; // кол-во задач на итерацию
    int baseRepeat = 20000; // базовая тяжесть задачи
    int virtualSlots = 16; // виртуальные слоты
    int pyramidHeight = 4; // коэфициент высоты для пирамиды
    int minTasksToKeep = 0; // сколько задач остается у донора при дележке

    bool balance = false; // вкл/выкл балансировки

    std::string scenario = "pyramid"; // pyramid | uniform | one
};
