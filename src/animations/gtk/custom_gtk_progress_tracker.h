/*
 * Copyright © 2016 Endless Mobile Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthew Watson <mattdangerw@gmail.com>
 * 
 */


/* 
 * This file was imported as-is in order to compile CustomGtkRevealer
 * 
 */

#ifndef __GTK_CUSTOM_GTK_PROGRESS_TRACKER_PRIVATE_H__
#define __GTK_CUSTOM_GTK_PROGRESS_TRACKER_PRIVATE_H__

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  CUSTOM_GTK_PROGRESS_STATE_BEFORE,
  CUSTOM_GTK_PROGRESS_STATE_DURING,
  CUSTOM_GTK_PROGRESS_STATE_AFTER,
} CustomGtkProgressState;

typedef struct _CustomGtkProgressTracker CustomGtkProgressTracker;

struct _CustomGtkProgressTracker
{
  gboolean is_running;
  guint64 last_frame_time;
  guint64 duration;
  gdouble iteration;
  gdouble iteration_count;
};

void                 custom_gtk_progress_tracker_init_copy           (CustomGtkProgressTracker *source,
                                                               CustomGtkProgressTracker *dest);

void                 custom_gtk_progress_tracker_start               (CustomGtkProgressTracker *tracker,
                                                               guint64 duration,
                                                               gint64 delay,
                                                               gdouble iteration_count);

void                 custom_gtk_progress_tracker_finish              (CustomGtkProgressTracker *tracker);

void                 custom_gtk_progress_tracker_advance_frame       (CustomGtkProgressTracker *tracker,
                                                               guint64 frame_time);

void                 custom_gtk_progress_tracker_skip_frame          (CustomGtkProgressTracker *tracker,
                                                               guint64 frame_time);

CustomGtkProgressState     custom_gtk_progress_tracker_get_state           (CustomGtkProgressTracker *tracker);

gdouble              custom_gtk_progress_tracker_get_iteration       (CustomGtkProgressTracker *tracker);

guint64              custom_gtk_progress_tracker_get_iteration_cycle (CustomGtkProgressTracker *tracker);

gdouble              custom_gtk_progress_tracker_get_progress        (CustomGtkProgressTracker *tracker,
                                                               gboolean reverse);

gdouble              custom_gtk_progress_tracker_get_ease_out_cubic  (CustomGtkProgressTracker *tracker,
                                                               gboolean reverse);

G_END_DECLS

#endif /* __GTK_CUSTOM_GTK_PROGRESS_TRACKER_PRIVATE_H__ */
