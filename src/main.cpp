#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "temp_sensors.h"
#include "heater.h"
#include "pump.h"
#include "valve.h"
#include "storage.h"
#include "power_control.h"
#include "display.h"
#include "buttons.h"
#include "web.h"
#include "utils.h"
#include "tasks.h"

// Глобальные переменные состояния системы
bool systemRunning = false;      // Запущен ли процесс
bool systemPaused = false;       // Поставлен ли процесс на паузу
OperationMode currentMode = MODE_NONE;  // Текущий режим работы
unsigned long processStartTime = 0;     // Время запуска процесса

// Переменные для ректификации
RectificationPhase rectPhase = PHASE_HEATING;    // Текущая фаза ректификации
unsigned long stabilizationStartTime = 0;        // Время начала фазы стабилизации
unsigned long headsStartTime = 0;                // Время начала отбора голов
bool temperatureStabilized = false;              // Стабилизировалась ли температура колонны

// Переменные для дистилляции
DistillationPhase distPhase = DIST_PHASE_HEATING;  // Текущая фаза дистилляции
bool inHeadsPhase = false;                        // Находимся ли в фазе отбора голов при дистилляции

void setup() {
  // Инициализация последовательного порта
  Serial.begin(115200);
  Serial.println("\n\n=============================================");
  Serial.println("Система управления ректификацией и дистилляцией");
  Serial.println("=============================================");
  Serial.println("Версия: " FIRMWARE_VERSION);
  Serial.println("Инициализация...");
  
  // Инициализация шины I2C
  Wire.begin(DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);

  // Инициализация подсистем
  initStorage();          // Сначала загружаем настройки
  
  // Загружаем настройки из EEPROM
  loadSystemSettings();
  loadRectificationParams();
  loadDistillationParams();
  loadPumpSettings();
  
  // Инициализируем остальные компоненты
  initTempSensors();      // Датчики температуры
  initHeater();           // Нагреватель
  initPump();             // Насос отбора
  initValve();            // Клапан отбора
  initPowerControl();     // Управление мощностью
  initDisplay();          // Дисплей
  initButtons();          // Кнопки
  initWebServer();        // Веб-сервер
  
  // Создаем задачи RTOS
  xTaskCreate(
    temperatureTask,      // Задача для опроса температур
    "TemperatureTask",    // Имя задачи
    4096,                 // Размер стека
    NULL,                 // Параметры
    1,                    // Приоритет
    &temperatureTaskHandle // Идентификатор задачи
  );
  
  xTaskCreate(
    controlTask,          // Задача для управления процессами
    "ControlTask",        // Имя задачи
    4096,                 // Размер стека
    NULL,                 // Параметры
    1,                    // Приоритет
    &controlTaskHandle    // Идентификатор задачи
  );
  
  xTaskCreate(
    interfaceTask,        // Задача для интерфейса
    "InterfaceTask",      // Имя задачи
    4096,                 // Размер стека
    NULL,                 // Параметры
    1,                    // Приоритет
    &interfaceTaskHandle  // Идентификатор задачи
  );
  
  // Выводим начальный экран
  showSplashScreen();
  
  // Воспроизводим звук приветствия
  playSound(SOUND_STARTUP);
  
  Serial.println("Инициализация завершена. Система готова к работе.");
}

void loop() {
  // Большинство задач выполняется в задачах RTOS
  // Основной цикл используется только для watchdog и других критических операций
  
  // Проверяем наличие сообщений в Serial
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Обрабатываем команды
    if (command == "start rect") {
      // Запуск ректификации
      if (!systemRunning) {
        currentMode = MODE_RECTIFICATION;
        startProcess();
      } else {
        Serial.println("Процесс уже запущен");
      }
    }
    else if (command == "start dist") {
      // Запуск дистилляции
      if (!systemRunning) {
        currentMode = MODE_DISTILLATION;
        startProcess();
      } else {
        Serial.println("Процесс уже запущен");
      }
    }
    else if (command == "stop") {
      // Остановка процесса
      if (systemRunning) {
        stopProcess();
      } else {
        Serial.println("Процесс не запущен");
      }
    }
    else if (command == "pause") {
      // Пауза процесса
      if (systemRunning && !systemPaused) {
        pauseProcess();
      } else {
        Serial.println("Процесс не запущен или уже на паузе");
      }
    }
    else if (command == "resume") {
      // Возобновление процесса
      if (systemRunning && systemPaused) {
        resumeProcess();
      } else {
        Serial.println("Процесс не на паузе или не запущен");
      }
    }
    else if (command.startsWith("power ")) {
      // Установка мощности
      int power = command.substring(6).toInt();
      if (power >= 0 && power <= 100) {
        setPowerPercent(power);
        Serial.println("Мощность установлена: " + String(power) + "%");
      } else {
        Serial.println("Неверное значение мощности (0-100)");
      }
    }
    else if (command == "status") {
      // Вывод статуса
      printSystemStatus();
    }
    else if (command == "temps") {
      // Вывод температур
      printTemperatures();
    }
    else if (command == "scan") {
      // Сканирование датчиков
      scanForTempSensors();
    }
    else if (command == "settings") {
      // Вывод настроек
      printSettings();
    }
    else if (command == "reset") {
      // Сброс настроек
      resetAllSettings();
      Serial.println("Настройки сброшены к значениям по умолчанию");
    }
    else if (command == "help") {
      // Вывод справки
      printHelp();
    }
    else {
      Serial.println("Неизвестная команда: " + command);
      Serial.println("Введите 'help' для справки");
    }
  }
  
  // Задержка для экономии ресурсов
  delay(10);
}

// Запуск процесса ректификации или дистилляции
void startProcess() {
  if (systemRunning) {
    Serial.println("Процесс уже запущен");
    return;
  }
  
  // Проверяем, что все необходимые датчики подключены
  if (!checkRequiredSensors()) {
    Serial.println("Не все необходимые датчики подключены!");
    playSound(SOUND_ERROR);
    return;
  }
  
  // Сбрасываем счетчики
  resetCollectedVolumes();
  
  // Настройка параметров в зависимости от режима
  if (currentMode == MODE_RECTIFICATION) {
    Serial.println("Запуск процесса ректификации...");
    
    // Сбрасываем фазу и флаги
    rectPhase = PHASE_HEATING;
    temperatureStabilized = false;
    
    // Устанавливаем начальную мощность нагрева
    if (sysSettings.powerControlMode == POWER_CONTROL_MANUAL) {
      setPowerPercent(rectParams.heatingPower);
    } else {
      setPowerWatts(rectParams.heatingPowerWatts);
    }
    
    // Пока не включаем насос и клапан
    disablePump();
    disableValve();
    
  } else if (currentMode == MODE_DISTILLATION) {
    Serial.println("Запуск процесса дистилляции...");
    
    // Сбрасываем фазу и флаги
    distPhase = DIST_PHASE_HEATING;
    inHeadsPhase = false;
    
    // Устанавливаем начальную мощность нагрева
    if (sysSettings.powerControlMode == POWER_CONTROL_MANUAL) {
      setPowerPercent(distParams.heatingPower);
    } else {
      setPowerWatts(distParams.heatingPowerWatts);
    }
    
    // Пока не включаем насос и клапан
    disablePump();
    disableValve();
  } else {
    Serial.println("Не выбран режим работы!");
    return;
  }
  
  // Включаем нагрев
  enableHeater();
  
  // Запоминаем время старта
  processStartTime = millis();
  
  // Устанавливаем флаги
  systemRunning = true;
  systemPaused = false;
  
  // Воспроизводим звук начала процесса
  playSound(SOUND_PROCESS_START);
  
  // Отправляем уведомление на веб-интерфейс
  String modeName = (currentMode == MODE_RECTIFICATION) ? "ректификации" : "дистилляции";
  sendWebNotification(NOTIFY_SUCCESS, "Процесс " + modeName + " запущен");
  
  // Выводим в консоль
  Serial.println("Процесс успешно запущен");
}

// Остановка текущего процесса
void stopProcess() {
  if (!systemRunning) {
    Serial.println("Процесс не запущен");
    return;
  }
  
  // Выключаем нагрев
  disableHeater();
  setPowerPercent(0);
  
  // Выключаем насос и клапан
  disablePump();
  disableValve();
  
  // Сбрасываем флаги
  systemRunning = false;
  systemPaused = false;
  
  // Воспроизводим звук окончания процесса
  playSound(SOUND_PROCESS_STOP);
  
  // Отправляем уведомление на веб-интерфейс
  String modeName = (currentMode == MODE_RECTIFICATION) ? "ректификации" : "дистилляции";
  sendWebNotification(NOTIFY_INFO, "Процесс " + modeName + " остановлен");
  
  // Выводим в консоль
  Serial.println("Процесс остановлен");
  
  // Выводим итоги
  if (currentMode == MODE_RECTIFICATION) {
    Serial.println("Итоги ректификации:");
    Serial.println("- Головы: " + String(headsCollected) + " мл");
    Serial.println("- Тело: " + String(bodyCollected) + " мл");
    Serial.println("- Хвосты: " + String(tailsCollected) + " мл");
    Serial.println("- Всего: " + String(headsCollected + bodyCollected + tailsCollected) + " мл");
  } else {
    Serial.println("Итоги дистилляции:");
    Serial.println("- Всего отобрано: " + String(distillationCollected) + " мл");
  }
  
  // Сбрасываем текущий режим
  currentMode = MODE_NONE;
}

// Пауза процесса
void pauseProcess() {
  if (!systemRunning || systemPaused) {
    return;
  }
  
  // Выключаем нагрев
  disableHeater();
  
  // Выключаем насос и клапан
  disablePump();
  disableValve();
  
  systemPaused = true;
  
  // Воспроизводим звук паузы
  playSound(SOUND_PAUSE);
  
  // Отправляем уведомление на веб-интерфейс
  sendWebNotification(NOTIFY_INFO, "Процесс приостановлен");
  
  Serial.println("Процесс приостановлен");
}

// Возобновление процесса
void resumeProcess() {
  if (!systemRunning || !systemPaused) {
    return;
  }
  
  // Включаем нагрев
  enableHeater();
  
  // Возобновляем работу насоса и клапана в зависимости от режима и фазы
  if (currentMode == MODE_RECTIFICATION) {
    if (rectPhase == PHASE_HEADS) {
      // Для фазы голов
      enableValve();
      enablePump(pumpSettings.headsFlowRate);
    } else if (rectPhase == PHASE_BODY) {
      // Для фазы тела
      enablePump(pumpSettings.bodyFlowRate);
      // Клапан будет управляться в режиме орошения
    } else if (rectPhase == PHASE_TAILS) {
      // Для фазы хвостов
      enableValve();
      enablePump(pumpSettings.tailsFlowRate);
    }
  } else if (currentMode == MODE_DISTILLATION) {
    if (distPhase == DIST_PHASE_DISTILLATION) {
      enableValve();
      if (inHeadsPhase) {
        enablePump(distParams.headsFlowRate);
      } else {
        enablePump(distParams.flowRate);
      }
    }
  }
  
  systemPaused = false;
  
  // Воспроизводим звук возобновления
  playSound(SOUND_RESUME);
  
  // Отправляем уведомление на веб-интерфейс
  sendWebNotification(NOTIFY_INFO, "Процесс возобновлен");
  
  Serial.println("Процесс возобновлен");
}

// Проверка, подключены ли все необходимые датчики
bool checkRequiredSensors() {
  bool sensorsOk = true;
  
  // Датчик температуры куба всегда необходим
  if (!isSensorConnected(TEMP_CUBE)) {
    Serial.println("Ошибка: датчик температуры куба не подключен!");
    sensorsOk = false;
  }
  
  // Датчик температуры колонны необходим для ректификации
  if (currentMode == MODE_RECTIFICATION && !isSensorConnected(TEMP_REFLUX)) {
    Serial.println("Ошибка: датчик температуры колонны не подключен!");
    sensorsOk = false;
  }
  
  return sensorsOk;
}

// Вывод текущего статуса системы
void printSystemStatus() {
  Serial.println("\n=== Статус системы ===");
  
  // Общий статус
  Serial.print("Статус: ");
  if (systemRunning) {
    if (systemPaused) {
      Serial.println("Пауза");
    } else {
      Serial.println("Работает");
    }
  } else {
    Serial.println("Остановлен");
  }
  
  // Режим работы
  Serial.print("Режим: ");
  switch (currentMode) {
    case MODE_NONE:
      Serial.println("Не выбран");
      break;
    case MODE_RECTIFICATION:
      Serial.print("Ректификация (");
      
      // Фаза ректификации
      switch (rectPhase) {
        case PHASE_HEATING:
          Serial.print("Нагрев");
          break;
        case PHASE_STABILIZATION:
          Serial.print("Стабилизация");
          break;
        case PHASE_HEADS:
          Serial.print("Отбор голов");
          break;
        case PHASE_POST_HEADS_STABILIZATION:
          Serial.print("Стабилизация после голов");
          break;
        case PHASE_BODY:
          Serial.print("Отбор тела");
          break;
        case PHASE_TAILS:
          Serial.print("Отбор хвостов");
          break;
        case PHASE_COMPLETED:
          Serial.print("Завершено");
          break;
      }
      
      Serial.println(")");
      break;
    case MODE_DISTILLATION:
      Serial.print("Дистилляция (");
      
      // Фаза дистилляции
      switch (distPhase) {
        case DIST_PHASE_HEATING:
          Serial.print("Нагрев");
          break;
        case DIST_PHASE_DISTILLATION:
          if (inHeadsPhase) {
            Serial.print("Отбор голов");
          } else {
            Serial.print("Отбор");
          }
          break;
        case DIST_PHASE_COMPLETED:
          Serial.print("Завершено");
          break;
      }
      
      Serial.println(")");
      break;
  }
  
  // Время работы
  if (systemRunning) {
    Serial.print("Время работы: ");
    Serial.println(getFormattedTime(millis() - processStartTime));
  }
  
  // Мощность нагрева
  Serial.print("Мощность нагрева: ");
  Serial.print(getCurrentPowerPercent());
  Serial.print("% (");
  Serial.print(getCurrentPowerWatts());
  Serial.println(" Вт)");
  
  // Температуры
  printTemperatures();
  
  // Статус насоса
  Serial.print("Насос: ");
  if (isPumpEnabled()) {
    Serial.print("Включен (");
    Serial.print(getCurrentFlowRate());
    Serial.println(" мл/час)");
  } else {
    Serial.println("Выключен");
  }
  
  // Статус клапана
  Serial.print("Клапан: ");
  Serial.println(isValveOpen() ? "Открыт" : "Закрыт");
  
  // Объемы отбора
  if (currentMode == MODE_RECTIFICATION) {
    Serial.println("\nОтобрано:");
    Serial.print("- Головы: ");
    Serial.print(headsCollected);
    Serial.println(" мл");
    
    Serial.print("- Тело: ");
    Serial.print(bodyCollected);
    Serial.println(" мл");
    
    Serial.print("- Хвосты: ");
    Serial.print(tailsCollected);
    Serial.println(" мл");
    
    Serial.print("- Всего: ");
    Serial.print(headsCollected + bodyCollected + tailsCollected);
    Serial.println(" мл");
  } 
  else if (currentMode == MODE_DISTILLATION) {
    Serial.print("\nОтобрано: ");
    Serial.print(distillationCollected);
    Serial.println(" мл");
  }
  
  Serial.println("=====================");
}

// Вывод температур
void printTemperatures() {
  Serial.println("\nТемпературы:");
  
  // Куб
  Serial.print("- Куб: ");
  if (isSensorConnected(TEMP_CUBE)) {
    Serial.print(temperatures[TEMP_CUBE]);
    Serial.println(" °C");
  } else {
    Serial.println("Датчик не подключен");
  }
  
  // Колонна
  Serial.print("- Колонна: ");
  if (isSensorConnected(TEMP_REFLUX)) {
    Serial.print(temperatures[TEMP_REFLUX]);
    Serial.println(" °C");
  } else {
    Serial.println("Датчик не подключен");
  }
  
  // Продукт
  Serial.print("- Продукт: ");
  if (isSensorConnected(TEMP_PRODUCT)) {
    Serial.print(temperatures[TEMP_PRODUCT]);
    Serial.println(" °C");
  } else {
    Serial.println("Датчик не подключен");
  }
}

// Вывод настроек
void printSettings() {
  Serial.println("\n=== Настройки системы ===");
  
  // Системные настройки
  Serial.println("Системные настройки:");
  Serial.print("- Максимальная мощность нагрева: ");
  Serial.print(sysSettings.maxHeaterPowerWatts);
  Serial.println(" Вт");
  
  Serial.print("- Режим управления мощностью: ");
  switch (sysSettings.powerControlMode) {
    case POWER_CONTROL_MANUAL:
      Serial.println("Ручной");
      break;
    case POWER_CONTROL_PI:
      Serial.println("PI-регулятор");
      break;
    case POWER_CONTROL_PZEM:
      Serial.println("По PZEM");
      break;
  }
  
  // Параметры ректификации
  Serial.println("\nПараметры ректификации:");
  Serial.print("- Модель ректификации: ");
  Serial.println((rectParams.model == MODEL_CLASSIC) ? "Классическая" : "Альтернативная");
  
  Serial.print("- Макс. температура куба: ");
  Serial.print(rectParams.maxCubeTemp);
  Serial.println(" °C");
  
  Serial.print("- Температура голов: ");
  Serial.print(rectParams.headsTemp);
  Serial.println(" °C");
  
  Serial.print("- Температура тела: ");
  Serial.print(rectParams.bodyTemp);
  Serial.println(" °C");
  
  Serial.print("- Температура хвостов: ");
  Serial.print(rectParams.tailsTemp);
  Serial.println(" °C");
  
  Serial.print("- Температура окончания: ");
  Serial.print(rectParams.endTemp);
  Serial.println(" °C");
  
  Serial.print("- Мощность при нагреве: ");
  Serial.print(rectParams.heatingPowerWatts);
  Serial.print(" Вт (");
  Serial.print(rectParams.heatingPower);
  Serial.println("%)");
  
  Serial.print("- Мощность при стабилизации: ");
  Serial.print(rectParams.stabilizationPowerWatts);
  Serial.print(" Вт (");
  Serial.print(rectParams.stabilizationPower);
  Serial.println("%)");
  
  Serial.print("- Мощность при отборе тела: ");
  Serial.print(rectParams.bodyPowerWatts);
  Serial.print(" Вт (");
  Serial.print(rectParams.bodyPower);
  Serial.println("%)");
  
  Serial.print("- Мощность при отборе хвостов: ");
  Serial.print(rectParams.tailsPowerWatts);
  Serial.print(" Вт (");
  Serial.print(rectParams.tailsPower);
  Serial.println("%)");
  
  Serial.print("- Время стабилизации: ");
  Serial.print(rectParams.stabilizationTime);
  Serial.println(" мин");
  
  Serial.print("- Объем голов: ");
  Serial.print(rectParams.headsVolume);
  Serial.println(" мл");
  
  Serial.print("- Объем тела: ");
  Serial.print(rectParams.bodyVolume);
  Serial.println(" мл");
  
  Serial.print("- Соотношение орошения: ");
  Serial.println(rectParams.refluxRatio);
  
  Serial.print("- Период орошения: ");
  Serial.print(rectParams.refluxPeriod);
  Serial.println(" с");
  
  // Альтернативные параметры ректификации
  if (rectParams.model == MODEL_ALTERNATIVE) {
    Serial.println("\nДополнительные параметры альтернативной модели:");
    
    Serial.print("- Целевое время отбора голов: ");
    Serial.print(rectParams.headsTargetTimeMinutes);
    Serial.println(" мин");
    
    Serial.print("- Время стабилизации после голов: ");
    Serial.print(rectParams.postHeadsStabilizationTime);
    Serial.println(" мин");
    
    Serial.print("- Скорость отбора тела: ");
    Serial.print(rectParams.bodyFlowRateMlPerHour);
    Serial.println(" мл/час");
    
    Serial.print("- Дельта температуры для окончания тела: ");
    Serial.print(rectParams.tempDeltaEndBody);
    Serial.println(" °C");
    
    Serial.print("- Температура куба для перехода к хвостам: ");
    Serial.print(rectParams.tailsCubeTemp);
    Serial.println(" °C");
    
    Serial.print("- Скорость отбора хвостов: ");
    Serial.print(rectParams.tailsFlowRateMlPerHour);
    Serial.println(" мл/час");
    
    Serial.print("- Использовать ту же скорость для хвостов: ");
    Serial.println(rectParams.useSameFlowRateForTails ? "Да" : "Нет");
  }
  
  // Параметры дистилляции
  Serial.println("\nПараметры дистилляции:");
  Serial.print("- Макс. температура куба: ");
  Serial.print(distParams.maxCubeTemp);
  Serial.println(" °C");
  
  Serial.print("- Температура начала отбора: ");
  Serial.print(distParams.startCollectingTemp);
  Serial.println(" °C");
  
  Serial.print("- Температура окончания: ");
  Serial.print(distParams.endTemp);
  Serial.println(" °C");
  
  Serial.print("- Мощность при нагреве: ");
  Serial.print(distParams.heatingPowerWatts);
  Serial.print(" Вт (");
  Serial.print(distParams.heatingPower);
  Serial.println("%)");
  
  Serial.print("- Мощность при дистилляции: ");
  Serial.print(distParams.distillationPowerWatts);
  Serial.print(" Вт (");
  Serial.print(distParams.distillationPower);
  Serial.println("%)");
  
  Serial.print("- Скорость отбора: ");
  Serial.print(distParams.flowRate);
  Serial.println(" мл/час");
  
  Serial.print("- Отделять головы: ");
  Serial.println(distParams.separateHeads ? "Да" : "Нет");
  
  if (distParams.separateHeads) {
    Serial.print("- Объем голов: ");
    Serial.print(distParams.headsVolume);
    Serial.println(" мл");
    
    Serial.print("- Скорость отбора голов: ");
    Serial.print(distParams.headsFlowRate);
    Serial.println(" мл/час");
  }
  
  // Параметры насоса
  Serial.println("\nНастройки насоса:");
  Serial.print("- Калибровочный коэффициент: ");
  Serial.print(pumpSettings.calibrationFactor);
  Serial.println(" мл/с при 100% мощности");
  
  Serial.print("- Скорость отбора голов: ");
  Serial.print(pumpSettings.headsFlowRate);
  Serial.println(" мл/час");
  
  Serial.print("- Скорость отбора тела: ");
  Serial.print(pumpSettings.bodyFlowRate);
  Serial.println(" мл/час");
  
  Serial.print("- Скорость отбора хвостов: ");
  Serial.print(pumpSettings.tailsFlowRate);
  Serial.println(" мл/час");
  
  Serial.print("- Минимальная скорость отбора: ");
  Serial.print(pumpSettings.minFlowRate);
  Serial.println(" мл/час");
  
  Serial.print("- Максимальная скорость отбора: ");
  Serial.print(pumpSettings.maxFlowRate);
  Serial.println(" мл/час");
  
  Serial.print("- Период работы насоса: ");
  Serial.print(pumpSettings.pumpPeriodMs);
  Serial.println(" мс");
  
  Serial.println("=====================");
}

// Вывод справки по командам
void printHelp() {
  Serial.println("\n=== Доступные команды ===");
  Serial.println("start rect - запуск ректификации");
  Serial.println("start dist - запуск дистилляции");
  Serial.println("stop - остановка процесса");
  Serial.println("pause - приостановка процесса");
  Serial.println("resume - возобновление процесса");
  Serial.println("power <0-100> - установка мощности нагрева");
  Serial.println("status - вывод текущего статуса системы");
  Serial.println("temps - вывод текущих температур");
  Serial.println("scan - сканирование датчиков температуры");
  Serial.println("settings - вывод текущих настроек");
  Serial.println("reset - сброс настроек к значениям по умолчанию");
  Serial.println("help - вывод этой справки");
  Serial.println("=====================");
}

// Аварийное отключение нагрева
void emergencyHeaterShutdown(const String& reason) {
  // Выключаем нагрев
  disableHeater();
  setPowerPercent(0);
  
  // Выключаем насос и клапан
  disablePump();
  disableValve();
  
  // Обновляем статус
  systemPaused = true;
  
  // Воспроизводим звук тревоги
  playSound(SOUND_ALARM);
  
  // Логируем
  Serial.println("АВАРИЙНОЕ ОТКЛЮЧЕНИЕ: " + reason);
  
  // Отправляем уведомление на веб-интерфейс
  sendWebNotification(NOTIFY_ERROR, "АВАРИЙНОЕ ОТКЛЮЧЕНИЕ: " + reason);
}

// Отправка уведомления через WebSocket
void sendWebNotification(NotificationType type, const String& message) {
  // Определяем строковое представление типа
  String typeStr;
  switch (type) {
    case NOTIFY_INFO:
      typeStr = "info";
      break;
    case NOTIFY_SUCCESS:
      typeStr = "success";
      break;
    case NOTIFY_WARNING:
      typeStr = "warning";
      break;
    case NOTIFY_ERROR:
      typeStr = "error";
      break;
    default:
      typeStr = "info";
      break;
  }

  // Отправляем уведомление через WebSocket в формате JSON
  String jsonMessage = "{\"type\":\"notification\",\"level\":\"" + typeStr + "\",\"message\":\"" + message + "\"}";
  broadcastWebSocketMessage(jsonMessage);

  // Выводим в консоль
  String consoleType;
  switch (type) {
    case NOTIFY_INFO:
      consoleType = "ИНФО";
      break;
    case NOTIFY_SUCCESS:
      consoleType = "УСПЕХ";
      break;
    case NOTIFY_WARNING:
      consoleType = "ПРЕДУПРЕЖДЕНИЕ";
      break;
    case NOTIFY_ERROR:
      consoleType = "ОШИБКА";
      break;
    default:
      consoleType = "ИНФО";
      break;
  }

  Serial.println("[" + consoleType + "] " + message);
}

// Перевод процентов мощности в ватты
int percentToWatts(int percent) {
  return (int)((float)percent * sysSettings.maxHeaterPowerWatts / 100.0);
}

// Перевод ваттов в проценты мощности
int wattsToPercent(int watts) {
  return (int)((float)watts * 100.0 / sysSettings.maxHeaterPowerWatts);
}

// Воспроизведение звукового сигнала
void playSound(SoundType sound) {
  if (!sysSettings.soundEnabled) {
    return; // Звук выключен
  }
  
  // Здесь можно добавить код для воспроизведения разных звуков
  // в зависимости от типа события
  #ifdef PIN_BUZZER
    int duration = 100; // длительность по умолчанию
    int frequency = 1000; // частота по умолчанию
    int repeats = 1; // количество повторений
    
    switch (sound) {
      case SOUND_STARTUP:
        frequency = 1318; // E6
        duration = 150;
        tone(PIN_BUZZER, frequency, duration);
        delay(duration + 50);
        frequency = 1568; // G6
        tone(PIN_BUZZER, frequency, duration);
        delay(duration + 50);
        frequency = 2090; // C7
        duration = 300;
        tone(PIN_BUZZER, frequency, duration);
        break;
        
      case SOUND_PROCESS_START:
        frequency = 1047; // C6
        duration = 100;
        repeats = 2;
        for (int i = 0; i < repeats; i++) {
          tone(PIN_BUZZER, frequency, duration);
          delay(duration + 50);
        }
        break;
        
      case SOUND_PROCESS_STOP:
        frequency = 1047; // C6
        duration = 300;
        tone(PIN_BUZZER, frequency, duration);
        break;
        
      case SOUND_PROCESS_COMPLETE:
        frequency = 1047; // C6
        duration = 100;
        tone(PIN_BUZZER, frequency, duration);
        delay(duration + 50);
        frequency = 1318; // E6
        tone(PIN_BUZZER, frequency, duration);
        delay(duration + 50);
        frequency = 1568; // G6
        tone(PIN_BUZZER, frequency, duration);
        delay(duration + 50);
        frequency = 2090; // C7
        duration = 300;
        tone(PIN_BUZZER, frequency, duration);
        break;
        
      case SOUND_PHASE_CHANGE:
        frequency = 1568; // G6
        duration = 100;
        repeats = 2;
        for (int i = 0; i < repeats; i++) {
          tone(PIN_BUZZER, frequency, duration);
          delay(duration + 50);
        }
        break;
        
      case SOUND_PAUSE:
        frequency = 1318; // E6
        duration = 150;
        tone(PIN_BUZZER, frequency, duration);
        break;
        
      case SOUND_RESUME:
        frequency = 1318; // E6
        duration = 100;
        repeats = 2;
        for (int i = 0; i < repeats; i++) {
          tone(PIN_BUZZER, frequency, duration);
          delay(duration + 50);
        }
        break;
        
      case SOUND_ALARM:
        frequency = 2093; // C7
        duration = 150;
        repeats = 3;
        for (int i = 0; i < repeats; i++) {
          tone(PIN_BUZZER, frequency, duration);
          delay(duration);
          tone(PIN_BUZZER, frequency / 2, duration);
          delay(duration + 50);
        }
        break;
        
      case SOUND_ERROR:
        frequency = 440; // A4
        duration = 200;
        repeats = 3;
        for (int i = 0; i < repeats; i++) {
          tone(PIN_BUZZER, frequency, duration);
          delay(duration + 50);
        }
        break;
        
      case SOUND_BUTTON:
        frequency = 1000;
        duration = 50;
        tone(PIN_BUZZER, frequency, duration);
        break;
        
      default:
        // Простой сигнал по умолчанию
        tone(PIN_BUZZER, frequency, duration);
        break;
    }
  #endif
}