const int numVertices = 8;
const int numNormals = 6;
const int numPoly2s = 0;
const int numPoly3s = 0;
const int numPoly4s = 6;

// format: x,y,z,0, ... (0 for padding into a SVECTOR struct)

static SVECTOR vertices[] = { 
{10,10,-10,0},
{10,-10,-10,0},
{10,10,10,0},
{10,-10,10,0},
{-10,10,-10,0},
{-10,-10,-10,0},
{-10,10,10,0},
{-10,-10,10,0}};

// format: x,y,z,0, ... (0 for padding into a SVECTOR struct)

static SVECTOR normals[] = { 
{0,4096,0,0},
{0,0,4096,0},
{-4096,0,0,0},
{0,-4096,0,0},
{4096,0,0,0},
{0,0,-4096,0}};

static short poly3arr[] = {};
// format: vi(1),vi(2),vi(3),vi(4),ni(1),ni(2),ni(3),ni(4) ... (vi = vertex index, ni = normal index)

static short poly4arr[] = { 
0,4,6,2,0,0,0,0,
3,2,6,7,1,1,1,1,
7,6,4,5,2,2,2,2,
5,1,3,7,3,3,3,3,
1,0,2,3,4,4,4,4,
5,4,0,1,5,5,5,5};

