#include "owl.h"

int owl_popwin_init(owl_popwin *pw)
{
  pw->active=0;
  pw->needsfirstrefresh=0;
  pw->lines=0;
  pw->cols=0;
  return(0);
}

int owl_popwin_up(owl_popwin *pw)
{
  int glines, gcols, startcol, startline;

  /* calculate the size of the popwin */
  glines=owl_global_get_lines(&g);
  gcols=owl_global_get_cols(&g);

  pw->lines = owl_util_min(glines,24)*3/4 + owl_util_max(glines-24,0)/2;
  startline = (glines-pw->lines)/2;

  pw->cols = owl_util_min(gcols,90)*15/16 + owl_util_max(gcols-90,0)/2;
  startcol = (gcols-pw->cols)/2;

  pw->borderwin = owl_window_new(pw->lines, pw->cols, startline, startcol);
  pw->popwin = owl_window_new(pw->lines-2, pw->cols-2, startline+1, startcol+1);
  pw->needsfirstrefresh = 1;

  owl_window_doinput(pw->popwin);

  werase(pw->popwin->win);
  werase(pw->borderwin->win);
  if (owl_global_is_fancylines(&g)) {
    box(pw->borderwin->win, 0, 0);
  } else {
    box(pw->borderwin->win, '|', '-');
    wmove(pw->borderwin->win, 0, 0);
    waddch(pw->borderwin->win, '+');
    wmove(pw->borderwin->win, pw->lines - 1, 0);
    waddch(pw->borderwin->win, '+');
    wmove(pw->borderwin->win, pw->lines - 1, pw->cols - 1);
    waddch(pw->borderwin->win, '+');
    wmove(pw->borderwin->win, 0, pw->cols - 1);
    waddch(pw->borderwin->win, '+');
  }
    
  wnoutrefresh(pw->popwin->win);
  wnoutrefresh(pw->borderwin->win);
  owl_global_set_needrefresh(&g);
  pw->active = 1;

  return 0;
}

int owl_popwin_close(owl_popwin *pw)
{
  owl_window_delete(pw->popwin);
  owl_window_delete(pw->borderwin);
  pw->active=0;
  owl_global_set_needrefresh(&g);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_function_full_redisplay();
  return(0);
}

int owl_popwin_is_active(const owl_popwin *pw)
{
  if (pw->active==1) return(1);
  return(0);
}

/* this will refresh the border as well as the text area */
int owl_popwin_refresh(const owl_popwin *pw)
{
  touchwin(pw->borderwin->win);
  touchwin(pw->popwin->win);

  wnoutrefresh(pw->borderwin->win);
  wnoutrefresh(pw->popwin->win);
  owl_global_set_needrefresh(&g);
  return(0);
}

WINDOW *owl_popwin_get_curswin(const owl_popwin *pw)
{
  return(pw->popwin->win);
}

int owl_popwin_get_lines(const owl_popwin *pw)
{
  return(pw->lines-2);
}

int owl_popwin_get_cols(const owl_popwin *pw)
{
  return(pw->cols-2);
}

int owl_popwin_needs_first_refresh(const owl_popwin *pw)
{
  if (pw->needsfirstrefresh) return(1);
  return(0);
}

void owl_popwin_no_needs_first_refresh(owl_popwin *pw)
{
  pw->needsfirstrefresh=0;
}
