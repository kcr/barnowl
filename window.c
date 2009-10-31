#include "owl.h"

owl_window *owl_window_new(int nlines, int ncols, int begin_y, int begin_x)
{
  WINDOW *win;
  owl_window *w = owl_malloc(sizeof(owl_window));

  win = newwin(nlines, ncols, begin_y, begin_x);
  if (win == NULL)
    return NULL;

  w->win = win;
  idlok(w->win, FALSE);
  
  w->dosearch = 0;

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

/*
 * Local Variables:
 * mode:C
 * c-basic-offset:2
 * End:
 */
