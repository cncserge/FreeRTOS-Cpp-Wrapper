#ifndef GUARDED_HPP
#define GUARDED_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <Arduino.h>

template <typename T>
class Guarded {
    T                 data;
    SemaphoreHandle_t mutex;

  public:
    Guarded() {
        mutex = xSemaphoreCreateMutex();
        // Можно добавить assert(mutex != nullptr);
    }

    ~Guarded() {
        if (mutex) {
            vSemaphoreDelete(mutex);
        }
    }

    // Запрет копирования и присваивания
    Guarded(const Guarded&)            = delete;
    Guarded& operator=(const Guarded&) = delete;

    // Можно разрешить только перемещение при желании
    Guarded(Guarded&&)            = delete;
    Guarded& operator=(Guarded&&) = delete;

    class Access {
        T*                ptr;
        SemaphoreHandle_t mutex;

      public:
        Access(T* p, SemaphoreHandle_t m) : ptr(p), mutex(m) {}

        // Запрет копирования, чтобы не было двойного Give
        Access(const Access&)            = delete;
        Access& operator=(const Access&) = delete;

        // Можно разрешить перемещение, если нужно
        Access(Access&& other) noexcept : ptr(other.ptr), mutex(other.mutex) {
            other.ptr   = nullptr;
            other.mutex = nullptr;
        }
        Access& operator=(Access&& other) noexcept {
            if (this != &other) {
                // сначала отдать старый, если есть
                if (mutex) xSemaphoreGive(mutex);
                ptr        = other.ptr;
                mutex      = other.mutex;
                other.ptr  = nullptr;
                other.mutex = nullptr;
            }
            return *this;
        }

        ~Access() {
            if (mutex) {
                xSemaphoreGive(mutex);
            }
        }

        T* operator->() { return ptr; }
        T& operator*()  { return *ptr; }
    };

    Access operator()() {
        xSemaphoreTake(mutex, portMAX_DELAY);
        return Access(&data, mutex);
    }
};

// Пример использования:
/*
#include <Arduino.h>
#include "Guarded.hpp"

struct Settings {
    int   speed;
    float kP;
    float kI;
};

// Глобальная защищённая структура
Guarded<Settings> g_settings;

void taskWriter(void* arg) {
    for (;;) {
        {
            // Берём доступ, в этот момент мьютекс захвачен
            auto s = g_settings();   // RAII-объект Access

            s->speed++;              // пишем как в обычную структуру
            s->kP += 0.1f;
            s->kI = 0.5f;
            // при выходе из блока ~Access() вызовет xSemaphoreGive()
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void taskReader(void* arg) {
    for (;;) {
        Settings local;
        {
            // Берём доступ и копируем данные себе локально
            auto s = g_settings();
            local  = *s;   // копируем всю структуру
        }                 // мьютекс освободился

        // Работаем с локальной копией без мьютекса
        Serial.print("speed=");
        Serial.print(local.speed);
        Serial.print(" kP=");
        Serial.print(local.kP);
        Serial.print(" kI=");
        Serial.println(local.kI);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void setup() {
    Serial.begin(115200);

    // Можно один раз инициализировать стартовые значения
    {
        auto s = g_settings();
        s->speed = 0;
        s->kP    = 1.0f;
        s->kI    = 0.0f;
    }

    xTaskCreate(taskWriter, "writer", 4096, nullptr, 2, nullptr);
    xTaskCreate(taskReader, "reader", 4096, nullptr, 2, nullptr);
}

void loop() {
    // пусто, всё в задачах
}
*/

#endif  // Guarded_HPP
