#include "owl.h"

owl_window *owl_window_new(int nlines, int ncols, int begin_y, int begin_x)
{
  return owl_window_new_prefix(nlines, ncols, begin_y, begin_x, NULL);
}

owl_window *owl_window_new_prefix(int nlines, int ncols, int begin_y, int begin_x, const char *prefix)
{
  WINDOW *win;
  owl_window *w = owl_malloc(sizeof(owl_window));

  win = newwin(nlines, ncols, begin_y, begin_x);
  if (win == NULL)
    return NULL;

  w->win = win;
  idlok(w->win, FALSE);
  
  w->dosearch = 0;
  w->foreground = OWL_COLOR_DEFAULT;
  w->background = OWL_COLOR_DEFAULT;
  if (prefix != NULL)
    w->prefix = owl_strdup(prefix);
  else
    w->prefix = NULL;

  owl_window_update_config(w);

  return w;
}

void owl_window_delete(owl_window *w)
{
  if (w->win != NULL)
    delwin(w->win);

  free(w);
}

void owl_window_doinput(owl_window *w)
{
  nodelay(w->win, 1);
  keypad(w->win, TRUE);
  meta(w->win, TRUE);
}

static const char *ow_prefix_var(const char *prefix, const char *suffix)
{
  char *name;
  const char *result;

  name = owl_sprintf("%s_%s", prefix, suffix);
  result = owl_variable_get_string(&g.vars, name);
  owl_free(name);
  return result;
}

void owl_window_update_config(owl_window *w)
{
  if (w->prefix != NULL) {
    w->foreground = owl_util_string_to_color(ow_prefix_var(w->prefix, "foreground"));
    w->background = owl_util_string_to_color(ow_prefix_var(w->prefix, "background"));

    owl_window_set_colors(w);
  }
}

void owl_window_set_colors(owl_window *w)
{
  if (w == NULL || w->win == NULL)
    return;

  wbkgdset(w->win, COLOR_PAIR(owl_fmtext_get_colorpair(w->foreground, w->background)));
}

/*
 * Local Variables:
 * mode:C
 * c-basic-offset:2
 * End:
 */
