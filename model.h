#define NUM_VERTICES 192
#define NUM_FACES 124

// x,y,z,padding (for 32bit alignment)
static short model_vertices[] = {
0,10,-10,0,
0,10,-8,0,
1,9,-8,0,
1,9,-10,0,
1,9,-10,0,
1,9,-8,0,
3,9,-8,0,
3,9,-10,0,
3,9,-10,0,
3,9,-8,0,
5,8,-8,0,
5,8,-10,0,
5,8,-10,0,
5,8,-8,0,
7,7,-8,0,
7,7,-10,0,
7,7,-10,0,
7,7,-8,0,
8,5,-8,0,
8,5,-10,0,
8,5,-10,0,
8,5,-8,0,
9,3,-8,0,
9,3,-10,0,
9,3,-10,0,
9,3,-8,0,
9,1,-8,0,
9,1,-10,0,
9,1,-10,0,
9,1,-8,0,
10,0,-8,0,
10,0,-10,0,
10,0,-10,0,
10,0,-8,0,
9,-1,-8,0,
9,-1,-10,0,
9,-1,-10,0,
9,-1,-8,0,
9,-3,-8,0,
9,-3,-10,0,
9,-3,-10,0,
9,-3,-8,0,
8,-5,-8,0,
8,-5,-10,0,
8,-5,-10,0,
8,-5,-8,0,
7,-7,-8,0,
7,-7,-10,0,
7,-7,-10,0,
7,-7,-8,0,
5,-8,-8,0,
5,-8,-10,0,
5,-8,-10,0,
5,-8,-8,0,
3,-9,-8,0,
3,-9,-10,0,
3,-9,-10,0,
3,-9,-8,0,
1,-9,-8,0,
1,-9,-10,0,
1,-9,-10,0,
1,-9,-8,0,
0,-10,-8,0,
0,-10,-10,0,
0,-10,-10,0,
0,-10,-8,0,
-1,-9,-8,0,
-1,-9,-10,0,
-1,-9,-10,0,
-1,-9,-8,0,
-3,-9,-8,0,
-3,-9,-10,0,
-3,-9,-10,0,
-3,-9,-8,0,
-5,-8,-8,0,
-5,-8,-10,0,
-5,-8,-10,0,
-5,-8,-8,0,
-7,-7,-8,0,
-7,-7,-10,0,
-7,-7,-10,0,
-7,-7,-8,0,
-8,-5,-8,0,
-8,-5,-10,0,
-8,-5,-10,0,
-8,-5,-8,0,
-9,-3,-8,0,
-9,-3,-10,0,
-9,-3,-10,0,
-9,-3,-8,0,
-9,-1,-8,0,
-9,-1,-10,0,
-9,-1,-10,0,
-9,-1,-8,0,
-10,0,-8,0,
-10,0,-10,0,
-10,0,-10,0,
-10,0,-8,0,
-9,1,-8,0,
-9,1,-10,0,
-9,1,-10,0,
-9,1,-8,0,
-9,3,-8,0,
-9,3,-10,0,
-9,3,-10,0,
-9,3,-8,0,
-8,5,-8,0,
-8,5,-10,0,
-8,5,-10,0,
-8,5,-8,0,
-7,7,-8,0,
-7,7,-10,0,
-7,7,-10,0,
-7,7,-8,0,
-5,8,-8,0,
-5,8,-10,0,
-5,8,-10,0,
-5,8,-8,0,
-3,9,-8,0,
-3,9,-10,0,
3,9,-8,0,
1,9,-8,0,
0,10,-8,0,
-1,9,-8,0,
-3,9,-8,0,
-5,8,-8,0,
-7,7,-8,0,
-8,5,-8,0,
-9,3,-8,0,
-9,1,-8,0,
-10,0,-8,0,
-9,-1,-8,0,
-9,-3,-8,0,
-8,-5,-8,0,
-7,-7,-8,0,
-5,-8,-8,0,
-3,-9,-8,0,
-1,-9,-8,0,
0,-10,-8,0,
1,-9,-8,0,
3,-9,-8,0,
5,-8,-8,0,
7,-7,-8,0,
8,-5,-8,0,
9,-3,-8,0,
9,-1,-8,0,
10,0,-8,0,
9,1,-8,0,
9,3,-8,0,
8,5,-8,0,
7,7,-8,0,
5,8,-8,0,
-3,9,-10,0,
-3,9,-8,0,
-1,9,-8,0,
-1,9,-10,0,
-1,9,-10,0,
-1,9,-8,0,
0,10,-8,0,
0,10,-10,0,
-1,9,-10,0,
0,10,-10,0,
1,9,-10,0,
3,9,-10,0,
5,8,-10,0,
7,7,-10,0,
8,5,-10,0,
9,3,-10,0,
9,1,-10,0,
10,0,-10,0,
9,-1,-10,0,
9,-3,-10,0,
8,-5,-10,0,
7,-7,-10,0,
5,-8,-10,0,
3,-9,-10,0,
1,-9,-10,0,
0,-10,-10,0,
-1,-9,-10,0,
-3,-9,-10,0,
-5,-8,-10,0,
-7,-7,-10,0,
-8,-5,-10,0,
-9,-3,-10,0,
-9,-1,-10,0,
-10,0,-10,0,
-9,1,-10,0,
-9,3,-10,0,
-8,5,-10,0,
-7,7,-10,0,
-5,8,-10,0,
-3,9,-10,0};

static int model_faces[] = {0,1,2,
0,2,3,
4,5,6,
4,6,7,
8,9,10,
8,10,11,
12,13,14,
12,14,15,
16,17,18,
16,18,19,
20,21,22,
20,22,23,
24,25,26,
24,26,27,
28,29,30,
28,30,31,
32,33,34,
32,34,35,
36,37,38,
36,38,39,
40,41,42,
40,42,43,
44,45,46,
44,46,47,
48,49,50,
48,50,51,
52,53,54,
52,54,55,
56,57,58,
56,58,59,
60,61,62,
60,62,63,
64,65,66,
64,66,67,
68,69,70,
68,70,71,
72,73,74,
72,74,75,
76,77,78,
76,78,79,
80,81,82,
80,82,83,
84,85,86,
84,86,87,
88,89,90,
88,90,91,
92,93,94,
92,94,95,
96,97,98,
96,98,99,
100,101,102,
100,102,103,
104,105,106,
104,106,107,
108,109,110,
108,110,111,
112,113,114,
112,114,115,
116,117,118,
116,118,119,
120,121,122,
122,123,124,
124,125,126,
126,127,128,
128,129,130,
130,131,132,
132,133,134,
134,135,136,
136,137,138,
138,139,140,
140,141,142,
142,143,144,
144,145,146,
146,147,148,
148,149,150,
150,151,120,
120,122,124,
124,126,128,
128,130,132,
132,134,136,
136,138,140,
140,142,144,
144,146,148,
148,150,120,
120,124,128,
128,132,136,
136,140,144,
144,148,120,
120,128,136,
136,144,120,
152,153,154,
152,154,155,
156,157,158,
156,158,159,
160,161,162,
162,163,164,
164,165,166,
166,167,168,
168,169,170,
170,171,172,
172,173,174,
174,175,176,
176,177,178,
178,179,180,
180,181,182,
182,183,184,
184,185,186,
186,187,188,
188,189,190,
190,191,160,
160,162,164,
164,166,168,
168,170,172,
172,174,176,
176,178,180,
180,182,184,
184,186,188,
188,190,160,
160,164,168,
168,172,176,
176,180,184,
184,188,160,
160,168,176,
176,184,160};

static SVECTOR model_normals[] = {
0,9,0,0,
0,9,0,0,
0,9,0,0,
0,9,0,0,
2,9,0,0,
2,9,0,0,
2,9,0,0,
2,9,0,0,
4,8,0,0,
4,8,0,0,
4,8,0,0,
4,8,0,0,
6,7,0,0,
6,7,0,0,
6,7,0,0,
6,7,0,0,
7,6,0,0,
7,6,0,0,
7,6,0,0,
7,6,0,0,
8,4,0,0,
8,4,0,0,
8,4,0,0,
8,4,0,0,
9,2,0,0,
9,2,0,0,
9,2,0,0,
9,2,0,0,
9,0,0,0,
9,0,0,0,
9,0,0,0,
9,0,0,0,
9,0,0,0,
9,0,0,0,
9,0,0,0,
9,0,0,0,
9,-2,0,0,
9,-2,0,0,
9,-2,0,0,
9,-2,0,0,
8,-4,0,0,
8,-4,0,0,
8,-4,0,0,
8,-4,0,0,
7,-6,0,0,
7,-6,0,0,
7,-6,0,0,
7,-6,0,0,
6,-7,0,0,
6,-7,0,0,
6,-7,0,0,
6,-7,0,0,
4,-8,0,0,
4,-8,0,0,
4,-8,0,0,
4,-8,0,0,
2,-9,0,0,
2,-9,0,0,
2,-9,0,0,
2,-9,0,0,
0,-9,0,0,
0,-9,0,0,
0,-9,0,0,
0,-9,0,0,
0,-9,0,0,
0,-9,0,0,
0,-9,0,0,
0,-9,0,0,
-2,-9,0,0,
-2,-9,0,0,
-2,-9,0,0,
-2,-9,0,0,
-4,-8,0,0,
-4,-8,0,0,
-4,-8,0,0,
-4,-8,0,0,
-6,-7,0,0,
-6,-7,0,0,
-6,-7,0,0,
-6,-7,0,0,
-7,-6,0,0,
-7,-6,0,0,
-7,-6,0,0,
-7,-6,0,0,
-8,-4,0,0,
-8,-4,0,0,
-8,-4,0,0,
-8,-4,0,0,
-9,-2,0,0,
-9,-2,0,0,
-9,-2,0,0,
-9,-2,0,0,
-9,0,0,0,
-9,0,0,0,
-9,0,0,0,
-9,0,0,0,
-9,0,0,0,
-9,0,0,0,
-9,0,0,0,
-9,0,0,0,
-9,2,0,0,
-9,2,0,0,
-9,2,0,0,
-9,2,0,0,
-8,4,0,0,
-8,4,0,0,
-8,4,0,0,
-8,4,0,0,
-7,6,0,0,
-7,6,0,0,
-7,6,0,0,
-7,6,0,0,
-6,7,0,0,
-6,7,0,0,
-6,7,0,0,
-6,7,0,0,
-4,8,0,0,
-4,8,0,0,
-4,8,0,0,
-4,8,0,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
0,0,10,0,
-2,9,0,0,
-2,9,0,0,
-2,9,0,0,
-2,9,0,0,
0,9,0,0,
0,9,0,0,
0,9,0,0,
0,9,0,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0,
0,0,-10,0};
