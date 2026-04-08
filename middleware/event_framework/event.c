/**
 * @file event.c
 * @brief Event-Driven Framework - Implementation
 * @date 2026-04-01
 */

#include "event.h"
#include <string.h>

/* ============================================================================
 * EVENT QUEUE IMPLEMENTATION
 * ============================================================================
 */

void EventQueue_Init(EventQueue_t *q)
{
    if (q == NULL) return;
    
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    memset(q->events, 0, sizeof(q->events));
}

bool EventQueue_Post(EventQueue_t *q, const Event_t *e)
{
    if (q == NULL || e == NULL) return false;
    
    /* Kiểm tra queue có đầy không */
    if (q->count >= EVENT_QUEUE_SIZE) {
        return false;  /* Queue full, không thêm được */
    }
    
    /* Thêm event vào tail */
    q->events[q->tail] = *e;
    q->tail = (q->tail + 1) % EVENT_QUEUE_SIZE;
    q->count++;
    
    return true;
}

bool EventQueue_Pend(EventQueue_t *q, Event_t *e)
{
    if (q == NULL || e == NULL) return false;
    
    /* Kiểm tra queue có trống không */
    if (q->count == 0) {
        return false;  /* Queue trống */
    }
    
    /* Lấy event từ head */
    *e = q->events[q->head];
    q->head = (q->head + 1) % EVENT_QUEUE_SIZE;
    q->count--;
    
    return true;
}

bool EventQueue_IsEmpty(const EventQueue_t *q)
{
    if (q == NULL) return true;
    return (q->count == 0);
}

uint16_t EventQueue_Count(const EventQueue_t *q)
{
    if (q == NULL) return 0;
    return q->count;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

Event_t Event_Create(EventSignal sig)
{
    Event_t e;
    e.sig = sig;
    e.data.u32 = 0;
    return e;
}

Event_t Event_CreateWithU32(EventSignal sig, uint32_t val)
{
    Event_t e;
    e.sig = sig;
    e.data.u32 = val;
    return e;
}

Event_t Event_CreateWithI32(EventSignal sig, int32_t val)
{
    Event_t e;
    e.sig = sig;
    e.data.i32 = val;
    return e;
}
