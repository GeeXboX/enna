/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <pthread.h>


#include "events_stack.h"


events_stack_t *
events_stack_create()
{
    events_stack_t *stack = NULL;
    
    if ((stack = (events_stack_t*)calloc(sizeof(events_stack_t), 1)) == NULL)
        return NULL;
    
    pthread_mutex_init(&(stack->m), NULL);
    stack->list = NULL;
    
    if (sem_init (&stack->semid, 0, 0)) 
    {
        free (stack);
        return NULL;
    }
    stack->nbevts = 0;
    return stack;
}


void 
events_stack_destroy(events_stack_t *stack)
{
    event_elt_t *elt = NULL;
    event_elt_t *next = NULL;
    
    if (stack == NULL)
        return;
    
    elt = stack->list;
    
    while (elt) 
    {
        next = elt->next;
        free (elt);
        elt = next;
    }
    
    sem_destroy (&(stack->semid));
    pthread_mutex_destroy (&stack->m);
    free (stack);
}


int
events_stack_push_event(events_stack_t *stack, event_elt_t *elt)
{
  if (stack == NULL || elt == NULL)
    return -1;

  pthread_mutex_lock(&(stack->m));

  elt->next = stack->list;
  stack->list = elt;


  stack->nbevts ++;
  sem_post (&stack->semid);
  pthread_mutex_unlock(&stack->m);
  
  return 0;
}


event_elt_t * 
events_stack_pop_event(events_stack_t *stack)
{
    event_elt_t *elt = NULL;
    
    if (stack == NULL)
        return NULL;
    
    sem_wait (&stack->semid);
    
    pthread_mutex_lock(&(stack->m));
    if (!stack->list) 
    {
        pthread_mutex_unlock(&(stack->m));
        return NULL;
    }
    
    elt = stack->list;
    stack->list = elt->next;
    stack->nbevts --;
    pthread_mutex_unlock(&(stack->m));
    
    return elt;
}
