#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define VERSION_STRING "0.21"

#define BARHEIGHT 16

struct {
  int size;
  char rule[16];
  int color0,color1,color2,color3;
  int sleep;
  char fullscreen;
  char mirror;
} param={127,"12359",0,0xffffff,0xffff00,0xff0000,0,0,0};

struct {
  int size;
  char *S,*D;
  char *I; // source state,destination state,image 
  char rule[16];
  unsigned  step;
  double steps;
  int noblt;
  int sleep;
  char fullscreen;
  int fullpos,fullnum,fulltop,fullleft,fullrows,fullcols,fullsize;
  char busy;
  char rulebar;
  char mirror;
  char wback;
  char cperm;
} state;


void *StateAlloc(int size) {
  int size2=size/2+1;
  int msize=(size2*size2+9*size2+8)/2+10;
  void *m;

  m=malloc(msize);
  memset(m,0,msize);
  return m;
}

// copy image octets 
void State2Image(char *image,char *src,int size)  { 
  register int size2=size/2+1;
  //register int *ih,*ih2,*ih3,*ih4;
  register char *ih,*ih2,*ih3,*ih4;
  register unsigned char *sh;
  register int x,y;
  int bpl=(size+3)&~3;

  ih4=ih3=ih2=ih=image+(size2-1)*(bpl+1);
  sh=src+4;
  // bottom half
  for(y=1;y<=size2;y++) {
    for(x=0;x<y;x++) {
      *ih4=*ih3--=*ih2=*ih++=*sh++;
      ih4+=bpl,ih2+=bpl;
    }
    sh+=3;
    ih+=bpl-y;
    ih2+=1-y*bpl;
    ih3+=bpl+y;
    ih4+=-1-y*bpl;
  }

  ih2=ih=image+(size2-1)*bpl;
  for(y=1;y<size2;y++) 
    memcpy((ih2-=bpl),(ih+=bpl),bpl);
}

// Faster octet step
void State2State(char *dst,char *src,int size,char *rule) {
  register unsigned char *h,*g;
  register int s0,s1,s2;
  register int x,y,size2;
  
  size2=size/2+1;
  // fill boundary
  h=src;
  h[0]=h[2]=h[9];
  h[1]=h[3]=h[8];
  h+=4;
  for(y=1;y<=size2;y++) {
    h+=y;
    *(h++)=h[3];
    *(h++)=h[y+2];
    *(h++)=h[2];//h[2*y+6];
  }
  if(state.mirror) {
    char *g=h-2*size2-5;
    h[-3]=h[-5];
    h[-1]=g[-1]; 
    for(y=0;y<size2;y++)
      *(h++)=*g++;
    g[size2]=*h=h[-2];    
  }
  // perform step
  h=src+3,g=dst+4;
  for(y=1;y<=size2;y++) {
    s0=h[-y-2]+h[0]+h[y+3];h++;
    s1=h[-y-2]+h[0]+h[y+3];h++;
    s2=h[-y-2]+h[0]+h[y+3];h++;
    for(x=0;x<y;x++) {
      *g++=rule[s0+s1+s2];
      s0=s1,s1=s2;
      s2=h[-y-2]+h[0]+h[y+3];h++;
    }
    g+=3;
  }
}

void ClearMirror(char *dst,int size) {
  size=size/2+1;
  dst+=3+size*(size+1)/2+3*size;
  memset(dst-2,0,size+4);
}

void StateDuality(char *dst,char *src,int size) {
  register int i,j,sd;
  register char *s;
  
  size=size/2+1;
  dst+=4;
  src+=3+size*(size+1)/2+3*size-3;
  for(j=1;j<=size;j++) {
    s=src;
    sd=size+2;
    for(s=src,i=1;i<=j;i++) {
      *dst++=*s;
      s-=sd--;
    }
    dst+=3;
    src--;
  }
}


void StateRandomize(char *dst,int size) {
  register int i,j;
  static char init=1;
 
  if(init) {
    srand(time(NULL));
    init=0;
  }
  
  size=size/2+1;
  dst+=4;
  for(j=1;j<=size;j++) {
    for(i=1;i<=j;i++) 
      *dst++=rand()&1;
    dst+=3;
  }
}

void StateInverse(char *dst,int size) {
  register int i,j;
 
  size=size/2+1;
  dst+=4;
  for(j=1;j<=size;j++) {
    for(i=1;i<=j;i++) 
      *dst++^=1;
    dst+=3;
  }
}

void SetRule(const char *rule) {
  register const char *h;
  
  memset(state.rule,0,sizeof(state.rule));
  for(h=rule;*h;h++)
    if(*h>='0'&&*h<='9')
      state.rule[*h-'0']=1;
    else if(*h=='m'||*h=='M')
      state.mirror=1;
}

#include <windows.h>

int SwapRgb(int c) {
  //register char cr,*cp=(char*)&c;
  //cr=cp[0],cp[0]=cp[2],cp[2]=cr;
  return ((c>>16)&255)|(c&0xff00)|((c&255)<<16);
}

int color3(int c0,int c1,int c2) {
  return (c0&255)|((c1&255)<<8)|((c2&255)<<16);
}

int MixRgb(int a,int b,int t) {
  if(t<=0) return b;else if(t>=255) return a;
  int a0=a&255,a1=(a>>8)&255,a2=(a>>16)&255;
  int b0=b&255,b1=(b>>8)&255,b2=(b>>16)&255;
  int s=255-t;
  return color3((t*a0+s*b0)/255,(t*a1+s*b1)/255,(t*a2+s*b2)/255);
}


void PushCust(COLORREF *cust,COLORREF color) {
  int i;
  COLORREF buf,c2;
  c2=color&=0xffffff;
  for(i=0;i<16;i++) {
    buf=cust[i]&0xffffff;
    cust[i]=c2;
    if(buf==color) break;
    c2=buf;
  }
}

int ChangeColor(HWND hwnd,int *color) {
  CHOOSECOLOR chc;
  static COLORREF cust[16];//={0x80ff00,0x8000ff,0xff8000,0x0080ff,0x00ff80,0xff0080,0x80ffff,0xff80ff,0xffff80,0x800000,0x008000,0x000080};
 
  memset(&chc,0,sizeof(chc));
  chc.lStructSize=sizeof(chc);
  chc.hwndOwner=hwnd;
  chc.rgbResult=SwapRgb(*color);
  chc.lpCustColors=cust;
  chc.Flags=CC_FULLOPEN|CC_RGBINIT;

  if(ChooseColor(&chc)) {
    *color=SwapRgb(chc.rgbResult)&0xffffff;  
    PushCust(cust,chc.rgbResult);
    return 1;
  }
  return 0;
}

HDC dc;
HWND hWnd;
HINSTANCE hInstance=NULL;
BITMAPINFO *bi;

int CreateBIH8(BITMAPINFO **bi,int width,int height
    ,COLORREF color0,COLORREF color1,char **bits) {
  BITMAPINFOHEADER *bih;
  COLORREF *c;
  int bpl=(width+3)&~3;

  if(!*bi) *bi=malloc(sizeof(BITMAPINFOHEADER)+2*sizeof(RGBQUAD));
  bih=&(*bi)->bmiHeader;
  bih->biSize=sizeof(*bih);
  bih->biWidth=width;
  bih->biHeight=height;
  bih->biPlanes=1;
  bih->biBitCount=8;
  bih->biCompression=BI_RGB;
  bih->biSizeImage=width*height;
  bih->biXPelsPerMeter=0;
  bih->biYPelsPerMeter=0;
  bih->biClrUsed=2;
  bih->biClrImportant=0;
  if(*bits) free(*bits);
  *bits=malloc(bpl*height);
  c=(COLORREF*)(*bi)->bmiColors;
  c[0]=color0;
  c[1]=color1;
  return 0;
}

int RuleIndex(int mx) {
  int i=mx*11/param.size;
  return i<0?0:i>10?10:i;
}

void HideRules() {
  RECT rt;
  int top=state.fullscreen?0:param.size;
  rt.left=0;rt.top=top;rt.bottom=BARHEIGHT;rt.right=param.size;
  FillRect(dc,&rt,GetStockObject(state.wback?WHITE_BRUSH:BLACK_BRUSH));
}

void DrawRules() {
  int i,size=param.size;
  int x0,x1,x2,y0,y1,y2;
  int top=size;
  RECT r;
  if(!state.rulebar) return;
  if(state.fullscreen) {
    top=0;
  }
  r.left=0;r.top=top;
  r.right=size;r.bottom=r.top+BARHEIGHT;
  FillRect(dc,&r,GetStockObject(GRAY_BRUSH));
  r.top+=1;r.bottom-=1;
  y0=r.top+1;y2=r.bottom-2;y1=(y0+y2)/2;
  for(i=0;i<11;i++) {
   char on=i>9?state.mirror:state.rule[i];
   POINT pt;
   r.left=size*i/11+1;
   r.right=size*(i+1)/11-1;
   FillRect(dc,&r,GetStockObject(on?BLACK_BRUSH:WHITE_BRUSH));
   SelectObject(dc,GetStockObject(on?WHITE_PEN:BLACK_PEN));
   x0=r.left+1;x2=r.right-2;x1=(x0+x2)/2;
   switch(i) {
    case 1:MoveToEx(dc,x1,y0,&pt);LineTo(dc,x1,y2+1);break;
    case 2:MoveToEx(dc,x0,y0,&pt);LineTo(dc,x2,y0);LineTo(dc,x2,y1);LineTo(dc,x0,y1);LineTo(dc,x0,y2);LineTo(dc,x2+1,y2);break;
    case 3:MoveToEx(dc,x0,y1,&pt);LineTo(dc,x2,y1);MoveToEx(dc,x0,y0,&pt);LineTo(dc,x2,y0);LineTo(dc,x2,y2);LineTo(dc,x0-1,y2);break;
    case 4:MoveToEx(dc,x0,y0,&pt);LineTo(dc,x0,y1);LineTo(dc,x2,y1);LineTo(dc,x2,y2+1);break;
    case 5:MoveToEx(dc,x2,y0,&pt);LineTo(dc,x0,y0);LineTo(dc,x0,y1);LineTo(dc,x2,y1);LineTo(dc,x2,y2);LineTo(dc,x0-1,y2);break;
    case 6:MoveToEx(dc,x2,y0,&pt);LineTo(dc,x0,y0);LineTo(dc,x0,y2);LineTo(dc,x2,y2);LineTo(dc,x2,y1);LineTo(dc,x0,y1);break;
    case 7:MoveToEx(dc,x0,y0,&pt);LineTo(dc,x2,y0);LineTo(dc,x2,y2+1);break;
    case 8:MoveToEx(dc,x0,y1,&pt);LineTo(dc,x2,y1);MoveToEx(dc,x0,y0,&pt);LineTo(dc,x2,y0);LineTo(dc,x2,y2);LineTo(dc,x0,y2);LineTo(dc,x0,y0);break;
    case 9:MoveToEx(dc,x0,y2,&pt);LineTo(dc,x2,y2);LineTo(dc,x2,y0);LineTo(dc,x0,y0);LineTo(dc,x0,y1);LineTo(dc,x2,y1);break;
    case 10:MoveToEx(dc,x0,y2,&pt);LineTo(dc,x0,y0);LineTo(dc,x1,y1);LineTo(dc,x2,y0);LineTo(dc,x2,y2+1);break;
   }
  }
}

void Redraw(HWND hwnd,char full) {
  char rr=0;
  if(state.fullscreen) {
    RECT r;
    int ht,w,h;
    char mosaic=state.fullscreen!=2;
    GetWindowRect(hwnd,&r);
    if(full) { FillRect(dc,&r,GetStockObject(state.wback?WHITE_BRUSH:BLACK_BRUSH)); }
    w=r.right/state.size,h=r.bottom/state.size;
    if(mosaic&&(w>0&&h>0)&&(w>1||h>1)) {
      int size=state.size;
      state.fullnum=w*h;
      state.fullcols=w;state.fullrows=h;
      if(state.fullpos>=w*h) state.fullpos=0;
      r.left=state.fullleft=r.right%size/2;
      r.top=state.fulltop=r.bottom%size/2;
      r.left+=state.fullpos%w*size;
      r.top+=state.fullpos/w*size;
      SetDIBitsToDevice(dc,r.left,r.top,state.size,state.size
        ,0,0,0,state.size,state.I,bi,DIB_RGB_COLORS);
      rr=state.rulebar&&state.fullpos==0;
      state.fullsize=size;
    } else {
      if(r.right>r.bottom) {r.left=(r.right-r.bottom)/2;r.right=r.bottom;}
      else {r.top=(r.bottom-r.right)/2;r.bottom=r.right;}
      ht=SetStretchBltMode(dc,HALFTONE);
      StretchDIBits(dc,r.left,r.top,r.right,r.bottom
        ,0,0,state.size,state.size,state.I,bi,DIB_RGB_COLORS,SRCCOPY);
      SetStretchBltMode(dc,ht);
      state.fullleft=r.left,state.fulltop=r.top;
      state.fullrows=state.fullcols=1;
      state.fullsize=r.bottom-r.top;
    }
  } else {
    SetDIBitsToDevice(dc,0,0,state.size,state.size
      ,0,0,0,state.size,state.I,bi,DIB_RGB_COLORS
    );
  }
  if(full||rr) DrawRules();
}

char *fn_file(char *fullname) { // vrati ukazatel na jmeno souboru (odrizne disk a adresar)
  char *p,*q;
  for(q=p=fullname;*p;p++)
    if(*p=='\\'||*p==':')
      q=p+1;
  return q;
}

int AskSaveFilename(char *file,char *title,char *filter) {
  char *fs,filename[256],dir[256];
  OPENFILENAME of;
  int r;

  memset(&of,0,sizeof(of));
  of.lStructSize=sizeof(of);
  of.hwndOwner=hWnd;
  of.Flags=OFN_LONGNAMES|OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;

  strcpy(filename,file);
  of.lpstrTitle=title;
  of.lpstrFilter=filter;
  fs=fn_file(filename);
  if(fs>filename) {
    fs[-1]=0;
    strcpy(dir,filename);
    strcpy(filename,fs);
    of.lpstrInitialDir=dir;
  } else {
    GetCurrentDirectory(sizeof(dir),dir);
    of.lpstrInitialDir=dir;
  }
  of.lpstrFile=filename;
  of.nMaxFile=sizeof(filename);

  if((r=GetSaveFileName(&of))) strcpy(file,filename);
  return r;
}

int writebmp() {
  BITMAPFILEHEADER bfh;
  int s1,s2,s3,bpl;
  static char filename[256]="out.bmp";
  FILE *f;

  if(!AskSaveFilename(filename,"Export BMP","Bitmaps (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0"))
    return -1;

  if(!(f=fopen(filename,"wb")))
    return -2;
  s1=sizeof(bfh);
  s2=sizeof(bi->bmiHeader)+2*sizeof(RGBQUAD);
  bpl=(bi->bmiHeader.biWidth+3)&~3;
  s3=bpl*(bi->bmiHeader.biHeight);
  bfh.bfType='B'|('M'<<8);
  bfh.bfSize=s1+s2+s3;
  bfh.bfReserved1=0;
  bfh.bfReserved2=0;
  bfh.bfOffBits=s1+s2;
  fwrite(&bfh,s1,1,f);
  fwrite(bi,s2,1,f);
  fwrite(state.I,s3,1,f);
  fclose(f);
  return 0;
}


void writeclip() {
  HANDLE sh;
  int s,s2,bpl;
  char *h;

  if(!OpenClipboard(NULL))
    return;
  EmptyClipboard();
  
  s2=sizeof(bi->bmiHeader)+2*sizeof(RGBQUAD);
  bpl=(bi->bmiHeader.biWidth+3)&~3;
  s=bpl*bi->bmiHeader.biHeight;
  //printf("allocating %d\n",s);
  sh=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,s2+s);

  h=GlobalLock(sh);
  memcpy(h,bi,s2);
  h+=s2;
  memcpy(h,state.I,s);

  GlobalUnlock(sh);
  SetClipboardData(CF_DIB,sh);
  CloseClipboard();

}

void do_command(int cmd) {
}

void printhelp();

char Ctrl() { return 0!=(GetKeyState(VK_CONTROL)&0x8000);}
char Shift() { return 0!=(GetKeyState(VK_SHIFT)&0x8000);}
char Alt() { return 0!=(GetKeyState(VK_MENU)&0x8000);}
char CapsLock() { return 0!=(GetKeyState(VK_CAPITAL)&0x1);}
//char AShift() { return 0!=(GetAsyncKeyState(VK_SHIFT)&0x8000);}


void lbutton() {
  if(state.step) state.noblt=!state.noblt;
  state.step=~1;
  SetWindowText(hWnd,"Stepping");
}

void rbutton() {
  state.step=state.fullscreen&&state.fullscreen!=2&&state.fullnum?state.fullnum:1;
  state.noblt=0;
}

void fullscreen(HWND wnd,char on) {
  LONG st;
  st=GetWindowLong(wnd,GWL_STYLE);
  st=!on?st|WS_CAPTION:st&~WS_CAPTION;
  //ShowWindow(wnd,SW_HIDE);
  SetWindowLong(wnd,GWL_STYLE,st);
  ShowWindow(wnd,on?SW_MAXIMIZE:SW_RESTORE);
  if(!on) {
    RECT r;
    r.left=r.top=0;
    r.right=param.size;r.bottom=r.right+(state.rulebar?BARHEIGHT:0);
    AdjustWindowRect(&r,st,0);
    SetWindowPos(hWnd,0,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
  }
}

void switchfullscreen() {
  state.fullscreen=!state.fullscreen;
  if(state.fullscreen) {
    state.fullpos=state.fullnum=0;
    if(Shift()) state.fullscreen=2;
  } else if(state.step<state.fullnum) 
    state.step=0;
  fullscreen(hWnd,state.fullscreen);
}

void CheckWinSize(HWND hwnd) {
  RECT r;
  GetClientRect(hWnd,&r);
  if((r.right-r.left<r.bottom-r.top)!=!!state.rulebar) {
    int height;
    GetWindowRect(hWnd,&r);
    height=r.bottom-r.top+(state.rulebar?BARHEIGHT:-BARHEIGHT);
    SetWindowPos(hWnd,0,0,0,r.right-r.left,height,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    if(state.rulebar) DrawRules();
  }
}

void SwitchColors(HWND hwnd) {
  int y;
  RGBQUAD x=bi->bmiColors[0];
  bi->bmiColors[0]=bi->bmiColors[1];
  bi->bmiColors[1]=x;
  y=param.color0;param.color0=param.color1;param.color1=y;
  Redraw(hwnd,0);
}

void UpdateMirror() {
  if(!state.mirror) {
    ClearMirror(state.S,state.size);
    ClearMirror(state.D,state.size);
  }
  DrawRules();
}

int *GetBits(HDC dc,HDC *dc2p,HBITMAP *hbmp,int w,int h,char border) {
  BITMAPINFOHEADER bi;
  int *bits; //,*h2;
  HBITMAP hbmp2;
  HDC dc2;

  memset(&bi,0,sizeof(bi));
  bi.biSize=sizeof(bi);
  bi.biWidth=(border?2:0)+w;
  bi.biHeight=(border?2:0)+h;
  bi.biPlanes=1;
  bi.biBitCount=32;
  bi.biCompression=BI_RGB;
  *dc2p=dc2=CreateCompatibleDC(dc);
  *hbmp=CreateDIBSection(dc2,(BITMAPINFO*)&bi,DIB_RGB_COLORS,(void**)(void*)&bits,NULL,0);
  hbmp2=SelectObject(dc2,*hbmp);
  DeleteObject(hbmp2);
  BitBlt(dc2,border?1:0,border?1:0,bi.biWidth,bi.biHeight,dc,0,0,SRCCOPY);
  return bits;
}


void Line(int c,int *start,int add,int len) {
  while(len--) { *start=c; start+=add; }
}
int Fill2Sub(int *bits,int w,int h,int nxy,int *xy,char diag,char mode,int color2,int color3) {
  int w2=w+2,r,r2,c=0,*p6=bits,*p6e=bits+(w+2)*(h+1);
  int m=0,n=0,size=w*h,*fifo,d=-1;
  
  if(nxy<1) return 0;

  fifo=malloc(4*size);
  while(nxy-->0) {
    int x,y=*xy++;
    x=LOWORD(y),y=HIWORD(y);
    if(x<0||y<0||x>=w||y>=h) continue;
    y=h-1-y;
    r=(y+1)*w2+x+1;
    if(!n) {
      c=bits[r];
      if(c<0) continue;
      if(mode==6) {
        int y2,c6=0x7fffffff;
        for(y2=0;y2<h;y2++) {
          int *g0=bits+w2+y2*w2+1,*g=g0,*ge=g+w;
          while(g<ge) {
            if(*g==c) *g=c6;
            g++;
          }
        }
        c=c6;
        p6=bits+w2+1;
        bits[r]=d;
        fifo[n++]=r;
        break;
      }
    }
    bits[r]=d;fifo[n++]=r;
  }
 n6:
  while(m<n) {
    r=fifo[m++];
    d=bits[r]-1;
    if(bits[(r2=r+1)]==c) {bits[r2]=d;fifo[n++]=r2;}
    if(bits[(r2=r-1)]==c) {bits[r2]=d;fifo[n++]=r2;}
    if(bits[(r2=r+w2)]==c) {bits[r2]=d;fifo[n++]=r2;}
    if(bits[(r2=r-w2)]==c) {bits[r2]=d;fifo[n++]=r2;}
    if(diag) {
      if(bits[(r2=r+w2+1)]==c) {bits[r2]=d;fifo[n++]=r2;}
      if(bits[(r2=r+w2-1)]==c) {bits[r2]=d;fifo[n++]=r2;}
      if(bits[(r2=r-w2+1)]==c) {bits[r2]=d;fifo[n++]=r2;}
      if(bits[(r2=r-w2-1)]==c) {bits[r2]=d;fifo[n++]=r2;}
    }
    /*if(n>=size-7) {
      if(m<7) break;
      memmove(fifo,fifo+m,4*(n-m));
      n-=m,m=0;
    }*/
  }
  if(mode==5||mode==6) {
    int c3=1<<31,n2;
    d=-1;c=0x80000001;
    m=n2=0;
    while(m<n) {
      int *g=bits+fifo[m++];
      char border=g[-1]>=0||g[-1]==c3||g[+1]>=0||g[+1]==c3||g[-w2]>=0||g[-w2]==c3||g[w2]>=0||g[w2]==c3;
      if(diag&&!border) if(g[-1-w2]>=0||g[-1-w2]==c3||g[-1+w2]>=0||g[-1+w2]==c3||g[1-w2]>=0||g[1-w2]==c3||g[1+w2]>=0||g[1+w2]==c3) border=1;
      if(border) {
        fifo[n2++]=g-bits;
        *g=d;
      } else
        *g=c;
    }
    /*
    n=m=0;
    for(y2=0;y2<h;y2++) {
      int *g0=bits+w2+y2*w2+1,*g=g0,*ge=g+w;
      while(g<ge) {
        if(*g<0&&*g!=c3) {
          char border=g[-1]>=0||g[-1]==c3||g[+1]>=0||g[+1]==c3||g[-w2]>=0||g[-w2]==c3||g[w2]>=0||g[w2]==c3;
          if(diag&&!border) if(g[-1-w2]>=0||g[-1-w2]==c3||g[-1+w2]>=0||g[-1+w2]==c3||g[1-w2]>=0||g[1-w2]==c3||g[1+w2]>=0||g[1+w2]==c3) border=1;
          if(border) {
            fifo[n++]=g-bits;
            *g=d;
          } else
            *g=c;
        }
        g++;
      }
    }*/
    n=n2;m=0;
    while(m<n) {
      r=fifo[m++];
      d=bits[r]-1;
      if(bits[(r2=r+1)]==c) {bits[r2]=d;fifo[n++]=r2;}
      if(bits[(r2=r-1)]==c) {bits[r2]=d;fifo[n++]=r2;}
      if(bits[(r2=r+w2)]==c) {bits[r2]=d;fifo[n++]=r2;}
      if(bits[(r2=r-w2)]==c) {bits[r2]=d;fifo[n++]=r2;}
      if(diag) {
        if(bits[(r2=r+w2+1)]==c) {bits[r2]=d;fifo[n++]=r2;}
        if(bits[(r2=r+w2-1)]==c) {bits[r2]=d;fifo[n++]=r2;}
        if(bits[(r2=r-w2+1)]==c) {bits[r2]=d;fifo[n++]=r2;}
        if(bits[(r2=r-w2-1)]==c) {bits[r2]=d;fifo[n++]=r2;}
      }
      /*if(n>=size-7) {
        if(m<7) break;
        memmove(fifo,fifo+m,4*(n-m));
        n-=m,m=0;
      }*/
    }
    if(mode==6) {
      d=-d-1;
      for(m=0;m<n;m++) {
        int d2,*g=bits+fifo[m];
        d2=*g;
        *g=d<2?MixRgb(color3,color2,128):MixRgb(color3,color2,-d2*255/d);
      }
      c=0x7fffffff;
      while(p6<p6e) {
       if(*p6++==c) {
         n=m=0;
         *p6=d=-1;
         fifo[n++]=p6-bits;
         goto n6;
       }
      }
      d=-1;
    }
  }
  free(fifo);
  return -d-1;
}

void BitBltResult(HDC dc2,int w,int fill) {
  int h;
  fill=fill?1:0;
  if(state.fullscreen) {
    if(state.fullscreen!=2) {
      w=state.fullcols*param.size;
      h=state.fullrows*param.size;
    } else 
      w=h=state.fullsize;
    BitBlt(dc,state.fullleft,state.fulltop,w,h,dc2,fill+state.fullleft,fill+state.fulltop,SRCCOPY);
  } else
    BitBlt(dc,0,0,w,w,dc2,fill,fill,SRCCOPY);
}

void Fill2(HWND hwnd,int x,int y,char f8,char diag,int color2,int color3,char mode) {
  int *bits;
  HDC dc2;
  HBITMAP hbmp;
  int w,w2,h,*g,*ge,c3,d;
  RECT cr;

  GetClientRect(hwnd,&cr);
  w=cr.right;
  h=cr.bottom;
  
  bits=GetBits(dc,&dc2,&hbmp,w,h,1);

  w2=w+2;
  for(g=bits,ge=g+w2*(h+2);g<ge;(*g++)&=0xffffff);
  c3=1<<31;
  Line(c3,bits,1,w2);Line(c3,bits+(h+1)*w2,1,w2);
  Line(c3,bits+w2,w2,h);Line(c3,bits+2*w2-1,w2,h);
  if((mode==6||mode==5)&&state.fullscreen) {
    int i,fr=state.fullleft+state.fullcols*state.fullsize,fb=state.fulltop+state.fullrows*state.fullsize;
    if(x<state.fullleft-1) x=state.fullleft-1;
    if(x>fr) x=fr;
    if(y<state.fulltop-1) y=state.fulltop-1;
    if(y>fb) y=fb;
    for(i=0;i<state.fullleft-1;i++) Line(c3,bits+w2+1+i,w2,h);
    for(i=fr+1;i<w;i++) Line(c3,bits+w2+1+i,w2,h);
    for(i=0;i<state.fulltop-1;i++) Line(c3,bits+w2+w2*(h-1-i),1,w2);
    for(i=fb+1;i<h;i++) Line(c3,bits+w2+w2*(h-1-i),1,w2);
  }

  if(f8) {
    int sx,sy,dx,dy,i,size=param.size,xy[9];
    if(state.fullscreen) {
      char mosaic=state.fullscreen!=2;
      if(mosaic) {
        sx=state.fullleft+(x-state.fullleft)/size*size;
        sy=state.fulltop+(y-state.fulltop)/size*size;
      } else {
        sx=state.fullleft+state.fullsize/2-size/2;
        sy=state.fulltop+state.fullsize/2-size/2;
      }
    } else sx=sy=0; 
    sx+=size/2,sy+=size/2;
    dx=x-sx,dy=y-sy;
    if(dx<0) dx=-dx;if(dy<0) dy=-dy;
    xy[0]=(x&0xffff)|((y&0xffff)<<16);
    for(i=0;i<8;i++) {
      int fx=sx+(i&1?-1:+1)*(i&4?dx:dy),fy=sy+(i&2?-1:+1)*(i&4?dy:dx);
      xy[1+i]=(fx&0xffff)|((fy&0xffff)<<16);
    }
    d=Fill2Sub(bits,w,h,9,xy,diag,mode,color2,color3);
  } else {
    int xy=(x&0xffff)|((y&0xffff)<<16);
    d=Fill2Sub(bits,w,h,1,&xy,diag,mode,color2,color3);
  }

  if(mode!=6) for(y=0;y<h;y++) {
    int *g=bits+w2+y*w2+1,*ge=g+w;
    while(g<ge) {
      int d2=*g;
      if(d2<0) {
        *g=d<2?MixRgb(color3,color2,128):MixRgb(color3,color2,-d2*255/d);
      }
      g++;
    }
  }
     
  BitBltResult(dc2,w,1);
  /*
  if(state.fullscreen) {
    if(state.fullscreen!=2) {
      w=state.fullcols*param.size;
      h=state.fullrows*param.size;
    } else 
      w=h=state.fullsize;
    BitBlt(dc,state.fullleft,state.fulltop,w,h,dc2,1+state.fullleft,1+state.fulltop,SRCCOPY);
  } else
    BitBlt(dc,0,0,w,h,dc2,1,1,SRCCOPY);*/

  DeleteDC(dc2);
  DeleteObject(hbmp);
}

void Boundary(HWND hwnd,char expand) {
  int *bits;
  char *buf,*b;
  HDC dc2;
  HBITMAP hbmp;
  int w,h,*g,*ge;
  int x,y;
  RECT cr;

  GetClientRect(hwnd,&cr);
  w=cr.right;
  h=cr.bottom;
  
  bits=GetBits(dc,&dc2,&hbmp,w,h,0);
  buf=malloc(w*h);
 
  for(y=0;y<h;y++) {
    g=bits+w*y;
    b=buf+w*y;
    for(x=0;x<w;x++,g++,b++) {
      char on=0;
      if(expand) {
        if(0!=((*g)&0xffffff)) {
          if(x>0&&!(g[-1]&0xffffff)) on=1;
          else if(x<w-1&&!(g[1]&0xffffff)) on=1;
          else if(y>0&&!(g[-w]&0xffffff)) on=1;
          else if(y<h-1&&!(g[+w]&0xffffff)) on=1;
          if(expand>1&&!on) {
            if(x>0&&y>0&&!(g[-w-1]&0xffffff)) on=1;
            else if(x<w-1&&y>0&&!(g[-w+1]&0xffffff)) on=1;
            else if(x>0&&y<h-1&&!(g[+w-1]&0xffffff)) on=1;
            else if(x<w-1&&y<h-1&&!(g[+w+1]&0xffffff)) on=1;
          }
        }
      } else {
        if(0==((*g)&0xffffff)) {
          if(x>0&&(g[-1]&0xffffff)) on=1;
          else if(x<w-1&&(g[1]&0xffffff)) on=1;
          else if(y>0&&(g[-w]&0xffffff)) on=1;
          else if(y<h-1&&(g[+w]&0xffffff)) on=1;
        }
      }
      *b=on;
    }
  }
  for(g=bits,ge=g+w*h,b=buf;g<ge;g++,b++) {
    char on=0==(*g&0xffffff);
    if(expand) {
      if(!on&&*b) *g=0x0;
    } else if(on&&!*b) *g=0xffffff;
  }

  free(buf);

  BitBltResult(dc2,w,0);

  DeleteDC(dc2);
  DeleteObject(hbmp);
}

/*
void Fill(HWND hwnd,int x,int y) {
  COLORREF c;
  HBRUSH hbr;
  HDC dc2;
  HBITMAP bmp=0;
  RECT cr;
  int c2;
  char offscreen=state.fullscreen;
  if(offscreen) {
    dc2=CreateCompatibleDC(dc);
    GetClientRect(hWnd,&cr);
    bmp=CreateCompatibleBitmap(dc,cr.right,cr.bottom);
    SelectObject(dc2,bmp);
    BitBlt(dc2,0,0,cr.right,cr.bottom,dc,0,0,SRCCOPY);
  } else dc2=dc;
  c2=SwapRgb(param.color2);
  hbr=SelectObject(dc2,CreateSolidBrush(c2));
  c=GetPixel(dc2,x,y);
  if(Shift()|Ctrl()) {
    int sx,sy,dx,dy,i,size=param.size;
    if(state.fullscreen) {
      char mosaic=state.fullscreen!=2;
      if(mosaic) {
        sx=state.fullleft+(x-state.fullleft)/size*size;
        sy=state.fulltop+(y-state.fulltop)/size*size;
      } else {
        sx=state.fullleft+state.fullsize/2-size/2;
        sy=state.fulltop+state.fullsize/2-size/2;
      }
    } else sx=sy=0; 
    sx+=size/2,sy+=size/2;
    dx=x-sx,dy=y-sy;
    if(dx<0) dx=-dx;if(dy<0) dy=-dy;
    for(i=0;i<8;i++) {
      int fx=sx+(i&1?-1:+1)*(i&4?dx:dy),fy=sy+(i&2?-1:+1)*(i&4?dy:dx);
      c=GetPixel(dc2,fx,fy);
      if(c!=c2) ExtFloodFill(dc2,fx,fy,c,FLOODFILLSURFACE);
    }
  } else 
    ExtFloodFill(dc2,x,y,c,FLOODFILLSURFACE);
  hbr=SelectObject(dc2,hbr);
  DeleteObject(hbr);
  if(offscreen) {
    BitBlt(dc,0,0,cr.right,cr.bottom,dc2,0,0,SRCCOPY);
    DeleteObject(bmp);
    DeleteDC(dc2);
  }
}
*/

void Invert(char border) {
  int w,h;
  w=GetDeviceCaps(dc,HORZRES);
  h=GetDeviceCaps(dc,VERTRES);
  if(border) {
    int iw=state.fullcols*state.fullsize,ih=state.fullrows*state.fullsize;
    int fr=state.fullleft+iw,fb=state.fulltop+ih;
    BitBlt(dc,0,0,w,state.fulltop,dc,0,0,DSTINVERT);
    BitBlt(dc,0,fb,w,h-fb,dc,0,0,DSTINVERT);
    BitBlt(dc,0,state.fulltop,state.fullleft,ih,dc,0,0,DSTINVERT);
    BitBlt(dc,fr,state.fulltop,w-fr,ih,dc,0,0,DSTINVERT);
  } else if(state.fullscreen)
    BitBlt(dc,state.fullleft,state.fulltop,state.fullcols*state.fullsize,state.fullrows*state.fullsize,dc,0,0,DSTINVERT);
  else
    BitBlt(dc,0,0,w,h,dc,0,0,DSTINVERT);
  /*
  RECT r;
  int rop2;
  r.right=GetDeviceCaps(dc,HORZRES);
  r.bottom=GetDeviceCaps(dc,VERTRES);
  rop2=SetROP2(dc,R2_NOT);
  FillRect(dc,&r,GetStockObject(BLACK_BRUSH));
  SetROP2(dc,rop2);
  */
}

int rgb2cmy(int color) {
  unsigned char p0=color&0xff,p1=(color>>8)&0xff,p2=(color>>16)&0xff,r,g,b;
  if(p0>p1&&p0>p2) {
    if(p1>p2) r=p2,g=p0-p1+p2,b=p0;
    else r=p1,g=p0,b=p0-p2+p1;
  } else if(p1>p0&&p1>p2) {
    if(p0>p2) r=p1-p0+p2,g=p2,b=p1;
    else r=p1,g=p0,b=p1-p2+p0;
  } else {
    if(p0>p1) r=p2-p0+p1,g=p2,b=p1;
    else r=p2,g=p2-p1+p0,b=p0;
  }
  return (color&0xff000000)|(b<<16)|(g<<8)|r;
}

void ColorPerm(HWND hwnd,char back,char cmy) {
  int *bits;
  HDC dc2;
  HBITMAP hbmp;
  int w,h,*g,*ge;
  RECT cr;
  char mode=state.cperm;

  if(!cmy) state.cperm=!mode;
  if(back) mode^=1;
  GetClientRect(hwnd,&cr);
  w=cr.right;
  h=cr.bottom;
  
  bits=GetBits(dc,&dc2,&hbmp,w,h,0);

  for(g=bits,ge=bits+w*h;g<ge;g++) {
    if(cmy) *g=rgb2cmy(*g);
    else if(mode) *g=(*g&0xffff0000)|((*g&0xff)<<8)|((*g>>8)&0xff);
    else *g=(*g&0xff0000ff)|((*g&0xff00)<<8)|((*g>>8)&0xff00);
  }

  BitBlt(dc,0,0,w,h,dc2,0,0,SRCCOPY);

  DeleteDC(dc2);
  DeleteObject(hbmp);
}

int bitscan(int x) {
  int r=0;
  unsigned short u;
  if(!x) return -1;
  if(x&0xffff0000) r=16,u=(x>>16);else u=x;
  if(u&0xff00) r+=8,u>>=8;
  if(u&0xf0) r+=4,u>>=4;
  if(u&0xc) r+=2,u>>=2;
  return r+(u==3?1:u-1);
}

int isqrt(int x) {
  register int a,a2;
  if(x<4) return x>0;
  a=x>>(bitscan(x)>>1);
  do {
    a2=a;
    a=(unsigned)(a2+x/a2)>>1;
  } while(a<a2);
  return a2;
}

void Gradient(HWND hwnd,int x,int y,int color,int c2,int c3) {
  int *bits;
  HDC dc2;
  HBITMAP hbmp;
  int w,h,*g,*ge,md,dy,dx,iy;
  RECT cr;

  GetClientRect(hwnd,&cr);
  w=cr.right;
  h=cr.bottom;
  if(x<0||y<0||x>=w||y>=h) return;
  y=h-y-1;
  
  bits=GetBits(dc,&dc2,&hbmp,w,h,0);
  if(color==-1) color=bits[w*y+x];
  color&=0xffffff;
  if(c2==c3) {
    for(g=bits,ge=g+w*h;g<ge;g++) {
      int cg=(*g)&0xffffff;
      if(cg==color)
        *g=c2;
    }
  } else {
    md=0;
    for(g=bits,iy=0;iy<h;iy++) {
      dy=iy-y;
      for(ge=g+w,dx=-x;g<ge;g++,dx++) {
        int cg=(*g)&0xffffff;
        if(cg==color) {
          int d=dy*dy+dx*dx;
          if(md<d) md=d;
        }
      }
    }
    if(md==0) md=1;else md=isqrt(md);
    for(g=bits,iy=0;iy<h;iy++) {
      dy=iy-y;
      for(ge=g+w,dx=-x;g<ge;g++,dx++) {
        int cg=(*g)&0xffffff;
        if(cg==color) {
          int d=dy*dy+dx*dx;
          d=isqrt(d);
          *g=MixRgb(c3,c2,d*255/md);
        }
      }
    }
  }

  if(state.fullscreen) {
    if(state.fullscreen!=2) {
      w=state.fullcols*param.size;
      h=state.fullrows*param.size;
    } else 
      w=h=state.fullsize;
    BitBlt(dc,state.fullleft,state.fulltop,w,h,dc2,state.fullleft,state.fulltop,SRCCOPY);
  } else
    BitBlt(dc,0,0,w,h,dc2,0,0,SRCCOPY);

  DeleteDC(dc2);
  DeleteObject(hbmp);
}

void MoveCursor(int dx,int dy,int mult,char white,char black) {
  POINT pt,wpt;
  RECT r;
  int n,c0;
  char first;
  if(!GetCursorPos(&pt)) return;
  if(!white&&!black) {
    SetCursorPos(pt.x+mult*dx,pt.y+mult*dy);
    return;
  }
  GetWindowRect(hWnd,&r);
  wpt=pt;
  ScreenToClient(hWnd,&wpt);
  n=0;
  c0=0xffffff&GetPixel(dc,wpt.x,wpt.y);
  first=(white&&c0==0xffffff)||(black&&c0==0x0);
  while(1) {
    COLORREF c;
    n++;wpt.x+=dx;wpt.y+=dy;
    if(wpt.x<0||wpt.x<0||wpt.x>=r.right||wpt.y>=r.bottom) return;
    c=GetPixel(dc,wpt.x,wpt.y);
    if(first&&c!=c0) first=0;
    if(c==CLR_INVALID) break;
    if(!first&&((white&&c==0xffffff)||(black&&c==0x0))) break;
  }
  SetCursorPos(pt.x+n*dx,pt.y+n*dy);
}

char KeyDown(WPARAM wParam) {
  switch(wParam) {
   case VK_LEFT:MoveCursor(-1,0,Shift()?10:1,Ctrl(),Alt());break;
   case VK_RIGHT:MoveCursor(+1,0,Shift()?10:1,Ctrl(),Alt());break;
   case VK_UP:MoveCursor(0,-1,Shift()?10:1,Ctrl(),Alt());break;
   case VK_DOWN:MoveCursor(0,+1,Shift()?10:1,Ctrl(),Alt());break;
   default:return 0;
  }
  return 1;
}

long FAR PASCAL WndProc (HWND hWnd, UINT iMessage,
    WPARAM wParam, LPARAM lParam) {
    int i;
  switch(iMessage)  {
        case WM_PAINT: {
         PAINTSTRUCT ps;
         BeginPaint(hWnd,&ps);
         Redraw(hWnd,1);
         // repaint(ps.hdc);
         EndPaint(hWnd,&ps);
        } break;
        case WM_COMMAND:
         do_command(LOWORD(wParam));
         break;
	case WM_SIZE:{
         switch(wParam) {
          case SIZE_MAXIMIZED:
            if(!state.fullscreen) {
              ShowWindow(hWnd,SW_RESTORE);
              switchfullscreen();
            }
            break;
          case SIZE_MINIMIZED:
            state.step=0;
            state.noblt=0;
            SetWindowText(hWnd,"Sleeping");
            break;
          case SIZE_RESTORED:
            CheckWinSize(hWnd);
            break;
         }
          //printf("WM_SIZE %d\n",wParam);
          Redraw(hWnd,1);
	} break;
	case WM_DESTROY:
        quit:
	 PostQuitMessage(0);
	 return 0;
        case WM_SYSKEYDOWN:
         if(wParam==VK_RETURN) switchfullscreen();
         else  if(!KeyDown(wParam))
	   return(DefWindowProc(hWnd, iMessage, wParam, lParam));
         break;
        case WM_KEYDOWN:
         //printf("%d\n",wParam);
         switch(wParam) {
          case '9':case VK_NUMPAD9:i=9;goto digit;
          case '8':case VK_NUMPAD8:i=8;goto digit;
          case '7':case VK_NUMPAD7:i=7;goto digit;
          case '6':case VK_NUMPAD6:i=6;goto digit;
          case '5':case VK_NUMPAD5:i=5;goto digit;
          case '4':case VK_NUMPAD4:i=4;goto digit;
          case '3':case VK_NUMPAD3:i=3;goto digit;
          case '2':case VK_NUMPAD2:i=2;goto digit;
          case '1':case VK_NUMPAD1:i=1;goto digit;
          case '0':case VK_NUMPAD0:i=0;
          digit:
           state.rule[i]=state.rulebar?!state.rule[i]:Ctrl()?0:1;//Shift()?0:!state.rule[i];
           DrawRules();
           //CheckDlgButton(hWndRule,UM_DlgRuleSum0+i,state.rule[i]?1:0);
           break;
          case VK_RETURN: if(Ctrl()||Shift()) switchfullscreen();else lbutton();break;
          case ' ':
          case VK_BACK:
          case VK_TAB:
          case VK_OEM_3:
          case VK_ADD: rbutton();break;
          case 'H':
          case VK_F1:
           printhelp();
           break;
          case 'N':{
           int r;
           if(Shift()||Ctrl()) {param.color2^=0xffffff;param.color3^=0xffffff;}
           else {r=param.color2;param.color2=param.color3;param.color3=r;}
           } break;
          case 'O':
          case 'K':
          case 'L':
          case 'F':if(!state.busy) {
               DWORD xy=GetMessagePos();
               POINT pt;
               char c1=wParam=='L';
               char mode=wParam=='O'?6:wParam=='K'?5:0;
               char diag=CapsLock(),wonly=!c1&&Ctrl();
               pt.x=LOWORD(xy),pt.y=HIWORD(xy);
               ScreenToClient(hWnd,&pt);
               if(!state.fullscreen&&state.rulebar&&pt.y>=param.size) break;
               if(wonly&&0xffffff!=(0xffffff&GetPixel(dc,pt.x,pt.y))) break;
               state.busy=1;
               Fill2(hWnd,pt.x,pt.y,Shift(),diag
                 ,c1?Ctrl()?0x0:0xffffff:param.color2,c1?Ctrl()?0x0:0xffffff:param.color3,mode);
             } break;
          case 'J':
          case 'G':if(!state.busy) {
              DWORD xy=GetMessagePos();
              POINT pt;
              pt.x=LOWORD(xy),pt.y=HIWORD(xy);
              ScreenToClient(hWnd,&pt);
              state.busy=1;
              if(wParam=='J') {
                int dst=Ctrl()?0x0:0xffffff;
                Gradient(hWnd,pt.x,pt.y,-1,dst,dst);
              } else
                Gradient(hWnd,pt.x,pt.y,Shift()?0xffffff:Ctrl()?0x0:-1,param.color2,param.color3);
           } break;
          case 'B':
           if(ChangeColor(hWnd,&param.color0))
             ((COLORREF*)bi->bmiColors)[0]=param.color0;
           Redraw(hWnd,0);
           break;
          case 'V':
           if(ChangeColor(hWnd,&param.color1))
             ((COLORREF*)bi->bmiColors)[1]=param.color1;
           Redraw(hWnd,0);
           break;
          case 'C':
           if(Ctrl()) {
             writeclip();
             break;
           }
           if(ChangeColor(hWnd,&param.color2))
             ChangeColor(hWnd,&param.color3);
           break;
          case 'T':if(Shift()||Ctrl()) {state.wback=!state.wback;Invert(1);} else Invert(0);break;
          case 'U':ColorPerm(hWnd,Shift(),Ctrl());break;
          case VK_DECIMAL:
          case 'M':
           state.mirror=Ctrl()?1:!state.mirror;
           UpdateMirror();
           break;
          case 'E':
           state.S[4]=!state.S[4];
           goto updateimage;
          case 'D': {
             void *r;
             StateDuality(state.D,state.S,state.size);
             r=state.S,state.S=state.D,state.D=r;
           }
           goto updateimage;
          case 'P':
           Boundary(hWnd,Ctrl()?2:!Shift());
           break;
          case VK_MULTIPLY:
          case 'I':
           if(Ctrl()) SwitchColors(hWnd);
           else StateInverse(state.S,state.size);
           goto updateimage;
          case VK_SUBTRACT:
          case 'X':
           StateRandomize(state.S,state.size);
          updateimage:
           State2Image(state.I,state.S,state.size);
           Redraw(hWnd,0);
           break;
          case VK_DIVIDE:
          case 'R':
           state.rulebar^=1;
           if(!state.fullscreen) {
             CheckWinSize(hWnd);
           } else {
             if(state.rulebar) DrawRules();
             else HideRules();
           }
           break;
          case 'W':
           writebmp();
           break;
          case VK_F11:switchfullscreen();break;
          case VK_ESCAPE:
           if(state.fullscreen) { switchfullscreen();break;}
          case 'Q':
           goto quit;
          default:
           if(KeyDown(wParam)) break;
	   return(DefWindowProc(hWnd, iMessage, wParam, lParam));
         }
         break;
        //case WM_LBUTTONDBLCLK: switchfullscreen(); break;
 	case WM_LBUTTONDOWN:{
           int x=(short)LOWORD(lParam),y=(short)HIWORD(lParam);
           if(state.rulebar&&(state.fullscreen?y<BARHEIGHT&&x<param.size:y>=param.size)) {
             int i=RuleIndex(x);
             if(i<10) {
               state.rule[i]^=1;
               DrawRules();
             } else {
               state.mirror^=1;
               UpdateMirror();
             }
             break;
           }
           lbutton();
         } break;
	case WM_RBUTTONDOWN:
         rbutton();
         break;
//	case WM_MOUSEMOVE:
          //SetFocus(hWnd);
          if(wParam&MK_RBUTTON) {
          }
          if(wParam&MK_LBUTTON) {
          }
	 break;
	default:
	 return(DefWindowProc(hWnd, iMessage, wParam, lParam));
  }
  return 0;
}

void printhelp() {
  static const char *Help=
   "cellscop [-s size] [-r rule] [-0 color0] [-1 color1]\n\n"
   "  -s size  : set half of window (for example: -s 127)\n"
   "  -r rule  : rule, sequence of digits for one in next step (-r123789)\n"
   "  -m       : mirror mode (-r 4567)\n"
   "  -0 color : set color 0, #rrggbb or #rgb or decimal or 0xrrggbb (-0 0)\n"
   "  -1 color : set color 1 (-1 #44ccff)\n"
   "  -i       : swap colors 0 and 1\n"
   "  -2 color : set fill/gradient color (-2 #ff0, -3 set 2nd gradient color)\n"
   "  -p sleep : sleep in milliseconds\n"
   "  -f       : fullscreen\n"
   "\nGood rules : -r123459 -r1357 -r1234567 -r123 -r1235 -r9870\n"
   "\nLeft Click : start automat / disable or enable window update  (Enter)\n"
   "Right Click: stop automat / one step  (Space or + on numeric)\n"
   "R          : change rule\n"
   "0-9        : switch rule (reset and set with Ctrl, when rulebar hidden)\n"
   "M          : mirror mode (-r 4567)\n"
   "I          : invert all cells (or / on numeric, -r 1)\n"
   "E          : invert central cell\n"
   "X          : randomize (or - on numeric, -r4567 -r36789 -r56789)\n"
   "D          : duality\n"
   "W          : write state image bmp\n"
   "Ctrl+C     : copy state image to clipboard\n"
   "C,V,B      : change fill/gradient,foreground,background color\n"
   "Ctrl+I     : swap foreground and background color (-r123456)\n"
   "N          : swap gradient colors (Ctrl|Shift invert them)\n"
   "F          : screen gradient flood fill (Shift for all octets,Ctrl white only)\n"
   "Caps Lock  : fill also diagonal (all 8 directions)\n"
   "K          : screen boundary flood fill (Shift for all octets,Ctrl white only)\n"
   "O          : screen boundary flood fill color replace\n"
   "L          : screen white flood fill (Ctrl for black,Shift for all octets)\n"
   "G          : screen gradient color replace (Shift - replace white,Ctrl - Black)\n"
   "J          : screen white color replace (Shift - replace white,Ctrl - Black)\n"
   "P          : expand black (Shift - just boundary from black,Ctrl - 8 direction)\n"
   "T          : screen invert (Shift|Ctrl - fullscren border)\n"
   "U          : screen rotate colors (Shift - back,Ctrl - rgb to cmy)\n"
   "F11        : fullscreen (or Ctrl|Alt+Enter)\n"
   "<F1>,H     : this help\n"
  ;
  MessageBox(NULL,Help,"CellScop " VERSION_STRING " Help",MB_OK);
}

char *StrWord(char *str,int wlen,char *word) {
  while(*str>0&&*str<=' ') str++;
  while(*str<0||*str>' ') {
    if(wlen>1) 
      wlen--,*word++=*str;
    str++;
  }
  if(wlen>0) 
    *word=0;
  return str;
}

int strtocolor(char **str) {
  char *h=*str,neg=0;
  int c=0,d,n=0;
  while(*h==' '||*h=='\t') h++;
  if(*h=='#') {
    char ch;
    h++;
   hex:
    ch=*h;
    if(ch>='0'&&ch<='9') d=ch-'0';
    else if(ch>='a'&&ch<='f') d=ch-'a'+10;
    else if(ch>='A'&&ch<='F') d=ch-'A'+10;
    else {
      if(n==1) c|=c<<4,c|=(c<<8)|(c<<16);
      else if(n==2) c|=(c<<8)|(c<<16);
      else if(n==3) c=((c&0xf00)<<8)|((c&0xf0)<<4)|(c&0xf),c|=c<<4;
      goto end;
    }
    c=(c<<4)+d;
    h++;n++;
    goto hex;
  } else if(*h=='0'&&(h[1]=='x'||h[1]=='X')) {
   h+=2;goto hex;
  } 
  if(*h=='-') neg=1,h++;
  while(*h>='0'&&*h<='9') {
    c=c*10+*h-'0';
    h++;
  }
  if(neg) c=((c>>16)&0xff)|(c&0xff00)|((c&0xff)<<16);
 end:
  *str=h;
  return c;
}

int PASCAL WinMain(HINSTANCE hCurInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
  MSG Message;
//  HMENU hmenu;
  WNDCLASS WndClass;
  char *h;

  for(h=lpCmdLine;*h;) {
    while(*h&&(*h==' '||*h=='\t')) h++;
    if(*h!='-')
      break;
    h++;
    puts(h);
    while(*h&&*h!=' '&&*h!='\t') {
      switch(*h++) {
       case 's':param.size=strtol(h,&h,0)|1;break;
       case 'r':h=StrWord(h,sizeof(param.rule),param.rule);break;
       case '0':param.color0=strtocolor(&h);break;
       case '1':param.color1=strtocolor(&h);break;
       case '2':param.color2=strtocolor(&h);break;
       case '3':param.color3=strtocolor(&h);break;
       case 'i':{int x=param.color0;param.color0=param.color1;param.color1=x;};break;
       case 'p':param.sleep=strtol(h,&h,0);break;
       case 'f':param.fullscreen=1;break;
       case 'm':param.mirror=1;break;
       case 'h':
       default:
        printhelp();
        return h[-1]!='h';
        break;
      }
    }
  }


  WndClass.cbClsExtra = 0;
  WndClass.cbWndExtra = 0;
  WndClass.hbrBackground = GetStockObject(BLACK_BRUSH);
  WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  WndClass.hIcon = LoadIcon (hCurInstance, "main_icon");
  WndClass.hInstance = hCurInstance;
  WndClass.lpfnWndProc = WndProc;
  WndClass.lpszClassName = "CellScop";
  WndClass.lpszMenuName = NULL;//"menu_main";
  WndClass.style = CS_HREDRAW | CS_VREDRAW ; //| CS_DBLCLKS;
  RegisterClass(&WndClass);

  //hInstance=hCurInstance;
  state.size=param.size;
  state.S=StateAlloc(state.size);
  state.D=StateAlloc(state.size);
  
  
  state.mirror=param.mirror;
  SetRule(param.rule);

  state.step=state.size/2;
  ((char*)state.S)[4]=1;

  bi=NULL;
  CreateBIH8(&bi,state.size,state.size,param.color0,param.color1,&state.I);

  { RECT r;
    DWORD st=WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX |WS_MAXIMIZEBOX;
 
    //rx=GetSystemMetrics(SM_CXFRAME);
    //ry=GetSystemMetrics(SM_CYFRAME);
    hWnd = CreateWindow ("CellScop","Cell Scope",st
       ,100,100,state.size,state.size,
       NULL,NULL,hCurInstance,NULL);
    dc=GetDC(hWnd);

    r.left=r.top=0;r.right=param.size;r.bottom=param.size+(state.rulebar?BARHEIGHT:0);
    AdjustWindowRect(&r,st,0);
    MoveWindow(hWnd,100,100,r.right-r.left,r.bottom-r.top,0);

    if(param.fullscreen) switchfullscreen();
    else ShowWindow (hWnd, SW_NORMAL);
    
  }

  UpdateWindow(hWnd);

  for(;;) {
   peek:
    if(state.step) {
      if(!PeekMessage (&Message, 0, 0, 0,PM_REMOVE)) {
        if(state.busy) state.busy=0;
        goto step;
      }
      else
        if(Message.message==WM_QUIT) 
          break;
    } else {
      DWORD tick=GetTickCount();
      if(!GetMessage(&Message,0,0,0))
        break;
      if(state.busy&&GetTickCount()!=tick) state.busy=0;
    }
    TranslateMessage(&Message);
    DispatchMessage(&Message);
    goto peek;
   step:
    if(state.sleep>0) {
      if(state.sleep>=100) {
        state.sleep-=100;
        Sleep(100);
      } else {
        Sleep(state.sleep);
        state.sleep=0;
      }
      if(state.sleep>0) continue;
    }
    { void *r;
      State2State(state.D,state.S,state.size,state.rule);
      r=state.S,state.S=state.D,state.D=r;
      if(!state.noblt) {
        State2Image(state.I,state.S,state.size);
        state.fullpos++;
        Redraw(hWnd,0);
        if(state.step>state.fullnum) state.sleep=param.sleep;
      }
    }
    state.step--;
    state.steps++;
    if(!state.step) {
      char buf[32];
      sprintf(buf,"%.0f",state.steps);
      SetWindowText(hWnd,buf);
    }
  }

  free(state.S);
  free(state.D);
  free(bi);
  free(state.I);

  return Message.wParam;
}

