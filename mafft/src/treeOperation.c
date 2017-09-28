#include "mltaln.h"

#define DEBUG 0

#define EF_THREEWAY 1.0
#define MAXBW 1.0
#define MINBW 0.01

#define MINLEN 0.001

#if DEBUG
Node *stopol_g;
#endif

void checkMinusLength( int nseq, double **len )
{
	int i, j;
	for( i=0; i<nseq-1; i++ ) for( j=0; j<2; j++ ) 
		if( len[i][j] < MINLEN ) len[i][j] = MINLEN;
}

int *negativeMember( int *query, int locnseq )
{
	int *bk, *value;
	char *tmp;
	int i;
	int n;

	tmp = AllocateCharVec( locnseq );
	bk = value = AllocateIntVec( locnseq );
	if( !value ) ErrorExit( "Cannot allocate value" );

	for( i=0; i<locnseq; i++ ) tmp[i] = 0;
	while( (n=*query++) != -1 ) tmp[n] = 1;

	for( i=0; i<locnseq; i++ ) 
		if( !tmp[i] ) *value++ = i;
	*value = -1;
	free( tmp );
	return( bk );
}

int IntExistsInVec( int query, int *vector )
{
	while( *vector != -1 )
		if( query == *vector++ ) return( 1 );
	return( 0 );
}

NodeInCub searchParent( int top, int ***topol, int Start, int End )
{
	int i, j;
	NodeInCub value;
	for( i=Start; i<End; i++ ) 
	{
		for( j=0; j<2; j++ ) 
		{
			if( IntExistsInVec( top, topol[i][j] ) )
			{
				value.step = i; 
				value.LorR = j; 
				return( value );
			}
		}
	}
	fprintf( stderr, "ERROR!!!\n" );
	ErrorExit( "Error in searchParent" );
}

void stopolInit( int n, Node *stopol )
{
	int i, j;
	for( i=0; i<n; i++ )
	{
		for( j=0; j<3; j++ ) 
		{	
			stopol[i].length[j] = 0.0;
			stopol[i].children[j] = NULL;
			stopol[i].tmpChildren[j] = -1;
			stopol[i].top[j] = -1;
			stopol[i].members[j] = NULL;
			stopol[i].weightptr[j] = NULL;
		}
	}
}

void treeCnv( Node *stopol, int locnseq, int ***topol, double **len, double **bw )
{
	int i;
	NodeInCub parent;
	char *count;
	char ccount;
	int rep;
	int tmpint;

	count = AllocateCharVec(  2 * locnseq ); /* oome */
	if( !count ) ErrorExit( "Cannot allocate count.\n" );

	checkMinusLength( locnseq, len ); /* uwagaki */

	stopolInit( locnseq * 2, stopol );
	for( i=0; i<locnseq * 2; i++ ) count[i] = 0;

	for( i=locnseq; i<locnseq*2; i++ ) 
	{
		rep = i - locnseq;
		parent = searchParent( rep, topol, 0, locnseq-1 ); 
#if DEBUG
		fprintf( stderr, "Parent of node No.%d ( Seq No.%d )  = %d - %d\n", i, i-locnseq, parent.step, parent.LorR );
#endif

		ccount = count[parent.step];
		stopol[parent.step].length[ccount] = len[parent.step][parent.LorR];
		stopol[parent.step].weightptr[ccount] = &(bw[parent.step][parent.LorR]);
		stopol[parent.step].children[ccount] = &stopol[i];
		stopol[parent.step].tmpChildren[ccount] = i;
		stopol[parent.step].members[ccount] = topol[parent.step][parent.LorR];
		count[parent.step]++;

		ccount = count[i];
		stopol[i].length[ccount] = len[parent.step][parent.LorR];
		stopol[i].weightptr[ccount] = &(bw[parent.step][parent.LorR]);
		stopol[i].children[ccount] = &stopol[parent.step];
		stopol[i].tmpChildren[ccount] = parent.step;
		stopol[i].members[ccount] = topol[parent.step][parent.LorR];
		count[i]++;
	}
	for( i=0; i<locnseq-2; i++ ) 
	{
		rep = MIN( topol[i][0][0], topol[i][1][0] );
		parent = searchParent( rep, topol, i+1, locnseq-1 ); 
		ccount = count[parent.step];
		stopol[parent.step].length[ccount] = len[parent.step][parent.LorR];
		stopol[parent.step].weightptr[ccount] = &(bw[parent.step][parent.LorR]);
		stopol[parent.step].children[ccount] = &stopol[i];
		stopol[parent.step].tmpChildren[ccount] = i;
		stopol[parent.step].members[ccount] = topol[parent.step][parent.LorR];
		count[parent.step]++;

		ccount = count[i];
		stopol[i].length[ccount] = len[parent.step][parent.LorR];
		stopol[i].weightptr[ccount] = &(bw[parent.step][parent.LorR]);
		stopol[i].children[ccount] = &stopol[parent.step];
		stopol[i].tmpChildren[ccount] = parent.step;
		stopol[i].members[ccount] = negativeMember( topol[parent.step][parent.LorR], locnseq );
		count[i]++;
#if DEBUG
		fprintf( stderr, "Parent of node No.%d = %d - %d\n", i, parent.step, parent.LorR );
#endif
	}
/*
			Unrooted tree.
			locnseq-2 no children no nakade, 
			locnseq-3 wo sashiteinai mono wo sagashite,
			locnseq-3 no children ni kuwae,
			locnseq-2 wo sashiteita node no chilren wo
			locnseq-3 ni kaeru.
*/
#if DEBUG
	fprintf( stderr, "BEFORE MODIFY\n" );
	for( i=0; i<locnseq*2; i++ )  
	{
		for( j=0; j<3; j++ ) 
		{
			fprintf( stderr, "stopol[%d].tmpChildren[%d] = %d, children[%d] = %d \n", i, j, stopol[i].tmpChildren[j], j, stopol[i].children[j] - stopol );
		}
	}
#endif

	if     ( stopol[locnseq-2].children[0] == &stopol[locnseq-3] ) i = 1;
	else if( stopol[locnseq-2].children[1] == &stopol[locnseq-3] ) i = 0;
	else ErrorExit( "?\n" );

	stopol[locnseq-3].length[2] = len[locnseq-2][0] + len[locnseq-2][1];
	stopol[locnseq-3].weightptr[2] = &bw[locnseq-2][0];
	stopol[locnseq-3].children[2] = stopol[locnseq-2].children[i];
	stopol[locnseq-3].tmpChildren[2] = stopol[locnseq-2].tmpChildren[i];

	tmpint = (int)( stopol[locnseq-2].children[i] - stopol );

	stopol[tmpint].children[2] = &stopol[locnseq-3];
	stopol[tmpint].length[2] = len[locnseq-2][0] + len[locnseq-2][1];
	stopol[tmpint].weightptr[2] = &bw[locnseq-2][0];
	stopol[tmpint].tmpChildren[2] = locnseq-3;


#if DEBUG
	for( i=0; i<locnseq*2; i++ )  
	{
		for( j=0; j<3; j++ ) 
		{
			fprintf( stderr, "stopol[%d].tmpChildren[%d] = %d, children[%d] = %d \n", i, j, stopol[i].tmpChildren[j], j, stopol[i].children[j] - stopol );
		}
	}

	for( i=0; i<locnseq*2; i++ )  
	{
		fprintf( stderr, "-- stopol[%d]\n", i );
		for( j=0; j<3; j++ ) 
		{
			if( !stopol[i].members[j] ) 
			{
				fprintf( stderr, "LEAF\n" );
				break;
			}
			fprintf( stderr, "	group %d are \n", j );
			for( k=0; (n=stopol[i].members[j][k]) != -1; k++ ) 
			{	
				fprintf( stderr, "%#5d", n );
			}
			fprintf( stderr, "\n" );
		}
		fprintf( stderr, "\n" );
	}
#endif

#if DEBUG
	stopol_g = stopol;
#endif
}

int isLeaf( Node node )
{
	if( node.children[1] ) return( 0 );
	else                   return( 1 );
}

double syntheticLength( Node *ob, Node *oppositeNode )
{
	int i, count;
	int dir_ch[3], dir_pa;
	double value, tmpvalue0, tmpvalue1;
	int nanflag = 0;

#if DEBUG
	fprintf( stderr, "In syntheticLength\n" );
	fprintf( stderr, "ob - stopol_g = %d\n", ob - stopol_g );
	fprintf( stderr, "op - stopol_g = %d\n", oppositeNode - stopol_g );
#endif

	if( isLeaf( *ob ) ) 
	{
#if DEBUG
		fprintf( stderr, "LEAF\n\n" );
#endif
		return( ob->length[0] );
	}

	for( i=0, count=0; i<3; i++ ) 
	{
#if DEBUG
		fprintf( stderr, "ob->tmpChildren[%d] = %d\n", i, ob->tmpChildren[i] );
#endif
		if( oppositeNode != ob->children[i] ) dir_ch[count++] = i;
		else dir_pa = i;
	}
#if DEBUG
		fprintf( stderr, "\n" );
#endif
	if( count != 2 ) 
	{
#if DEBUG
		fprintf( stderr, "Node No.%d has no child like No.%d \n", ob-stopol_g, oppositeNode-stopol_g );
#endif
		ErrorExit( "Invalid call\n" );
	}

	tmpvalue0 = syntheticLength( ob->children[dir_ch[0]], ob ); 
	tmpvalue1 = syntheticLength( ob->children[dir_ch[1]], ob ); 

#if DEBUG
	fprintf( stderr, "tmpvalue0 = %f\n", tmpvalue0 );
	fprintf( stderr, "tmpvalue1 = %f\n", tmpvalue1 );
#endif	
	if( tmpvalue0 ) tmpvalue0 = 1.0 / tmpvalue0;
	else nanflag = 1;
	if( tmpvalue1 ) tmpvalue1 = 1.0 / tmpvalue1;
	else nanflag = 1;

	if( nanflag ) value = 0.0;
	else
	{
		value = tmpvalue0 + tmpvalue1;
		value = 1.0 / value;
	}
	value += ob->length[dir_pa];
#if DEBUG
	fprintf( stderr, "value = %f\n", value  );
#endif

	return( value );
}

double calcW( Node *ob, Node *op )
{
	int i, count;
	int dir_ch[3], dir_pa;
	double a, b, c, f, s;
	double value;

	if( isLeaf( *ob ) ) 
		return( 1.0 );

	for( i=0, count=0; i<3; i++ ) 
	{
		if( op != ob->children[i] ) dir_ch[count++] = i;
		else dir_pa = i;
	}
	if( count != 2 ) ErrorExit( "Invalid call of calcW\n" );

#if DEBUG
	fprintf( stderr, "In calcW\n" );
	fprintf( stderr, "ob = %d\n", ob - stopol_g );
	fprintf( stderr, "op = %d\n", op - stopol_g );
	fprintf( stderr, "ob->children[c1] = %d\n", ob->children[dir_ch[0]] - stopol_g );
	fprintf( stderr, "ob->children[c2] = %d\n", ob->children[dir_ch[1]] - stopol_g );
	fprintf( stderr, "ob->children[pa] = %d\n", ob->children[dir_pa] - stopol_g );
	fprintf( stderr, "\n" );
#endif

	a = syntheticLength( ob->children[dir_ch[0]], ob );
	b = syntheticLength( ob->children[dir_ch[1]], ob );
	c = syntheticLength( ob->children[dir_pa], ob );

#if DEBUG
	fprintf( stderr, "a = %f\n", a );
	fprintf( stderr, "b = %f\n", b );
	fprintf( stderr, "c = %f\n", c );
#endif

	if( !c ) return( MAXBW );
	if ( !a || !b ) return( MINBW );  /* ? */

	f = EF_THREEWAY;
	s = ( b*c + c*a + a*b );

	value = a*b*(c+a)*(c+b) / ( c*(a+b) * f * s );

	value = sqrt( value );

	return( value );
}

void calcBranchWeight( double **bw, int locnseq, Node *stopol, int ***topol, double **len )
{
	NodeInCub parent;
	int i;
	int rep;
	Node *topNode, *btmNode;
	double topW, btmW;

	for( i=locnseq; i<locnseq*2; i++ )
	{
		rep = i - locnseq;
		parent = searchParent( rep, topol, 0, locnseq-1 );
		if( parent.step == locnseq - 2 ) continue;

		topNode = stopol+parent.step; btmNode = stopol+i;
#if DEBUG
		fprintf( stderr, "In calcBranchWeight, topNode=%d, btmNode=%d\n", topNode-stopol_g, btmNode-stopol_g );
#endif
		topW = calcW( topNode, btmNode );
		btmW = calcW( btmNode, topNode );
		bw[parent.step][parent.LorR] = topW * btmW;
	}
	for( i=0; i<locnseq-3; i++ ) 
	{
		rep = MIN( topol[i][0][0], topol[i][1][0] );
		parent = searchParent( rep, topol, i+1, locnseq-1 ); 
		if( parent.step == locnseq - 2 ) continue;
		topNode = stopol+parent.step;
		btmNode = stopol+i;
#if DEBUG
		fprintf( stderr, "In calcBranchWeight, topNode=%d, btmNode=%d\n", topNode-stopol_g, btmNode-stopol_g );
#endif
		topW = calcW( topNode, btmNode );
		btmW = calcW( btmNode, topNode );
		bw[parent.step][parent.LorR] = topW * btmW;
	}

	topNode = stopol[locnseq-3].children[2];
	btmNode = stopol + i;
	topW = calcW( topNode, btmNode );
	btmW = calcW( btmNode, topNode );
	bw[locnseq-2][0] = topW * btmW;
	bw[locnseq-2][1] = 1.0;
}

void branchWeightToPairWeight( int locnseq, int ***topol, double **pw, double **bw )
{
	int i, j, k, n0, n1;
#if 0
	double wFromLeaf[locnseq];
#else
	static double *wFromLeaf = NULL;
	if( wFromLeaf == NULL )
		wFromLeaf = AllocateDoubleVec( locnseq );
#endif

#if DEBUG
	for( i=0; i<locnseq-1; i++ ) for( j=0; j<2; j++ ) 
		fprintf( stderr, "pw[%d][%d] = %f\n", i, j, bw[i][j] );
#endif

	for( i=0; i<locnseq; i++ ) wFromLeaf[i] = 1.0;
	for( i=0; i<locnseq; i++ ) for( j=0; j<locnseq; j++ ) 
		pw[i][j] = 0.0;
	for( i=0; i<locnseq-1; i++ ) 
	{
		for( j=0; (n0=topol[i][0][j])!=-1; j++ ) 
			for( k=0; (n1=topol[i][1][k])!=-1; k++ ) 
				pw[MIN( n0, n1 )][MAX( n0, n1 )] 
				= wFromLeaf[n0] * wFromLeaf[n1] * bw[i][0] * bw[i][1];
		for( j=0; (n0=topol[i][0][j])!=-1; j++ ) 
			wFromLeaf[n0] *= bw[i][0];
		for( j=0; (n1=topol[i][1][j])!=-1; j++ ) 
			wFromLeaf[n1] *= bw[i][1];
	}
}

void weightFromABranch_rec( double *result, Node *ob, Node *op )
{
	int i, n, count;
	int dir_ch[3], dir_pa;

#if DEBUG
	fprintf( stderr, "In weightFromABranch_rec, ob = %d\n", ob - stopol_g );
#endif
	if( isLeaf( *ob ) ) return;
	for( i=0, count=0; i<3; i++ ) 
	{
		if( ob->children[i] != op ) dir_ch[count++] = i;
		else dir_pa = i;
	}
	if( count != 2 ) 
	{
#if DEBUG
		fprintf( stderr, "Node No.%d has no child like No.%d \n", ob-stopol_g, op-stopol_g );
#endif
		ErrorExit( "Incorrect call of weightFromABranch_rec" );
	}
	for( i=0; (n=ob->members[dir_ch[0]][i])!=-1; i++ ) 
		result[n] *= *ob->weightptr[dir_ch[0]];
	weightFromABranch_rec( result, ob->children[dir_ch[0]], ob );

	for( i=0; (n=ob->members[dir_ch[1]][i])!=-1; i++ ) 
		result[n] *= *ob->weightptr[dir_ch[1]];
	weightFromABranch_rec( result, ob->children[dir_ch[1]], ob );
}

void weightFromABranch( int nseq, double *result, Node *stopol, int ***topol, int step, int LorR )
{
	Node *topNode, *btmNode;
	int i;

	if( step == nseq - 2 )
	{
		topNode = stopol[nseq-2].children[0];
		btmNode = stopol + nseq-3;
#if DEBUG
		fprintf( stderr, "Now step == nseq-3, topNode = %d, btmNode = %d\n", topNode - stopol_g, btmNode-stopol_g );
#endif
	}
		
	else
	{
		for( i=0; i<3; i++ ) 
		{
			if( stopol[step].members[i][0] == topol[step][LorR][0] )
			break;
		}
		if( i== 3 ) ErrorExit( "Incorrect call of weightFromABranch." );
		btmNode = stopol[step].children[i];
		topNode = stopol+step;
	}

	for( i=0; i<nseq; i++ ) result[i] = 1.0;
	weightFromABranch_rec( result, btmNode, topNode ); 
	weightFromABranch_rec( result, topNode, btmNode ); 
}
