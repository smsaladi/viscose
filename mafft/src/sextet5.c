#include "mltaln.h"
#include "mtxutl.h"

#define DEBUG 0
#define TEST  0

#define END_OF_VEC -1

static int maxl;
static int tsize;

void arguments( int argc, char *argv[] )
{
	int c;

	disopt = 0;
	scoremtx = NOTSPECIFIED;

    while( --argc > 0 && (*++argv)[0] == '-' )
        while ( c = *++argv[0] )
            switch( c )
            {
				case 'D':
					scoremtx = -1;
					break;
				case 'P':
					scoremtx = 0;
					break;
				case 'i':
					disopt = 1;
					break;
                default:
                    fprintf( stderr, "illegal option %c\n", c );
                    argc = 0;
                    break;
            }
    if( argc != 0 )
    {
        fprintf( stderr, "options: -i\n" );
        exit( 1 );
    }
}

void seq_grp_nuc( int *grp, char *seq )
{
	int tmp;
	while( *seq )
	{
		tmp = amino_grp[*seq++];
		if( tmp < 4 )
			*grp++ = tmp;
		else
			fprintf( stderr, "WARNING : Unknown character %c\n", *(seq-1) );
	}
	*grp = END_OF_VEC;
}

void seq_grp( int *grp, char *seq )
{
	int tmp;
	while( *seq )
	{
		tmp = amino_grp[*seq++];
		if( tmp < 6 )
			*grp++ = tmp;
		else
			fprintf( stderr, "WARNING : Unknown character %c\n", *(seq-1) );
	}
	*grp = END_OF_VEC;
}

void makecompositiontable_p( short *table, int *pointt )
{
	int point;

	while( ( point = *pointt++ ) != END_OF_VEC )
		table[point]++;
}

int commonsextet_p( short *table, int *pointt )
{
	int value = 0;
	short tmp;
	int point;
	static short *memo = NULL;
	static int *ct = NULL;
	static int *cp;

	if( !memo )
	{
		memo = calloc( tsize, sizeof( short ) );
		if( !memo ) ErrorExit( "Cannot allocate memo\n" );
		ct = calloc( MIN( maxl, tsize), sizeof( int ) );
		if( !ct ) ErrorExit( "Cannot allocate memo\n" );
	}

	cp = ct;
	while( ( point = *pointt++ ) != END_OF_VEC )
	{
		tmp = memo[point]++;
		if( tmp < table[point] )
			value++;
		if( tmp == 0 ) *cp++ = point;
	}
	*cp = END_OF_VEC;
	
	cp =  ct;
	while( *cp != END_OF_VEC )
		memo[*cp++] = 0;

	return( value );
}

void makepointtable_nuc( int *pointt, int *n )
{
	int point;
	register int *p;

	p = n;
	point  = *n++ *  1024;
	point += *n++ *   256;
	point += *n++ *    64;
	point += *n++ *    16;
	point += *n++ *     4;
	point += *n++;
	*pointt++ = point;

	while( *n != END_OF_VEC )
	{
		point -= *p++ * 1024;
		point *= 4;
		point += *n++;
		*pointt++ = point;
	}
	*pointt = END_OF_VEC;
}

void makepointtable( int *pointt, int *n )
{
	int point;
	register int *p;

	p = n;
	point  = *n++ *  7776;
	point += *n++ *  1296;
	point += *n++ *   216;
	point += *n++ *    36;
	point += *n++ *     6;
	point += *n++;
	*pointt++ = point;

	while( *n != END_OF_VEC )
	{
		point -= *p++ * 7776;
		point *= 6;
		point += *n++;
		*pointt++ = point;
	}
	*pointt = END_OF_VEC;
}

int main( int argc, char **argv )
{
	int i, j;
	FILE *fp;
	char **seq;
	int *grpseq;
	char *tmpseq;
	int  **pointt;
	static char name[M][B];
	static int nlen[M];
	double **mtx;
	double **mtx2;
	double score, score0;
	static short *table1;
	char b[B];

	arguments( argc, argv );

#if 0
	PreRead( stdin, &njob, &nlenmax );
#else
	getnumlen( stdin );
#endif
	rewind( stdin );
	if( njob < 2 )
	{
		fprintf( stderr, "At least 2 sequences should be input!\n"
						 "Only %d sequence found.\n", njob );
		exit( 1 );
	}

	tmpseq = AllocateCharVec( nlenmax+1 );
	seq = AllocateCharMtx( njob, nlenmax+1 );
	grpseq = AllocateIntVec( nlenmax+1 );
	pointt = AllocateIntMtx( njob, nlenmax+1 );
	mtx = AllocateDoubleMtx( njob, njob );
	mtx2 = AllocateDoubleMtx( njob, njob );
	pamN = NOTSPECIFIED;

#if 0
	FRead( stdin, name, nlen, seq );
#else
	readData( stdin, name, nlen, seq );
#endif
	constants();

	if( scoremtx == -1 ) tsize = (int)pow( 4, 6 );
	else                 tsize = (int)pow( 6, 6 );

	maxl = 0;
	for( i=0; i<njob; i++ ) 
	{
		gappick0( tmpseq, seq[i] );
		nlen[i] = strlen( tmpseq );
		if( nlen[i] > maxl ) maxl = nlen[i];
		if( scoremtx == -1 ) /* nuc */
		{
			seq_grp_nuc( grpseq, tmpseq );
			makepointtable_nuc( pointt[i], grpseq );
		}
		else                 /* amino */
		{
			seq_grp( grpseq, tmpseq );
			makepointtable( pointt[i], grpseq );
		}
	}
	for( i=0; i<njob; i++ )
	{
		table1 = calloc( tsize, sizeof( short ) );
		if( !table1 ) ErrorExit( "Cannot allocate table1\n" );
		if( i % 10 == 0 )
		{
			fprintf( stderr, "%#4d / %#4d\r", i+1, njob );
		}
		makecompositiontable_p( table1, pointt[i] );

		for( j=i; j<njob; j++ ) 
		{
			score = (double)commonsextet_p( table1, pointt[j] );
			mtx[i][j] = score;
		} 
		free( table1 );
	}
	for( i=0; i<njob; i++ )
	{
		score0 = mtx[i][i];
		for( j=0; j<njob; j++ ) 
			mtx2[i][j] = ( score0 - mtx[MIN(i,j)][MAX(i,j)] ) / score0 * 3.0;
	}
	for( i=0; i<njob-1; i++ ) for( j=i+1; j<njob; j++ ) 
	{
#if TEST
                double jscore;
                jscore = mtx[i][j] / ( MIN( strlen( seq[i] ), strlen( seq[j] ) ) - 2 );
                fprintf( stdout, "jscore = %f\n", jscore );

		fprintf( stdout, "mtx2[%d][%d] = %f, mtx2[%d][%d] = %f\n", i, j, mtx2[i][j], j, i, mtx2[j][i] );
#endif
		mtx2[i][j] = MIN( mtx2[i][j], mtx2[j][i] );
#if TEST
		fprintf( stdout, "sonokekka mtx2[%d][%d] %f\n", i, j, mtx2[i][j] );
#endif
	}

	if( disopt )
	{
		for( i=0; i<njob; i++ ) 
		{
			sprintf( b, "=lgth = %#04d", nlen[i] );
			strins( b, name[i] );
		}
	}
		
	fp = fopen( "hat2", "w" );
	if( !fp ) ErrorExit( "Cannot open hat2." );
	WriteHat2( fp, njob, name, mtx2 );
	fclose( fp );

	fprintf( stderr, "\n" );
	SHOWVERSION;
	exit( 0 );
}
