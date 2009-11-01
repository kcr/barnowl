#include <string.h>
#include "owl.h"

#define BOTTOM_OFFSET 1

/* initialize the viewwin e.  'win' is an already initialzed curses
 * window that will be used by viewwin
 */
void owl_viewwin_init_text(owl_viewwin *v, owl_window *w, int winlines, int wincols, const char *text)
{
  owl_fmtext_init_null(&(v->fmtext));
  if (text) {
    owl_fmtext_append_normal(&(v->fmtext), text);
    if (text[0] != '\0' && text[strlen(text) - 1] != '\n') {
      owl_fmtext_append_normal(&(v->fmtext), "\n");
    }
    v->textlines=owl_fmtext_num_lines(&(v->fmtext));
  }
  v->topline=0;
  v->rightshift=0;
  v->winlines=winlines;
  v->wincols=wincols;
  v->win = w;
  v->onclose_hook = NULL;
}

void owl_viewwin_append_text(owl_viewwin *v, const char *text) {
    owl_fmtext_append_normal(&(v->fmtext), text);
    v->textlines=owl_fmtext_num_lines(&(v->fmtext));  
}

/* initialize the viewwin e.  'win' is an already initialzed curses
 * window that will be used by viewwin
 */
void owl_viewwin_init_fmtext(owl_viewwin *v, owl_window *w, int winlines, int wincols, const owl_fmtext *fmtext)
{
  char *text;

  owl_fmtext_copy(&(v->fmtext), fmtext);
  text = owl_fmtext_print_plain(fmtext);
  if (text[0] != '\0' && text[strlen(text) - 1] != '\n') {
      owl_fmtext_append_normal(&(v->fmtext), "\n");
  }
  v->textlines=owl_fmtext_num_lines(&(v->fmtext));
  v->topline=0;
  v->rightshift=0;
  v->winlines=winlines;
  v->wincols=wincols;
  v->win = w;
}

void owl_viewwin_set_onclose_hook(owl_viewwin *v, void (*onclose_hook) (owl_viewwin *vwin, void *data), void *onclose_hook_data) {
  v->onclose_hook = onclose_hook;
  v->onclose_hook_data = onclose_hook_data;
}

/* regenerate text on the curses window. */
/* if update == 1 then do a doupdate() */
void owl_viewwin_redisplay(owl_viewwin *v, int update)
{
  owl_fmtext fm1, fm2;
  
  werase(v->win->win);
  wmove(v->win->win, 0, 0);

  owl_fmtext_init_null(&fm1);
  owl_fmtext_init_null(&fm2);
  
  owl_fmtext_truncate_lines(&(v->fmtext), v->topline, v->winlines-BOTTOM_OFFSET, &fm1);
  owl_fmtext_truncate_cols(&fm1, v->rightshift, v->wincols-1+v->rightshift, &fm2);

  owl_fmtext_ow_addstr(&fm2, v->win);

  /* print the message at the bottom */
  wmove(v->win->win, v->winlines-1, 0);
  wattrset(v->win->win, A_REVERSE);
  if (v->textlines - v->topline > v->winlines-BOTTOM_OFFSET) {
    waddstr(v->win->win, "--More-- (Space to see more, 'q' to quit)");
  } else {
    waddstr(v->win->win, "--End-- (Press 'q' to quit)");
  }
  wattroff(v->win->win, A_REVERSE);
  wnoutrefresh(v->win->win);

  if (update==1) {
    doupdate();
  }

  owl_fmtext_free(&fm1);
  owl_fmtext_free(&fm2);
}

void owl_viewwin_pagedown(owl_viewwin *v)
{
  v->topline+=v->winlines - BOTTOM_OFFSET;
  if ( (v->topline+v->winlines-BOTTOM_OFFSET) > v->textlines) {
    v->topline = v->textlines - v->winlines + BOTTOM_OFFSET;
  }
}

void owl_viewwin_linedown(owl_viewwin *v)
{
  v->topline++;
  if ( (v->topline+v->winlines-BOTTOM_OFFSET) > v->textlines) {
    v->topline = v->textlines - v->winlines + BOTTOM_OFFSET;
  }
}

void owl_viewwin_pageup(owl_viewwin *v)
{
  v->topline-=v->winlines;
  if (v->topline<0) v->topline=0;
}

void owl_viewwin_lineup(owl_viewwin *v)
{
  v->topline--;
  if (v->topline<0) v->topline=0;
}

void owl_viewwin_right(owl_viewwin *v, int n)
{
  v->rightshift+=n;
}

void owl_viewwin_left(owl_viewwin *v, int n)
{
  v->rightshift-=n;
  if (v->rightshift<0) v->rightshift=0;
}

void owl_viewwin_top(owl_viewwin *v)
{
  v->topline=0;
  v->rightshift=0;
}

void owl_viewwin_bottom(owl_viewwin *v)
{
  v->topline = v->textlines - v->winlines + BOTTOM_OFFSET;
}

void owl_viewwin_free(owl_viewwin *v)
{
  if (v->onclose_hook) {
    v->onclose_hook(v, v->onclose_hook_data);
    v->onclose_hook = NULL;
    v->onclose_hook_data = NULL;
  }
  owl_fmtext_free(&(v->fmtext));
}
