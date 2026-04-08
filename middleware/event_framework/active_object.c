/**
 * @file active_object.c
 * @brief Active Object Implementation
 * @date 2026-04-01
 */

#include "active_object.h"

/* ============================================================================
 * ACTIVE OBJECT API IMPLEMENTATION
 * ============================================================================
 */

void ActiveObject_Init(
    ActiveObject_t  *me,
    uint8_t          priority,
    StateHandler_t   state,
    void            *priv)
{
    if (me == NULL) return;
    
    me->priority = priority;
    me->state = state;
    me->priv = priv;
    
    EventQueue_Init(&me->queue);
}

bool ActiveObject_Post(ActiveObject_t *me, const Event_t *e)
{
    if (me == NULL || e == NULL) return false;
    return EventQueue_Post(&me->queue, e);
}

bool ActiveObject_Dispatch(ActiveObject_t *me)
{
    if (me == NULL) return false;
    
    Event_t e;
    
    /* Lấy event từ queue (RTC step) */
    if (!EventQueue_Pend(&me->queue, &e)) {
        return false;  /* Queue trống */
    }
    
    /* Gọi state handler để xử lý event */
    if (me->state != NULL) {
        StateHandler_t next_state = me->state(me, &e);
        
        /* Nếu state handler return state khác, tạo transition */
        if (next_state != NULL && next_state != me->state) {
            /* Exit action từ state hiện tại */
            if (me->state != NULL) {
                Event_t exit_e = Event_Create(EXIT_SIG);
                me->state(me, &exit_e);
            }
            
            /* Transition */
            me->state = next_state;
            
            /* Entry action vào state mới */
            if (me->state != NULL) {
                Event_t entry_e = Event_Create(ENTRY_SIG);
                me->state(me, &entry_e);
            }
        }
    }
    
    return true;
}

void ActiveObject_Start(ActiveObject_t *me)
{
    if (me == NULL) return;
    
    /* Initial transition - gửi ENTRY_SIG */
    Event_t init_e = Event_Create(ENTRY_SIG);
    if (me->state != NULL) {
        me->state(me, &init_e);
    }
}

StateHandler_t ActiveObject_GetState(const ActiveObject_t *me)
{
    if (me == NULL) return NULL;
    return me->state;
}

void ActiveObject_SetState(ActiveObject_t *me, StateHandler_t state)
{
    if (me == NULL) return;
    me->state = state;
}

bool ActiveObject_HasEvents(const ActiveObject_t *me)
{
    if (me == NULL) return false;
    return !EventQueue_IsEmpty(&me->queue);
}

uint16_t ActiveObject_EventCount(const ActiveObject_t *me)
{
    if (me == NULL) return 0;
    return EventQueue_Count(&me->queue);
}

/* ============================================================================
 * SCHEDULER IMPLEMENTATION (Cooperative Kernel QV style)
 * ============================================================================
 */

void Scheduler_Init(Scheduler_t *sched, void (*idle_cb)(void))
{
    if (sched == NULL) return;
    
    sched->count = 0;
    sched->idle_callback = idle_cb;
}

bool Scheduler_Register(Scheduler_t *sched, ActiveObject_t *ao)
{
    if (sched == NULL || ao == NULL) return false;
    
    if (sched->count >= MAX_ACTIVE_OBJECTS) {
        return false;  /* Scheduler đầy */
    }
    
    sched->aos[sched->count] = ao;
    sched->count++;
    
    return true;
}

bool Scheduler_Step(Scheduler_t *sched)
{
    if (sched == NULL) return false;
    
    /* Tìm AO có priority cao nhất (số nhỏ nhất) và có events */
    ActiveObject_t *highest_priority_ao = NULL;
    uint8_t highest_priority = 0xFF;
    
    for (uint8_t i = 0; i < sched->count; i++) {
        ActiveObject_t *ao = sched->aos[i];
        if (ao != NULL && ActiveObject_HasEvents(ao)) {
            if (ao->priority < highest_priority) {
                highest_priority = ao->priority;
                highest_priority_ao = ao;
            }
        }
    }
    
    /* Dispatch event từ AO với priority cao nhất */
    if (highest_priority_ao != NULL) {
        return ActiveObject_Dispatch(highest_priority_ao);
    }
    
    return false;  /* Không có AO nào có events */
}

void Scheduler_Run(Scheduler_t *sched)
{
    if (sched == NULL) return;
    
    while (1) {
        /* Dispatch một event */
        if (!Scheduler_Step(sched)) {
            /* Tất cả queue trống - gọi idle callback */
            if (sched->idle_callback != NULL) {
                sched->idle_callback();
            }
        }
    }
}
