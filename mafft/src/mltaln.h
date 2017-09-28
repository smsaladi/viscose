#define USE_XCED 0

#if USE_XCED
#include "config.h"
#include "share.h"
#else
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include "mtxutl.h"

#define SHOWVERSION fprintf( stderr, "%s (%s) Version 3.89\n", progName( argv[0] ), (scoremtx==-1)?"nuc":"aa" )

#define FFT_THRESHOLD  70
#define FFT_WINSIZE_P   30
#define FFT_WINSIZE_D   100
#define DISPSEQF  60
#define DISPSITEI    0
#define MAXITERATION 500
#define M   10000       /* njob no saidaiti */
#define N  500000       /* nlen no saidaiti */
#define MAXSEG 100000
#define B     256
#define C     60       /*  1 gyou no mojisuu */
#define D      6
#define rnd() ( ( 1.0 / ( RAND_MAX + 1.0 ) ) * rand() )
#define MAX(X,Y)    ( ((X)>(Y))?(X):(Y) )
#define MIN(X,Y)    ( ((X)<(Y))?(X):(Y) )
#define G(X)        ( ((X)>(0))?(X):(0) )
#define BEFF   1.0   /* 0.6 ni suruto zureru */
#define WIN   3
#define SGAPP -1000
#define GETA2 0.001
#define GETA3 0.001
#define NOTSPECIFIED  100009
#define SUEFF   0.1  /* upg/(spg+upg)  -> sueff.sed */ 

int njob, nlenmax;
int amino_n[0x80];
char amino_grp[0x80];
int amino_dis[0x80][0x80];
int n_dis[26][26];
char amino[26];
double polarity[20];
double volume[20];

int ppid;
int contin;
int calledByXced;
int devide;
int scmtd;
int weight;
int utree;
int tbutree;
int refine;
int check;
double cut;
int cooling;
int penalty, ppenalty;
int penalty_ex, ppenalty_ex;
int offset, poffset;
int scoremtx;
char use_fft;
int fftscore;
int fftWinSize;
int fftThreshold;
int fftRepeatStop;
int fftNoAnchStop;
int divWinSize;
int divThreshold;
int disp;
int outgap;
char alg;
int cnst;
int mix;
int tbitr;
int tbweight;
int tbrweight;
int disopt;
int pamN;
int checkC;
float geta2;
int treemethod;
int kimuraR;

char *signalSM;
FILE *prep_g;
FILE *trap_g;

/* sengen no ichi ha koko dake de ha nai */
extern void constants();
extern char **Calignm1();
extern char **Dalignm1();
extern char **align0();
extern double Cscore_m_1( char **, int, int, double ** );
extern double score_m_1(  char **, int, int, double ** );
extern double score_calc0( char **, int, double **, int );
extern char seqcheck( char ** );
extern float substitution( char *, char * );
extern float substitution_hosei( char *, char * );
extern double ipower( double, int );
extern float translate_and_Calign();
extern float A__align();
extern float Falign();
extern float Aalign();

typedef struct _NodeInCub
{
	int step;
	int LorR;
} NodeInCub;

typedef struct _Node
{
	struct _Node *children[3];
	int tmpChildren[3];
	double length[3];
	double *weightptr[3];
	int top[3];
	int *members[3];
} Node;

typedef struct _Segment
{
	int start;
	int end;
	int center;
	double score;
	int skipForeward;
	int skipBackward;
	struct _Segment *pair;
	int  number;
} Segment;

typedef struct _Segments
{
	Segment group1;
	Segment group2;
	int number1;
	int number2;
} Segments;


char **seq_g;
char **res_g;
