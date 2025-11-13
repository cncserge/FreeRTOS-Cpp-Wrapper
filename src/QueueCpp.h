#ifndef QUEUE_CPP_H
#define QUEUE_CPP_H

#ifdef Arduino_h
#include <Arduino.h>
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <cstddef>
#include <stdexcept>

class QueueBase {
  public:
    QueueBase(QueueHandle_t handle) : handle(handle) {}
    // Очищает очередь
    bool reset() const {
        return xQueueReset(handle) == pdTRUE;
    }
    // Возвращает количество сообщений в очереди
    size_t messagesWaiting() const {
        return uxQueueMessagesWaiting(handle);
    }
    // Возвращает количество доступных мест в очереди
    size_t spacesAvailable() const {
        return uxQueueSpacesAvailable(handle);
    }
    // Проверяет, пуста ли очередь из прерывания
    bool isEmptyISR() const {
        return xQueueIsQueueEmptyFromISR(handle) == pdTRUE;
    }
    // Проверяет, полна ли очередь из прерывания
    bool isFullISR() const {
        return xQueueIsQueueFullFromISR(handle) == pdTRUE;
    }
    // Проверяет, пуста ли очередь
    bool isEmpty() const {
        return uxQueueMessagesWaiting(handle) == 0;
    }
    // Проверяет, полна ли очередь
    bool isFull() const {
        return uxQueueSpacesAvailable(handle) == 0;
    }
    // Возвращает нативный дескриптор очереди
    QueueHandle_t nativeHandle() const {
        return handle;
    }
    virtual ~QueueBase() {
        vQueueDelete(handle);
    }

  protected:
    QueueHandle_t handle;

  private:
    QueueBase(QueueBase const&) = delete;
    void operator=(QueueBase const&) = delete;
};

// Queue

template <typename T>
class QueueTypeBase : public QueueBase {
  protected:
    QueueTypeBase(QueueHandle_t handle) : QueueBase(handle) {}

  public:
    // Добавляет элемент в очередь
    bool send(const T& item, uint32_t ms = 0) {
        return xQueueSend(handle, &item, pdMS_TO_TICKS(ms)) == pdTRUE;
    }
    // Добавляет элемент в конец очереди
    bool sendToBack(const T& item, uint32_t ms = 0) {
        return xQueueSendToBack(handle, &item, pdMS_TO_TICKS(ms)) == pdTRUE;
    }
    // Добавляет элемент в начало очереди
    bool sendToFront(const T& item, uint32_t ms = 0) {
        return xQueueSendToFront(handle, &item, pdMS_TO_TICKS(ms)) == pdTRUE;
    }
    // Получает элемент из очереди
    bool receive(T& item, uint32_t ms = 0) {
        return xQueueReceive(handle, &item, pdMS_TO_TICKS(ms)) == pdTRUE;
    }
    // Просматривает первый элемент очереди без удаления
    bool peek(T& item, uint32_t ms = 0) {
        return xQueuePeek(handle, &item, pdMS_TO_TICKS(ms)) == pdTRUE;
    }
    // Перезаписывает элемент в очереди
    bool overwrite(const T& item) {
        return xQueueOverwrite(handle, &item) == pdTRUE;
    }
    // Добавляет элемент в очередь из прерывания
    bool sendFromISR(const T& item, BaseType_t* pxHigherPriorityTaskWoken) {
        return xQueueSendFromISR(handle, &item, pxHigherPriorityTaskWoken) == pdTRUE;
    }
    // Добавляет элемент в конец очереди из прерывания
    bool sendToBackFromISR(const T& item, BaseType_t* pxHigherPriorityTaskWoken) {
        return xQueueSendToBackFromISR(handle, &item, pxHigherPriorityTaskWoken) == pdTRUE;
    }
    // Добавляет элемент в начало очереди из прерывания
    bool sendToFrontFromISR(const T& item, BaseType_t* pxHigherPriorityTaskWoken) {
        return xQueueSendToFrontFromISR(handle, &item, pxHigherPriorityTaskWoken) == pdTRUE;
    }
    // Получает элемент из очереди из прерывания
    bool receiveFromISR(T& item, BaseType_t* pxHigherPriorityTaskWoken) {
        return xQueueReceiveFromISR(handle, &item, pxHigherPriorityTaskWoken) == pdTRUE;
    }
    // Просматривает первый элемент очереди из прерывания без удаления
    bool peekFromISR(T& item) {
        return xQueuePeekFromISR(handle, &item) == pdTRUE;
    }
    // Перезаписывает элемент в очереди из прерывания
    bool overwriteFromISR(const T& item, BaseType_t* pxHigherPriorityTaskWoken) {
        return xQueueOverwriteFromISR(handle, &item, pxHigherPriorityTaskWoken) == pdTRUE;
    }
};

template <class T>
class Queue : public QueueTypeBase<T> {
  public:
    Queue(size_t length, const char* name = nullptr)
        : QueueTypeBase<T>(xQueueCreate(length, sizeof(T))) {
        if (this->handle == nullptr) {
            throw std::runtime_error("Failed to create queue");
        }
#if (configQUEUE_REGISTRY_SIZE > 0)
        if (name) {
            vQueueAddToRegistry(this->handle, name);
        }
#else
        (void)name;
#endif
    };
};
#endif  // QUEUE_CPP_H