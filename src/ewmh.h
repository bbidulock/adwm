#ifndef __LOCAL_EWMH_H__
#define __LOCAL_EWMH_H__

/* ewmh.c */
Bool checkatom(Window win, Atom bigatom, Atom smallatom);
unsigned getwintype(Window win);
Bool checkwintype(Window win, int wintype);
Bool clientmessage(XEvent *e);
void ewmh_release_user_time_window(Client *c);
Atom *getatom(Window win, Atom atom, unsigned long *nitems);
long *getcard(Window win, Atom atom, unsigned long *nitems);
void initewmh(char *name);
void exitewmh(WithdrawCause cause);
void ewmh_add_client(Client *c);
void ewmh_del_client(Client *c, WithdrawCause cause);
void setopacity(Client *c, unsigned opacity);
int getstruts(Client *c);

void ewmh_process_net_desktop_names(void);
void ewmh_process_net_number_of_desktops(void);
void ewmh_process_net_showing_desktop(void);
void ewmh_process_net_desktop_layout(void);
void ewmh_update_kde_splash_progress(void);
void ewmh_update_net_active_window(void);
void ewmh_update_net_client_list(void);
void ewmh_update_net_current_desktop(void);
void ewmh_update_net_desktop_geometry(void);
void ewmh_update_net_desktop_modes(void);
void ewmh_update_net_number_of_desktops(void);
void ewmh_update_net_showing_desktop(void);
void ewmh_update_net_virtual_roots(void);
void ewmh_update_net_work_area(void);
void ewmh_update_net_desktop_layout(void);

void mwmh_process_motif_wm_hints(Client *);

Bool ewmh_process_net_window_desktop(Client *);
Bool ewmh_process_net_window_desktop_mask(Client *);

void ewmh_process_net_window_type(Client *);
void ewmh_process_kde_net_window_type_override(Client *);
void ewmh_process_net_startup_id(Client *);
void ewmh_process_net_window_state(Client *c);
void ewmh_process_net_window_sync_request_counter(Client *);
void ewmh_process_net_window_user_time(Client *);
void ewmh_process_net_window_user_time_window(Client *);
void ewmh_update_net_window_desktop(Client *);
void ewmh_update_net_window_extents(Client *);
void ewmh_update_net_window_fs_monitors(Client *);
void ewmh_update_net_window_state(Client *);
void ewmh_update_net_window_visible_icon_name(Client *);
void ewmh_update_net_window_visible_name(Client *);
void wmh_process_win_window_hints(Client *);
void wmh_process_win_layer(Client *);

#endif				/* __LOCAL_EWMH_H__ */
