#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-dlggen.h"

#include "../ptrlist.h"

static HMENU g_swell_defaultmenu,g_swell_defaultmenumodal;

void (*SWELL_DDrop_onDragLeave)();
void (*SWELL_DDrop_onDragOver)(POINT pt);
void (*SWELL_DDrop_onDragEnter)(void *hGlobal, POINT pt);
const char* (*SWELL_DDrop_getDroppedFileTargetPath)(const char* extension);

bool SWELL_owned_windows_levelincrease=false;

#include "swell-internal.h"

static DWORD s_lastMessagePos;
HWND ChildWindowFromPoint(HWND h, POINT p);
bool IsWindowEnabled(HWND hwnd);

static int swell_gdkConvertKey(int key)
{
  //gdk key to VK_ conversion
  switch(key)
  {
  case GDK_KEY_Home: key = VK_HOME; break;
  case GDK_KEY_End: key = VK_END; break;
  case GDK_KEY_Up: key = VK_UP; break;
  case GDK_KEY_Down: key = VK_DOWN; break;
  case GDK_KEY_Left: key = VK_LEFT; break;
  case GDK_KEY_Right: key = VK_RIGHT; break;
  case GDK_KEY_Page_Up: key = VK_PRIOR; break;
  case GDK_KEY_Page_Down: key = VK_NEXT; break;
  case GDK_KEY_Insert: key = VK_INSERT; break;
  case GDK_KEY_Delete: key = VK_DELETE; break;
  case GDK_KEY_Escape: key = VK_ESCAPE; break;
  }
  return key;
}

static LRESULT SendMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd || !hwnd->m_wndproc) return -1;
  if (!IsWindowEnabled(hwnd)) 
  {
    HWND DialogBoxIsActive();
    HWND h = DialogBoxIsActive();
    if (h) SetForegroundWindow(h);
    return -1;
  }

  LRESULT htc;
  if (msg != WM_MOUSEWHEEL && !GetCapture())
  {
    DWORD p=GetMessagePos(); 

    htc=hwnd->m_wndproc(hwnd,WM_NCHITTEST,0,p); 
    if (hwnd->m_hashaddestroy||!hwnd->m_wndproc) 
    {
      return -1; // if somehow WM_NCHITTEST destroyed us, bail
    }
     
    if (htc!=HTCLIENT) 
    {
      if (msg==WM_MOUSEMOVE) return hwnd->m_wndproc(hwnd,WM_NCMOUSEMOVE,htc,p); 
      if (msg==WM_LBUTTONUP) return hwnd->m_wndproc(hwnd,WM_NCLBUTTONUP,htc,p); 
      if (msg==WM_LBUTTONDOWN) return hwnd->m_wndproc(hwnd,WM_NCLBUTTONDOWN,htc,p); 
      if (msg==WM_LBUTTONDBLCLK) return hwnd->m_wndproc(hwnd,WM_NCLBUTTONDBLCLK,htc,p); 
      if (msg==WM_RBUTTONUP) return hwnd->m_wndproc(hwnd,WM_NCRBUTTONUP,htc,p); 
      if (msg==WM_RBUTTONDOWN) return hwnd->m_wndproc(hwnd,WM_NCRBUTTONDOWN,htc,p); 
      if (msg==WM_RBUTTONDBLCLK) return hwnd->m_wndproc(hwnd,WM_NCRBUTTONDBLCLK,htc,p); 
      if (msg==WM_MBUTTONUP) return hwnd->m_wndproc(hwnd,WM_NCMBUTTONUP,htc,p); 
      if (msg==WM_MBUTTONDOWN) return hwnd->m_wndproc(hwnd,WM_NCMBUTTONDOWN,htc,p); 
      if (msg==WM_MBUTTONDBLCLK) return hwnd->m_wndproc(hwnd,WM_NCMBUTTONDBLCLK,htc,p); 
    } 
  }


  LRESULT ret=hwnd->m_wndproc(hwnd,msg,wParam,lParam);

  if (msg==WM_LBUTTONUP || msg==WM_RBUTTONUP || msg==WM_MOUSEMOVE || msg==WM_MBUTTONUP) 
  {
    if (!GetCapture() && (hwnd->m_hashaddestroy || !hwnd->m_wndproc || !hwnd->m_wndproc(hwnd,WM_SETCURSOR,(WPARAM)hwnd,htc | (msg<<16))))    
    {
       // todo: set default cursor
    }
  }

  return ret;
}

static gboolean swell_gtkDraw(GtkWidget *widget, cairo_t *crc, gpointer data)
{
#ifdef SWELL_LICE_GDI
  // super slow
  RECT r,cr;

  // don't use GetClientRect(),since we're getting it pre-NCCALCSIZE etc
  cr.right = gtk_widget_get_allocated_width(widget);
  cr.bottom = gtk_widget_get_allocated_height(widget);
  cr.left=cr.top=0;

  double left, top, right, bottom;
  cairo_clip_extents(crc, &left, &top, &right, &bottom);
  r.left = left;
  r.top = top;
  r.right = right;
  r.bottom = bottom;

  HWND hwnd = (HWND)data;
  if (!hwnd->m_backingstore)
    hwnd->m_backingstore = new LICE_MemBitmap;

  bool forceref = hwnd->m_backingstore->resize(cr.right-cr.left,cr.bottom-cr.top);
  if (forceref) r = cr;

  LICE_SubBitmap tmpbm(hwnd->m_backingstore,r.left,r.top,r.right-r.left,r.bottom-r.top);

  if (tmpbm.getWidth()>0 && tmpbm.getHeight()>0) 
  {
    void SWELL_internalLICEpaint(HWND hwnd, LICE_IBitmap *bmout, int bmout_xpos, int bmout_ypos, bool forceref);
    SWELL_internalLICEpaint(hwnd, &tmpbm, r.left, r.top, forceref);
    cairo_surface_t *temp_surface = cairo_image_surface_create_for_data((guchar*)tmpbm.getBits(), CAIRO_FORMAT_RGB24, tmpbm.getWidth(),tmpbm.getHeight(), tmpbm.getRowSpan()*4);
    cairo_set_source_surface(crc, temp_surface, r.left, r.top);
    cairo_paint(crc);
    cairo_surface_destroy(temp_surface);
  }
#endif

  return TRUE;
}

static gboolean swell_gtkDeleteEvent(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  HWND hwnd = (HWND)data;
  if (IsWindowEnabled(hwnd) && !SendMessage(hwnd,WM_CLOSE,0,0))
    SendMessage(hwnd,WM_COMMAND,IDCANCEL,0);

  return TRUE;
}

static gboolean swell_gtkConfigureEvent(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  HWND hwnd = (HWND)data;

  GdkEventConfigure *cfg = (GdkEventConfigure*)event;
  int flag=0;
  if (cfg->x != hwnd->m_position.left || cfg->y != hwnd->m_position.top)  flag|=1;
  if (cfg->width != hwnd->m_position.right-hwnd->m_position.left || cfg->height != hwnd->m_position.bottom - hwnd->m_position.top) flag|=2;
  hwnd->m_position.left = cfg->x;
  hwnd->m_position.top = cfg->y;
  hwnd->m_position.right = cfg->x + cfg->width;
  hwnd->m_position.bottom = cfg->y + cfg->height;

  if (flag&1) SendMessage(hwnd, WM_MOVE, 0, MAKELPARAM(cfg->x, cfg->y));
  if (flag&2) SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(cfg->width, cfg->height));

  return TRUE;
}

static void swell_gdkEventHandler(GtkWidget *widget, GdkEvent *evt, gpointer data)
{
  HWND hwnd = (HWND)data;

  // if (evt->type == GDK_FOCUS_CHANGE)
  // {
  //   GdkEventFocus *fc = (GdkEventFocus *)evt;
  //   if (fc->in) SWELL_g_focus_oswindow = hwnd ? fc->window : NULL;
  // }

  if (hwnd) switch (evt->type)
  {
  case GDK_WINDOW_STATE: /// GdkEventWindowState for min/max
    printf("minmax\n");
    break;
  // case GDK_GRAB_BROKEN:
  //   {
  //     GdkEventGrabBroken *bk = (GdkEventGrabBroken*)evt;
  //     if (s_captured_window)
  // 	{
  // 	  SendMessage(s_captured_window,WM_CAPTURECHANGED,0,0);
  // 	  s_captured_window=0;
  // 	}
  //   }
  //   break;
  case GDK_KEY_PRESS:
  case GDK_KEY_RELEASE:
    { // todo: pass through app-specific default processing before sending to child window
      GdkEventKey *k = (GdkEventKey *)evt;
      //printf("key%s: %d %s\n", evt->type == GDK_KEY_PRESS ? "down" : "up", k->keyval, k->string);
      int modifiers = FVIRTKEY;
      if (k->state&GDK_SHIFT_MASK) modifiers|=FSHIFT;
      if (k->state&GDK_CONTROL_MASK) modifiers|=FCONTROL;
      if (k->state&GDK_MOD1_MASK) modifiers|=FALT;

      int kv = swell_gdkConvertKey(k->keyval);
      kv=toupper(kv);

      HWND foc = GetFocus();
      if (foc && IsChild(hwnd,foc)) hwnd=foc;
      MSG msg = { hwnd, evt->type == GDK_KEY_PRESS ? WM_KEYDOWN : WM_KEYUP, kv, modifiers, };
      if (SWELLAppMain(SWELLAPP_PROCESSMESSAGE,(INT_PTR)&msg,0)<=0)
	SendMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
    }
    break;
  case GDK_MOTION_NOTIFY:
    {
      GdkEventMotion *m = (GdkEventMotion *)evt;
      s_lastMessagePos = MAKELONG(((int)m->x_root&0xffff),((int)m->y_root&0xffff));
      //            printf("motion %d %d %d %d\n", (int)m->x, (int)m->y, (int)m->x_root, (int)m->y_root); 
      POINT p={m->x, m->y};
      HWND hwnd2 = GetCapture();
      if (!hwnd2) hwnd2=ChildWindowFromPoint(hwnd, p);
      //char buf[1024];
      //GetWindowText(hwnd2,buf,sizeof(buf));
      //           printf("%x %s\n", hwnd2,buf);
      POINT p2={m->x_root, m->y_root};
      ScreenToClient(hwnd2, &p2);
      //printf("%d %d\n", p2.x, p2.y);
      if (hwnd2) hwnd2->Retain();
      SendMouseMessage(hwnd2, WM_MOUSEMOVE, 0, MAKELPARAM(p2.x, p2.y));
      if (hwnd2) hwnd2->Release();
      gdk_event_request_motions(m);
    }
    break;
  case GDK_BUTTON_PRESS:
  case GDK_2BUTTON_PRESS:
  case GDK_BUTTON_RELEASE:
    {
      //printf("ButtonEvent for %s\n", hwnd->m_title);
      GdkEventButton *b = (GdkEventButton *)evt;
      s_lastMessagePos = MAKELONG(((int)b->x_root&0xffff),((int)b->y_root&0xffff));

      POINT p={b->x, b->y};
      HWND hwnd2 = GetCapture();
      if (!hwnd2)
	hwnd2=ChildWindowFromPoint(hwnd, p);

      POINT p2={b->x_root, b->y_root};
      ScreenToClient(hwnd2, &p2);

      int msg=WM_LBUTTONDOWN;
      if (b->button==2)
	msg=WM_MBUTTONDOWN;
      else if (b->button==3)
	msg=WM_RBUTTONDOWN;
            
      // if (hwnd && hwnd->m_oswindow && 
      // 	  SWELL_g_focus_oswindow != hwnd->m_oswindow)
      // 	SWELL_g_focus_oswindow = hwnd->m_oswindow;

      if(evt->type == GDK_BUTTON_RELEASE)
	msg++; // move from down to up
      else if(evt->type == GDK_2BUTTON_PRESS)
	msg+=2; // move from down to up

      if (hwnd2) hwnd2->Retain();
      SendMouseMessage(hwnd2, msg, 0, MAKELPARAM(p2.x, p2.y));
      if (hwnd2) hwnd2->Release();
    }
    break;
  default:
    //printf("msg: %d\n",evt->type);
    break;
  }
}

DWORD GetMessagePos()
{  
  return s_lastMessagePos;
}

static SWELL_DialogResourceIndex *resById(SWELL_DialogResourceIndex *reshead, const char *resid)
{
  SWELL_DialogResourceIndex *p=reshead;
  while (p)
  {
    if (p->resid == resid) return p;
    p=p->_next;
  }
  return 0;
}

// keep list of modal dialogs
struct modalDlgRet { 
  HWND hwnd; 
  bool has_ret;
  int ret;
};


static WDL_PtrList<modalDlgRet> s_modalDialogs;

HWND DialogBoxIsActive()
{
  return s_modalDialogs.GetSize() ? s_modalDialogs.Get(s_modalDialogs.GetSize()-1)->hwnd : NULL;
}

void EndDialog(HWND wnd, int ret)
{   
  if (!wnd) return;
  
  int x;
  for (x = 0; x < s_modalDialogs.GetSize(); x ++)
    if (s_modalDialogs.Get(x)->hwnd == wnd)  
    {
      s_modalDialogs.Get(x)->has_ret=true;
      s_modalDialogs.Get(x)->ret = ret;
    }
  DestroyWindow(wnd);
  // todo
}

int SWELL_DialogBox(SWELL_DialogResourceIndex *reshead, const char *resid, HWND parent,  DLGPROC dlgproc, LPARAM param)
{
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (resid) // allow modal dialogs to be created without template
  {
    if (!p||(p->windowTypeFlags&SWELL_DLG_WS_CHILD)) return -1;
  }


  int ret=-1;
  HWND hwnd = SWELL_CreateDialog(reshead,resid,parent,dlgproc,param);
  // create dialog
  if (hwnd)
  {
    ReleaseCapture(); // force end of any captures

    WDL_PtrList<HWND__> enwnds;
    extern HWND__ *SWELL_topwindows;
    HWND a = SWELL_topwindows;
    while (a)
    {
      if (a->m_enabled && a != hwnd) { EnableWindow(a,FALSE); enwnds.Add(a); }
      a = a->m_next;
    }

    modalDlgRet r = { hwnd,false, -1 };
    s_modalDialogs.Add(&r);
    ShowWindow(hwnd,SW_SHOW);
    while (s_modalDialogs.Find(&r)>=0 && !r.has_ret)
    {
      void SWELL_RunMessageLoop();
      SWELL_RunMessageLoop();
      Sleep(10);
    }
    ret=r.ret;
    s_modalDialogs.Delete(s_modalDialogs.Find(&r));

    a = SWELL_topwindows;
    while (a)
    {
      if (!a->m_enabled && a != hwnd && enwnds.Find(a)>=0) EnableWindow(a,TRUE);
      a = a->m_next;
    }
  }
  // while in list, do something
  return ret;
}

HWND SWELL_CreateDialog(SWELL_DialogResourceIndex *reshead, const char *resid, HWND parent, DLGPROC dlgproc, LPARAM param)
{
  int forceStyles=0; // 1=resizable, 2=no minimize, 4=no close
  bool forceNonChild=false;
  if ((((INT_PTR)resid)&~0xf)==0x400000)
  {
    forceStyles = (int) (((INT_PTR)resid)&0xf);
    if (forceStyles) forceNonChild=true;
    resid=0;
  }
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (!p&&resid) return 0;
  
  RECT r={0,0,p?p->width : 300, p?p->height : 200};

  HWND__ *h;

  // Create GtkEventBox for dialog windows
  GtkWidget *event_box = gtk_event_box_new();
  gtk_widget_set_app_paintable(event_box, TRUE);

  // Child or top level window?
  if (!forceNonChild && parent && (!p || (p->windowTypeFlags&SWELL_DLG_WS_CHILD)))
  {
    // Child window
    h = new HWND__(parent,0,&r,NULL,false,NULL,NULL);
    h->m_oswindow = event_box;
    // Check if parent already has a container
    GtkWidget *fixed = gtk_bin_get_child(GTK_BIN(parent->m_oswindow));
    if (!fixed)
    {
      fixed = gtk_fixed_new();
      gtk_container_add(GTK_CONTAINER(parent->m_oswindow), fixed);
      gtk_widget_show(fixed);
    }
    // Add child window to parent at specified position
    gtk_fixed_put(GTK_FIXED(fixed), h->m_oswindow, r.left, r.top);
  } 
  else 
  {
    // Top level window
    h = new HWND__(NULL,0,&r,NULL,false,NULL,NULL);
    h->m_oswindow = event_box;
    h->m_owner = parent;

    // Create GTK window and add GtkEventBox as child
    GtkWidget *toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_add(GTK_CONTAINER(toplevel), h->m_oswindow);
    g_signal_connect(toplevel, "delete-event", G_CALLBACK(swell_gtkDeleteEvent), h);
    g_signal_connect(toplevel, "configure-event", G_CALLBACK(swell_gtkConfigureEvent), h);
    g_object_set_data(G_OBJECT(h->m_oswindow), "toplevel", toplevel);

    // Set position and size
    gtk_window_move(GTK_WINDOW(toplevel), r.left, r.top);
    gtk_widget_set_size_request(h->m_oswindow, r.right-r.left, r.bottom-r.top);

    // Set several options for window
    /*if (h->m_owner && h->m_owner->m_oswindow)
      gtk_window_set_transient_for(GTK_WINDOW(toplevel), GTK_WINDOW(gtk_widget_get_parent(h->m_owner->m_oswindow)));
    if (!(h->m_style & WS_CAPTION)) 
    {
      gtk_window_set_decorated(GTK_WINDOW(toplevel), false);
    }
    else if (!(h->m_style&WS_THICKFRAME))
    {
      gtk_window_set_type_hint(GTK_WINDOW(toplevel), GDK_WINDOW_TYPE_HINT_DIALOG);
      }*/
  }

  // Connect events
  //gtk_widget_add_events(h->m_oswindow, GDK_ALL_EVENTS_MASK);
  g_signal_connect(h->m_oswindow, "button-press-event", G_CALLBACK(swell_gdkEventHandler), h);
  g_signal_connect(h->m_oswindow, "button-release-event", G_CALLBACK(swell_gdkEventHandler), h);
  g_signal_connect(h->m_oswindow, "grab-broken-event", G_CALLBACK(swell_gdkEventHandler), h);
  g_signal_connect(h->m_oswindow, "key-press-event", G_CALLBACK(swell_gdkEventHandler), h);
  g_signal_connect(h->m_oswindow, "key-release-event", G_CALLBACK(swell_gdkEventHandler), h);
  g_signal_connect(h->m_oswindow, "motion-notify-event", G_CALLBACK(swell_gdkEventHandler), h);
  g_signal_connect(h->m_oswindow, "window-state-event", G_CALLBACK(swell_gdkEventHandler), h);
  g_signal_connect(h->m_oswindow, "draw", G_CALLBACK(swell_gtkDraw), h);

  /*if (forceNonChild || (p && !(p->windowTypeFlags&SWELL_DLG_WS_CHILD)))
  {
    if ((forceStyles&1) || (p && (p->windowTypeFlags&SWELL_DLG_WS_RESIZABLE)))
      h->m_style |= WS_THICKFRAME|WS_CAPTION;
    else h->m_style |= WS_CAPTION;
  }
  else if (!p && !parent) h->m_style |= WS_CAPTION;
  else if (parent && (!p || (p->windowTypeFlags&SWELL_DLG_WS_CHILD))) h->m_style |= WS_CHILD;*/

  if (p)
  {
    p->createFunc(h,p->windowTypeFlags);
    if (p->title) SetWindowText(h,p->title);

    h->m_dlgproc = dlgproc;
    h->m_wndproc = SwellDialogDefaultWindowProc;

    //HWND hFoc=m_children;
//    while (hFoc && !hFoc->m_wantfocus) hFoc=hFoc->m_next;
 //   if (!hFoc) hFoc=this;
  //  if (dlgproc(this,WM_INITDIALOG,(WPARAM)hFoc,0)&&hFoc) SetFocus(hFoc);

    h->m_dlgproc(h,WM_INITDIALOG,0,param);
  } 
  else
  {
    h->m_wndproc = (WNDPROC)dlgproc;
    h->m_wndproc(h,WM_CREATE,0,param);
  }
    
  return h;
}


HMENU SWELL_GetDefaultWindowMenu() { return g_swell_defaultmenu; }
void SWELL_SetDefaultWindowMenu(HMENU menu)
{
  g_swell_defaultmenu=menu;
}
HMENU SWELL_GetDefaultModalWindowMenu() 
{ 
  return g_swell_defaultmenumodal; 
}
void SWELL_SetDefaultModalWindowMenu(HMENU menu)
{
  g_swell_defaultmenumodal=menu;
}



SWELL_DialogResourceIndex *SWELL_curmodule_dialogresource_head; // this eventually will go into a per-module stub file


static char* s_dragdropsrcfn = 0;
static void (*s_dragdropsrccallback)(const char*) = 0;

void SWELL_InitiateDragDrop(HWND hwnd, RECT* srcrect, const char* srcfn, void (*callback)(const char* dropfn))
{
  SWELL_FinishDragDrop();

  if (1) return;

  s_dragdropsrcfn = strdup(srcfn);
  s_dragdropsrccallback = callback;
  
  char* p = s_dragdropsrcfn+strlen(s_dragdropsrcfn)-1;
  while (p >= s_dragdropsrcfn && *p != '.') --p;
  ++p;
  
} 

// owner owns srclist, make copies here etc
void SWELL_InitiateDragDropOfFileList(HWND hwnd, RECT *srcrect, const char **srclist, int srccount, HICON icon)
{
  SWELL_FinishDragDrop();

  if (1) return;
  
}

void SWELL_FinishDragDrop()
{
  free(s_dragdropsrcfn);
  s_dragdropsrcfn = 0;
  s_dragdropsrccallback = 0;  
}

#endif
