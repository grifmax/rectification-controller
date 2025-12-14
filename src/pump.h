#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>
#include "config.h"

// Глобальные переменные для отслеживания объемов
extern float headsCollected;
extern float bodyCollected;
extern float tailsCollected;
extern float distillationCollected;

// Инициализация насоса
void initPump();

// Включение насоса с заданной скоростью отбора (мл/час)
void enablePump(float flowRateMlPerHour);

// Выключение насоса
void disablePump();

// Обновление работы насоса (вызывается периодически)
void updatePump();

// Получение текущей скорости отбора (мл/час)
float getCurrentFlowRate();

// Получение общего объема отбора (мл) для текущей фазы
float getCollectedVolume(int phase);

// Калибровка насоса
void calibratePump(float calibrationFactor);

// Проверка, включен ли насос
bool isPumpEnabled();

// Сброс счетчиков отбора
void resetCollectedVolumes();

#endif // PUMP_H