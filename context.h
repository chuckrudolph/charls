// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#pragma once


#define MIN_C -128
#define MAX_C 127

//
// JlsContext: a JPEG-LS context with it's current statistics.
//
struct JlsContext
{
public:
	JlsContext() 
	{}

 	JlsContext(int a) :
		A(a),
		B(0),
		C(0),
		N(1)
	{
	}

	UINT A;
	int B;
	int N;
	int C;

	inlinehint int GetErrorCorrection(int k) const
	{
		if (k != 0)
			return 0;

		return BitWiseSign(2 * B + N - 1);
	}


	inlinehint void UpdateVariables(int Errval, int NEAR, int NRESET)
	{
		ASSERT(N != 0);

		B = B + Errval * (2 * NEAR + 1); 
		A = A + abs(Errval);
		
		ASSERT(A < 65536 * 256);
		ASSERT(abs(B) < 65536 * 256);

		if (N == NRESET) 
		{
			A = A >> 1;
			B = B >> 1;
			N = N >> 1;
		}

		N = N + 1;

		if (B <= - N) 
		{
			B = max(-N + 1, B + N);
			if (C > MIN_C)
			{
				C--;
			}
		} 
		if (B > 0) 
		{
			B = min(B - N, 0);				
			if (C < MAX_C) 
			{
				C++;
			}

		}
		ASSERT(N != 0);
	}



	inlinehint int GetGolomb() const
	{
		UINT Ntest	= N;
		UINT Atest	= A;
		UINT k = 0;
		for(; (Ntest << k) < Atest; k++) 
		{ 
			ASSERT(k <= 32); 
		};
		return k;
	}

};
