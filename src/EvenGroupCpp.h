#ifndef EVEN_GROUP_CPP_H
#define EVEN_GROUP_CPP_H

#ifdef Arduino_h
#include <Arduino.h>
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <cstdint>
#include <stdexcept>

class EventGroup {
  public:
    // Создать свою группу событий (RAII)
    EventGroup() : handle(xEventGroupCreate()), owned(true) {
        if (!handle) {
            configASSERT(false && "Failed to create EventGroup");
            owned = false;
            // на случай, если configASSERT ничего не делает
            // можно повиснуть, перезагрузиться или хотя бы не использовать объект
            // for(;;) {}  // или
            abort();
        }
    }

    // Обернуть уже существующий EventGroupHandle_t
    explicit EventGroup(EventGroupHandle_t existing, bool takeOwnership = false)
        : handle(existing), owned(takeOwnership) {}

    // Нельзя копировать
    EventGroup(const EventGroup&) = delete;
    EventGroup& operator=(const EventGroup&) = delete;

    // Можно перемещать
    EventGroup(EventGroup&& other) noexcept : handle(other.handle), owned(other.owned) {
        other.handle = nullptr;
        other.owned = false;
    }

    EventGroup& operator=(EventGroup&& other) noexcept {
        if (this != &other) {
            destroy();
            handle = other.handle;
            owned = other.owned;
            other.handle = nullptr;
            other.owned = false;
        }
        return *this;
    }

    ~EventGroup() {
        destroy();
    }

    bool isValid() const {
        return handle != nullptr;
    }

    // ==== Установка/сброс флагов ====

    // Установить биты (из задачи)
    EventBits_t setBits(EventBits_t bits) {
        return xEventGroupSetBits(handle, bits);
    }

    // Установить биты (из ISR)
    BaseType_t setBitsFromISR(EventBits_t bits, BaseType_t* higherPriorityTaskWoken = nullptr) {
        return xEventGroupSetBitsFromISR(handle, bits, higherPriorityTaskWoken);
    }

    // Сбросить биты (из задачи)
    EventBits_t clearBits(EventBits_t bits) {
        return xEventGroupClearBits(handle, bits);
    }

    // Сбросить биты (из ISR)
    EventBits_t clearBitsFromISR(EventBits_t bits) {
        return xEventGroupClearBitsFromISR(handle, bits);
    }

    // Прочитать текущее состояние битов (без ожидания)
    EventBits_t getBits() const {
        return xEventGroupGetBits(handle);
    }

    EventBits_t getBitsFromISR() const {
        return xEventGroupGetBitsFromISR(handle);
    }

    // ==== Ожидание ====

    // Ожидать bits, timeoutMs – в миллисекундах (0 по умолчанию = не ждать)
    // waitAll = true  → ждём все биты
    // waitAll = false → достаточно любого бита
    // clearOnExit = true → указанные биты будут очищены при выходе
    EventBits_t waitBits(EventBits_t bits, bool waitAll = true, bool clearOnExit = false, uint32_t timeoutMs = 0) {
        TickType_t to = (timeoutMs == 0) ? 0 : pdMS_TO_TICKS(timeoutMs);
        return xEventGroupWaitBits(handle, bits, clearOnExit ? pdTRUE : pdFALSE, waitAll ? pdTRUE : pdFALSE, to);
    }

    // Барьерная синхронизация (xEventGroupSync)
    EventBits_t sync(EventBits_t bitsToSet, EventBits_t bitsToWaitFor, uint32_t timeoutMs = 0) {
        TickType_t to = (timeoutMs == 0) ? 0 : pdMS_TO_TICKS(timeoutMs);
        return xEventGroupSync(handle, bitsToSet, bitsToWaitFor, to);
    }

    // Доступ к "сырую" ручке
    EventGroupHandle_t nativeHandle() const {
        return handle;
    }

  protected:
    static constexpr unsigned MaxUserBits = sizeof(EventBits_t) * 8u - 8u;  // 24 бита при 32-битном EventBits_t

    // Компайлтайм-вариант для enum'ов
    template <unsigned N>
    static constexpr EventBits_t getBit() {
        static_assert(N < MaxUserBits,"EventGroup::getBit<N>(): bit index out of range");
        return static_cast<EventBits_t>(EventBits_t(1u) << N);
    }

    // Рантайм-вариант для переменных
    static EventBits_t getBit(unsigned n) {
        configASSERT(n < MaxUserBits);
        return static_cast<EventBits_t>(EventBits_t(1u) << n);
    }

  private:
    EventGroupHandle_t handle = nullptr;
    bool               owned = false;

    void destroy() {
        if (owned && handle) {
            vEventGroupDelete(handle);
            handle = nullptr;
        }
    }
};

#endif  // EVEN_GROUP_CPP_H

/*
class MyEventGroup : public EventGroup {
public:
  MyEventGroup() : EventGroup() {}
  enum {
    door_open = getBit<0>(),
    door_closed = getBit<1>(),
    level_high = getBit<2>(),
  };
    EventBits_t maskFromIndex(unsigned i) {
        return getBit(i);  // рантайм-вариант
    }
};
MyEventGroup event;
// Задача-производитель
void producerTask(void*) {
    for (;;) {
        // ... что-то сделали
        event.setBits(MyEventGroup::door_open);  // выставили флаг "готово"
    }
}

// Задача-потребитель
void consumerTask(void*) {
    for (;;) {
        // ждём бит door_open, все/любой = всё равно один бит, не очищаем, таймаут 1000 мс
        EventBits_t bits = event.waitBits(MyEventGroup::door_open, true, false, 1000);
        if (bits & MyEventGroup::door_open) {
            // обработать событие
        }
    }
}
*/
