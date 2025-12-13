/**
 * @file tasks.h
 * @brief FreeRTOS tasks for the rectification controller
 */

#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Task handles for FreeRTOS tasks
extern TaskHandle_t temperatureTaskHandle;
extern TaskHandle_t controlTaskHandle;
extern TaskHandle_t interfaceTaskHandle;

/**
 * @brief Task for reading temperature sensors
 *
 * @param parameter Task parameter (unused)
 */
void temperatureTask(void* parameter);

/**
 * @brief Task for controlling the distillation/rectification process
 *
 * @param parameter Task parameter (unused)
 */
void controlTask(void* parameter);

/**
 * @brief Task for handling user interface (buttons and display)
 *
 * @param parameter Task parameter (unused)
 */
void interfaceTask(void* parameter);

#endif // TASKS_H
