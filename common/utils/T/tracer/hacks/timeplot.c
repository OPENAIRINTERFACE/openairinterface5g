#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

void RESET(void)
{ printf("\x1b""c"); fflush(stdout); }

void GOTO(int x, int y)
{ char s[64];sprintf(s,"\x1b[%d;%dH",y,x);printf("%s",s);fflush(stdout); }

void HIDE_CURSOR(void)
{ printf("\x1b[?25l"); fflush(stdout); }

void sig(int x)
{
  RESET();
  exit(0);
}

/* 0: [0-2[
 * ...
 * 48: [96-98[
 * 49: [98-infinity]
 */
int bins[50];

#define N 1000
int data[N];
long start = 100;

void plot(void)
{
  int i;
  int max = 0;
  int vmin = data[0];
  int vmax = 0;
  long vavg;
  memset(bins, 0, sizeof(bins));
  int binsize_ns = 500;

  vavg = 0;

  for (i = 0; i < N; i++) {
    if (data[i] < vmin) vmin = data[i];
    if (data[i] > vmax) vmax = data[i];
    vavg += data[i];
    int ms2 = (data[i] - start * 1000)/binsize_ns;
    if (ms2 < 0) ms2 = 0;
    if (ms2 > 49) ms2 = 49;
    bins[ms2]++;
    if (bins[ms2] > max) max = bins[ms2];
  }

  vavg /= N;

  GOTO(1,1);
  for (i = 0; i < 50; i++) {
    double binend = (i+1) * binsize_ns / 1000. + start;
    int k;
    int width = bins[i] * 70 / max;
    /* force at least width of 1 if some point is there */
    if (bins[i] && width == 0) width = 1;
    printf("%#5.1f ", binend);
    for (k = 0; k < width; k++) putchar('#');
    for (; k < 70; k++) putchar(' ');
    putchar('\n');
  }
  printf("min %d ns    max %d ns    avg %ld ns\n", vmin, vmax, vavg);
}

void up(int x)
{
  start += 5;
}

void down(int x)
{
  start -= 5;
}

int main(void)
{
  int i;
  int pos = 0;
  signal(SIGINT, sig);
  signal(SIGUSR1, up);
  signal(SIGUSR2, down);
  RESET();
  HIDE_CURSOR();
  while (!feof(stdin)) {
    for (i=0; i<100; i++) { scanf("%d", &data[pos]); pos++; pos%=N; }
    plot();
  }
  RESET();
  return 0;
}
