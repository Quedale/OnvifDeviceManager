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
 */

#include "custom_gtk_progress_tracker.h"
// #include "gtkprogresstrackerprivate.h"
// #include "gtkprivate.h"
// #include "gtkcsseasevalueprivate.h"

#include <math.h>
#include <string.h>

/*
 * Progress tracker is small helper for tracking progress through gtk
 * animations. It's a simple zero-initable struct, meant to be thrown in a
 * widget's private data without the need for setup or teardown.
 *
 * Progress tracker will handle translating frame clock timestamps to a
 * fractional progress value for interpolating between animation targets.
 *
 * Progress tracker will use the GTK_SLOWDOWN environment variable to control
 * the speed of animations. This can be useful for debugging.
 */

static gdouble custom_slowdown = 1.0;

void
_custom_set_slowdown (gdouble factor)
{
  custom_slowdown = factor;
}

gdouble
_custom_get_slowdown (void)
{
  return custom_slowdown;
}

/**
 * custom_gtk_progress_tracker_init_copy:
 * @source: The source progress tracker
 * @dest: The destination progress tracker
 *
 * Copy all progress tracker state from the source tracker to dest tracker.
 **/
void
custom_gtk_progress_tracker_init_copy (CustomGtkProgressTracker *source,
                                CustomGtkProgressTracker *dest)
{
  memcpy (dest, source, sizeof (CustomGtkProgressTracker));
}

/**
 * custom_gtk_progress_tracker_start:
 * @tracker: The progress tracker
 * @duration: Animation duration in us
 * @delay: Animation delay in us
 * @iteration_count: Number of iterations to run the animation, must be >= 0
 *
 * Begins tracking progress for a new animation. Clears all previous state.
 **/
void
custom_gtk_progress_tracker_start (CustomGtkProgressTracker *tracker,
                            guint64 duration,
                            gint64 delay,
                            gdouble iteration_count)
{
  tracker->is_running = TRUE;
  tracker->last_frame_time = 0;
  tracker->duration = duration;
  tracker->iteration = - delay / (gdouble) MAX (duration, 1);
  tracker->iteration_count = iteration_count;
}

/**
 * custom_gtk_progress_tracker_finish:
 * @tracker: The progress tracker
 *
 * Stops running the current animation.
 **/
void
custom_gtk_progress_tracker_finish (CustomGtkProgressTracker *tracker)
{
  tracker->is_running = FALSE;
}

/**
 * custom_gtk_progress_tracker_advance_frame:
 * @tracker: The progress tracker
 * @frame_time: The current frame time, usually from the frame clock.
 *
 * Increments the progress of the animation forward a frame. If no animation has
 * been started, does nothing.
 **/
void
custom_gtk_progress_tracker_advance_frame (CustomGtkProgressTracker *tracker,
                                    guint64             frame_time)
{
  gdouble delta;

  if (!tracker->is_running)
    return;

  if (tracker->last_frame_time == 0)
    {
      tracker->last_frame_time = frame_time;
      return;
    }

  if (frame_time < tracker->last_frame_time)
    {
      g_warning ("Progress tracker frame set backwards, ignoring.");
      return;
    }

  delta = (frame_time - tracker->last_frame_time) / custom_slowdown / MAX (tracker->duration, 1);
  tracker->last_frame_time = frame_time;
  tracker->iteration += delta;
}

/**
 * custom_gtk_progress_tracker_skip_frame:
 * @tracker: The progress tracker
 * @frame_time: The current frame time, usually from the frame clock.
 *
 * Does not update the progress of the animation forward, but records the frame
 * to calculate future deltas. Calling this each frame will effectively pause
 * the animation.
 **/
void
custom_gtk_progress_tracker_skip_frame (CustomGtkProgressTracker *tracker,
                                 guint64 frame_time)
{
  if (!tracker->is_running)
    return;

  tracker->last_frame_time = frame_time;
}

/**
 * custom_gtk_progress_tracker_get_state:
 * @tracker: The progress tracker
 *
 * Returns whether the tracker is before, during or after the currently started
 * animation. The tracker will only ever be in the before state if the animation
 * was started with a delay. If no animation has been started, returns
 * %CUSTOM_GTK_PROGRESS_STATE_AFTER.
 *
 * Returns: A CustomGtkProgressState
 **/
CustomGtkProgressState
custom_gtk_progress_tracker_get_state (CustomGtkProgressTracker *tracker)
{
  if (!tracker->is_running || tracker->iteration > tracker->iteration_count)
    return CUSTOM_GTK_PROGRESS_STATE_AFTER;
  if (tracker->iteration < 0)
    return CUSTOM_GTK_PROGRESS_STATE_BEFORE;
  return CUSTOM_GTK_PROGRESS_STATE_DURING;
}

/**
 * custom_gtk_progress_tracker_get_iteration:
 * @tracker: The progress tracker
 *
 * Returns the fractional number of cycles the animation has completed. For
 * example, it you started an animation with iteration-count of 2 and are half
 * way through the second animation, this returns 1.5.
 *
 * Returns: The current iteration.
 **/
gdouble
custom_gtk_progress_tracker_get_iteration (CustomGtkProgressTracker *tracker)
{
  return tracker->is_running ? CLAMP (tracker->iteration, 0.0, tracker->iteration_count) : 1.0;
}

/**
 * custom_gtk_progress_tracker_get_iteration_cycle:
 * @tracker: The progress tracker
 *
 * Returns an integer index of the current iteration cycle tracker is
 * progressing through. Handles edge cases, such as an iteration value of 2.0
 * which could be considered the end of the second iteration of the beginning of
 * the third, in the same way as custom_gtk_progress_tracker_get_progress().
 *
 * Returns: The integer count of the current animation cycle.
 **/
guint64
custom_gtk_progress_tracker_get_iteration_cycle (CustomGtkProgressTracker *tracker)
{
  gdouble iteration = custom_gtk_progress_tracker_get_iteration (tracker);

  /* Some complexity here. We want an iteration of 0.0 to always map to 0 (start
   * of the first iteration), but an iteration of 1.0 to also map to 0 (end of
   * first iteration) and 2.0 to 1 (end of the second iteration).
   */
  if (iteration == 0.0)
    return 0;

  return (guint64) ceil (iteration) - 1;
}

/**
 * custom_gtk_progress_tracker_get_progress:
 * @tracker: The progress tracker
 * @reversed: If progress should be reversed.
 *
 * Gets the progress through the current animation iteration, from [0, 1]. Use
 * to interpolate between animation targets. If reverse is true each iteration
 * will begin at 1 and end at 0.
 *
 * Returns: The progress value.
 **/
gdouble
custom_gtk_progress_tracker_get_progress (CustomGtkProgressTracker *tracker,
                                   gboolean reversed)
{
  gdouble progress, iteration;
  guint64 iteration_cycle;

  iteration = custom_gtk_progress_tracker_get_iteration (tracker);
  iteration_cycle = custom_gtk_progress_tracker_get_iteration_cycle (tracker);

  progress = iteration - iteration_cycle;
  return reversed ? 1.0 - progress : progress;
}

/* From clutter-easing.c, based on Robert Penner's
 * infamous easing equations, MIT license.
 */
static gdouble
ease_out_cubic (gdouble t)
{
  gdouble p = t - 1;
  return p * p * p + 1;
}

/**
 * custom_gtk_progress_tracker_get_ease_out_cubic:
 * @tracker: The progress tracker
 * @reversed: If progress should be reversed before applying the ease function.
 *
 * Applies a simple ease out cubic function to the result of
 * custom_gtk_progress_tracker_get_progress().
 *
 * Returns: The eased progress value.
 **/
gdouble
custom_gtk_progress_tracker_get_ease_out_cubic (CustomGtkProgressTracker *tracker,
                                         gboolean reversed)
{
  gdouble progress = custom_gtk_progress_tracker_get_progress (tracker, reversed);
  return ease_out_cubic (progress);
}
