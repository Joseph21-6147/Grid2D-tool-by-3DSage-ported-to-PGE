// Grid2D designer tool - by 3DSage (https://www.youtube.com/3dsage)
// ================================
//
// Ported from C / OpenGL to C++ and olc::PixelGameEngine - by Javidx9
// Port by Joseph21
// February 25, 2024


#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"


#define res          1                     // 1=SW_BASExSH_BASE 2=2xSW_BASEx2xSH_BASE etc
#define SW_BASE      160
#define SH_BASE      120
#define SW           SW_BASE*res           // screen width
#define SH           SH_BASE*res           // screen height
#define SW2          (SW/2)                // half of screen width
#define SH2          (SH/2)                // half of screen height
#define pixelScale   4/res                 // OpenGL pixel scale
#define GLSW         (SW*pixelScale)       // OpenGL window width
#define GLSH         (SH*pixelScale)       // OpenGL window height

#define LEVEL_FILE   "level.txt"


// include all textures files
#include "T_NUMBERS.h"
#include "T_VIEW2D.h"
#include "T_00.h"
#include "T_01.h"
#include "T_02.h"
#include "T_03.h"
#include "T_04.h"
#include "T_05.h"
#include "T_06.h"
#include "T_07.h"
#include "T_08.h"
#include "T_09.h"
#include "T_10.h"
#include "T_11.h"
#include "T_12.h"
#include "T_13.h"
#include "T_14.h"
#include "T_15.h"
#include "T_16.h"
#include "T_17.h"
#include "T_18.h"
#include "T_19.h"
#include "T_20.h"

int numText = 20;                        // number of textures - 1
int numSect =  0;                        // number of sectors
int numWall =  0;                        // number of walls

//------------------------------------------------------------------------------

// aux. struct for timing
typedef struct {
    int fr1, fr2;          // frame 1, frame 2, to create constant frame rate (in seconds)
} myTime;
myTime T;                  // T is the global time struct

// lookup table for conversion of degrees to sine/cosine values
typedef struct {
    float cos[360];        // save sin and cos values for [0, 359] degrees
    float sin[360];
} math;
math M;                    // M is the global lookup table for cos and sin

// player info
typedef struct {
    int x, y, z;           // player position. Z is up
    int a;                 // player angle of rotation left right
    int l;                 // variable to look up and down
} player;
player P;                  // P is the global var for player info

// walls info
typedef struct {
    int x1, y1;            // bottom line point 1
    int x2, y2;            // bottom line point 2
    int wt, u, v;          // wall texture and u/v tile
    int shade;             // shade of the wall
} walls;
walls W[256];              // a max of 256 walls are supported

// sectors info
typedef struct {
    int ws, we;            // wall number start and end
    int z1, z2;            // height of bottom and top
    int d;                 // add y distances to sort drawing order
    int st, ss;            // surface texture, surface scale
    int surf[SW];          // to hold points for surfaces
} sectors;
sectors S[128];            // a max of 128 sectors are supported

// texture map
typedef struct {
    int w, h;              // texture width/height
    const int *name = nullptr;   // texture name (could be better: "data", since it points at the data array
} TexureMaps;
TexureMaps Textures[64];   //increase for more textures

typedef struct {
    int mx, my;            // rounded mouse position
    int addSect;           // 0=nothing, 1=add sector
    int wt, wu, wv;        // wall    texture, uv texture tile
    int st, ss;            // surface texture, surface scale
    int z1, z2;            // bottom and top height
    int scale;             // scale down grid
    int move[4];           // 0=wall ID, 1=v1v2, 2=wallID, 3=v1v2
    int selS, selW;        // select sector/wall
} grid;
grid G;                    // this struct holds all the data for controlling the editor


//------------------------------------------------------------------------------

class Grid2D_port : public olc::PixelGameEngine {

public:
    Grid2D_port() {
        sAppName = "3DSage's Grid2D (by Joseph21)";
        sAppName.append( " - S:(" + std::to_string( SW         ) + ", " + std::to_string( SH         ) + ")" );
        sAppName.append(  ", P:(" + std::to_string( pixelScale ) + ", " + std::to_string( pixelScale ) + ")" );
    }

private:
    // put your class variables here

    // save current content for sectors, walls and player to file
    void save() {
        std::ofstream fp( LEVEL_FILE );
        if(!fp.is_open()) {
            std::cout << "ERROR: save() --> error opening file: " << LEVEL_FILE << std::endl;
        } else {
            if(numSect > 0) {
                // save sector data
                fp << numSect << std::endl;
                for (int s = 0; s < numSect; s++) {
                    fp << S[s].ws << " " << S[s].we << " " << S[s].z1 << " " << S[s].z2 << " " << S[s].st << " " << S[s].ss << std::endl;
                }
                // wall data
                fp << numWall << std::endl;
                for (int w = 0; w < numWall; w++) {
                    fp << W[w].x1 << " " << W[w].y1 << " " << W[w].x2 << " " << W[w].y2 << " " << W[w].wt << " " << W[w].u << " " << W[w].v << " " << W[w].shade << std::endl;
                }
                // player position
                fp << std::endl << P.x << " " << P.y << " " << P.z << " " << P.a << " " << P.l << std::endl;

            }
            fp.close();
        }
    }

    // load file data into S (sector data), W (wall data) and P (player data)
    void load() {
        std::ifstream fp( LEVEL_FILE );
        if(!fp.is_open()) {
            std::cout << "ERROR: load() --> Error opening file: " << LEVEL_FILE << std::endl;
            return;
        } else {
            fp >> numSect;
            for(int s = 0; s < numSect; s++) { //load all sectors
                fp >> S[s].ws >> S[s].we >> S[s].z1 >> S[s].z2 >> S[s].st >> S[s].ss;
            }
            fp >> numWall;
            for(int w = 0; w < numWall; w++) { //load all walls
                fp >> W[w].x1 >> W[w].y1 >> W[w].x2 >> W[w].y2 >> W[w].wt >> W[w].u >> W[w].v >> W[w].shade;
            }
            fp >> P.x >> P.y >> P.z >> P.a >> P.l;
            fp.close();
        }
    }

    void initGlobals() {       // define grid globals

        G.scale =  4;      // scale down grid
        G.selS  =  0;      // select sector
        G.selW  =  0;      // select wall
        G.z1    =  0;
        G.z2    = 40;      // sector bottom top height
        G.st    =  1;
        G.ss    =  4;      // sector texture, scale
        G.wt    =  0;
        G.wu    =  1;
        G.wv    =  1;      // wall texture, u,v
    }

    // draw a pixel at (x, y) with rgb
    void drawPixel( int x, int y, int r, int g, int b ) {
        // since origin is at lower left corner of screen (in stead of upper left), we must
        // subtract y from lower screen boundary
        Draw( x, SH - 1 - y, olc::Pixel( r, g, b ));
    }

    void drawLine( float x1, float y1, float x2, float y2, int r, int g, int b ) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float fLarge = std::max( fabs( dx ), fabs( dy ));
        dx /= fLarge;
        dy /= fLarge;
        for(int n = 0; n < fLarge; n++) {
            drawPixel( x1, y1, r, g, b );
            x1 += dx;
            y1 += dy;
        }
    }

    // number patches are 12 pixels wide and 5 pixels high
    void drawNumber( int nx, int ny, int n ) {
        int nCharW = 12;
        int nCharH =  5;
        for(int y = 0; y < nCharH; y++) {
            int y2 = ((nCharH - 1 - y) + nCharH * n) * 3 * nCharW;
            for(int x = 0; x < nCharW; x++) {
                int x2 = x * 3;
                if(T_NUMBERS[y2 + x2] == 0) {
                    continue;
                }
                drawPixel( x + nx, y + ny, 255, 255, 255 );
            }
        }
    }

    void draw2D() {
        int c;
        // draw background using the T_VIEW2D sprite
        for(int y = 0; y < SH_BASE; y++) {
            int y2 = (SH - 1 - y) * 3 * SW_BASE; //invert height, x3 for rgb, x15 for texture width
            for(int x = 0; x < SW_BASE; x++) {
                int pixel = x * 3 + y2;
                int r = T_VIEW2D[pixel + 0];
                int g = T_VIEW2D[pixel + 1];
                int b = T_VIEW2D[pixel + 2];
                // what are the magic numbers below?
                if(G.addSect > 0 && y > 48 - 8 && y < 56 - 8 && x > 144) {
                    r = r >> 1;    //darken sector button (by halfing rgb values )
                    g = g >> 1;
                    b = b >> 1;
                }
                drawPixel( x, y, r, g, b );
            }
        }
        // draw sectors
        for (int s = 0; s < numSect; s++) {              // iterate all sectors

            for (int w = S[s].ws; w < S[s].we; w++) {    // iterate all walls for one sector

                if (s == G.selS - 1) {                   // if this sector is selected
                    // set sector to globals
                    S[G.selS - 1].z1 = G.z1;
                    S[G.selS - 1].z2 = G.z2;
                    S[G.selS - 1].st = G.st;
                    S[G.selS - 1].ss = G.ss;
                    // yellow select
                    if(G.selW == 0) {
                        c = 80;                          // all walls yellow
                    } else if(G.selW + S[s].ws - 1 == w) {
                        c = 80;                          //one wall selected
                        W[w].wt = G.wt;
                        W[w].u  = G.wu;
                        W[w].v  = G.wv;
                    } else {
                        c = 0;                           //grey walls
                    }
                } else {
                    c = 0;                               //sector not selected, grey
                }

                // draw the wall by drawing a line and drawing the (highlighted) end points
                drawLine(  W[w].x1 / G.scale, W[w].y1 / G.scale, W[w].x2 / G.scale, W[w].y2 / G.scale, 128 + c, 128 + c, 128 - c );
                drawPixel( W[w].x1 / G.scale, W[w].y1 / G.scale,                                       255    , 255    , 255     );
                drawPixel(                                       W[w].x2 / G.scale, W[w].y2 / G.scale, 255    , 255    , 255     );
            }
        }

        // draw player
        int dx = M.sin[P.a] * 12;
        int dy = M.cos[P.a] * 12;
        drawPixel(  P.x       / G.scale,  P.y       / G.scale, 0, 255, 0 );
        drawPixel( (P.x + dx) / G.scale, (P.y + dy) / G.scale, 0, 175, 0 );

        // draw wall texture
        float tx = 0, tx_stp = Textures[G.wt].w / 15.0;
        float ty = 0, ty_stp = Textures[G.wt].h / 15.0;
        for(int y = 0; y < 15; y++) {
            tx = 0;
            for(int x = 0; x < 15; x++) {
                int x2 = (int)tx % Textures[G.wt].w;
                tx += tx_stp;
                int y2 = (int)ty % Textures[G.wt].h;
                int r = Textures[G.wt].name[(Textures[G.wt].h - y2 - 1) * 3 * Textures[G.wt].w + x2 * 3 + 0];
                int g = Textures[G.wt].name[(Textures[G.wt].h - y2 - 1) * 3 * Textures[G.wt].w + x2 * 3 + 1];
                int b = Textures[G.wt].name[(Textures[G.wt].h - y2 - 1) * 3 * Textures[G.wt].w + x2 * 3 + 2];
                drawPixel( x + 145, y + 105 - 8, r, g, b );
            }
            ty += ty_stp;
        }
        // draw surface texture
        tx = 0, tx_stp = Textures[G.st].w / 15.0;
        ty = 0, ty_stp = Textures[G.st].h / 15.0;
        for(int y = 0; y < 15; y++) {
            tx = 0;
            for(int x = 0; x < 15; x++) {
                int x2 = (int)tx % Textures[G.st].w;
                tx += tx_stp;
                int y2 = (int)ty % Textures[G.st].h;
                int r = Textures[G.st].name[(Textures[G.st].h - y2 - 1) * 3 * Textures[G.st].w + x2 * 3 + 0];
                int g = Textures[G.st].name[(Textures[G.st].h - y2 - 1) * 3 * Textures[G.st].w + x2 * 3 + 1];
                int b = Textures[G.st].name[(Textures[G.st].h - y2 - 1) * 3 * Textures[G.st].w + x2 * 3 + 2];
                drawPixel( x + 145, y + 105 - 24 - 8, r, g, b );
            }
            ty += ty_stp; //*G.ss;
        }
        //draw numbers
        drawNumber( 140, 90, G.wu   ); // wall u
        drawNumber( 148, 90, G.wv   ); // wall v
        drawNumber( 148, 66, G.ss   ); // surface v
        drawNumber( 148, 58, G.z2   ); // top height
        drawNumber( 148, 50, G.z1   ); // bottom height
        drawNumber( 148, 26, G.selS ); // sector number
        drawNumber( 148, 18, G.selW ); // wall number
    }

    //darken buttons
    int dark = 0;

    void darken() {
        int xs, xe, ys, ye;
        switch (dark) {
            case  0:                                                             return;   //no buttons were clicked
            case  1: xs =   0; xe =  15; ys =   0 / G.scale; ye =  32 / G.scale; break;    // save button
            case  2: xs =   0; xe =   3; ys =  96 / G.scale; ye = 128 / G.scale; break;    // u left
            case  3: xs =   4; xe =   8; ys =  96 / G.scale; ye = 128 / G.scale; break;    // u right
            case  4: xs =   7; xe =  11; ys =  96 / G.scale; ye = 128 / G.scale; break;    // v left
            case  5: xs =  11; xe =  15; ys =  96 / G.scale; ye = 128 / G.scale; break;    // u right
            case  6: xs =   0; xe =   8; ys = 192 / G.scale; ye = 224 / G.scale; break;    // u left
            case  7: xs =   8; xe =  15; ys = 192 / G.scale; ye = 224 / G.scale; break;    // u right
            case  8: xs =   0; xe =   7; ys = 224 / G.scale; ye = 256 / G.scale; break;    // Top left
            case  9: xs =   7; xe =  15; ys = 224 / G.scale; ye = 256 / G.scale; break;    // Top right
            case 10: xs =   0; xe =   7; ys = 256 / G.scale; ye = 288 / G.scale; break;    // Bot left
            case 11: xs =   7; xe =  15; ys = 256 / G.scale; ye = 288 / G.scale; break;    // Bot right
            case 12: xs =   0; xe =   7; ys = 352 / G.scale; ye = 386 / G.scale; break;    // sector left
            case 13: xs =   7; xe =  15; ys = 352 / G.scale; ye = 386 / G.scale; break;    // sector right
            case 14: xs =   0; xe =   7; ys = 386 / G.scale; ye = 416 / G.scale; break;    // wall left
            case 15: xs =   7; xe =  15; ys = 386 / G.scale; ye = 416 / G.scale; break;    // wall right
            case 16: xs =   0; xe =  15; ys = 416 / G.scale; ye = 448 / G.scale; break;    // delete
            case 17: xs =   0; xe =  15; ys = 448 / G.scale; ye = 480 / G.scale; break;    // load
        }

        for(int y = ys; y < ye; y++) {
            for(int x = xs; x < xe; x++) {
//                glColor4f(0, 0, 0, 0.4);
//                glBegin(GL_POINTS);
//                glVertex2i(x * pixelScale + 2 + 580, (SH_BASE - y)*pixelScale);
//                glEnd();

                // I think what's happening here ^^ is that the pixels in the nested for loop range
                // are darkened by multiplying them with 0.4f. So I mimick this effect
                int nPixX = x * pixelScale + 2 + 580;
                int nPixY = (SH_BASE - y) * pixelScale;
                olc::Sprite *screenPtr = GetDrawTarget();
                olc::Pixel curPix = screenPtr->GetPixel( nPixX, nPixY );
                curPix *= 0.4f;
                screenPtr->SetPixel( nPixX, nPixY, curPix );
            }
        }
    }

    void mouse( int x, int y ) {

        //round mouse x,y
        G.mx = ((     x / pixelScale + 4) >> 3) << 3;
        G.my = ((SH - y / pixelScale + 4) >> 3) << 3;   // round to nearest 8th

        // ee = exclusive-exclusive, so exclusive comparison on both boundaries
        auto in_range_ee = [=]( int a, int a_low, int a_hgh ) {
            return (a_low < a && a < a_hgh);
        };

        if (GetMouse( 0 ).bPressed) {
            // 2D view buttons only
            if(x > 580) {
                //2d 3d view buttons
                if (in_range_ee( y, 0, 32 )) {
                    save();
                    dark = 1;
                }
                //wall texture
                if (in_range_ee( y, 32, 96 )) {
                    if (x < 610) { G.wt -= 1; if (G.wt <       0) { G.wt = numText; } }
                    else         { G.wt += 1; if (G.wt > numText) { G.wt =       0; } }
                }
                //wall uv
                if (in_range_ee( y, 96, 128 )) {
                    if (x < 595) { dark = 2; G.wu -= 1; if (G.wu < 1) { G.wu = 1; } } else
                    if (x < 610) { dark = 3; G.wu += 1; if (G.wu > 9) { G.wu = 9; } } else
                    if (x < 625) { dark = 4; G.wv -= 1; if (G.wv < 1) { G.wv = 1; } } else
                    if (x < 640) { dark = 5; G.wv += 1; if (G.wv > 9) { G.wv = 9; } }
                }
                //surface texture
                if (in_range_ee( y, 128, 192 )) {
                    if (x < 610) { G.st -= 1; if (G.st <       0) { G.st = numText; } }
                    else         { G.st += 1; if (G.st > numText) { G.st =       0; } }
                }
                //surface uv
                if (in_range_ee( y, 192, 222 )) {
                    if (x < 610) { dark = 6; G.ss -= 1; if (G.ss < 1) { G.ss = 1; } }
                    else         { dark = 7; G.ss += 1; if (G.ss > 9) { G.ss = 9; } }
                }
                //top height
                if (in_range_ee( y, 222, 256 )) {
                    if (x < 610) { dark = 8; G.z2 -= 5; if (G.z2 == G.z1) { G.z1 -= 5; } }
                    else         { dark = 9; G.z2 += 5;                                  }
                }
                //bot height
                if (in_range_ee( y, 256, 288 )) {
                    if (x < 610) { dark = 10; G.z1 -= 5;                                  }
                    else         { dark = 11; G.z1 += 5; if (G.z1 == G.z2) { G.z2 += 5; } }
                }
                //add sector
                if (in_range_ee( y, 288, 318 )) {
                    G.addSect += 1;
                    G.selS = 0;
                    G.selW = 0;
                    if(G.addSect > 1) {
                        G.addSect = 0;
                    }
                }
                //limit
                std::clamp( G.z1, 0, 90 );
                std::clamp( G.z2, 5, 95 );

                //select sector
                if (in_range_ee( y, 352, 386 )) {
                    G.selW = 0;
                    if (x < 610) { dark = 12; G.selS -= 1; if (G.selS <       0) { G.selS = numSect; } }
                    else         { dark = 13; G.selS += 1; if (G.selS > numSect) { G.selS =       0; } }
                    int s = G.selS - 1;
                    G.z1 = S[s].z1;         // sector bottom height
                    G.z2 = S[s].z2;         // sector top    height
                    G.st = S[s].st;         // surface texture
                    G.ss = S[s].ss;         // surface scale
                    G.wt = W[S[s].ws].wt;
                    G.wu = W[S[s].ws].u;
                    G.wv = W[S[s].ws].v;
                    if(G.selS == 0) {
                        initGlobals();      // defaults
                    }
                }
                // select sector's walls
                int snw = S[G.selS - 1].we - S[G.selS - 1].ws; // sector's number of walls

                if (in_range_ee( y, 386, 416 )) {
                    if (x < 610) { dark = 14; G.selW -= 1; if (G.selW <   0) { G.selW = snw; } }   // select sector wall left
                    else         { dark = 15; G.selW += 1; if (G.selW > snw) { G.selW =   0; } }   // select sector wall right
                    if(G.selW > 0) {
                        G.wt = W[S[G.selS - 1].ws + G.selW - 1].wt;
                        G.wu = W[S[G.selS - 1].ws + G.selW - 1].u;
                        G.wv = W[S[G.selS - 1].ws + G.selW - 1].v;
                    }
                }
                //delete
                if (in_range_ee( y, 416, 448 )) {
                    dark = 16;
                    if (G.selS > 0) {
                        int d = G.selS - 1;                         // delete this one
                        numWall -= (S[d].we - S[d].ws);             // first subtract number of walls
                        for (x = d; x < numSect; x++) {
                            S[x] = S[x + 1];                        // remove from array
                        }
                        numSect -= 1;                               // 1 less sector
                        G.selS = 0;
                        G.selW = 0;                                 // deselect
                    }
                }

                //load
                if (in_range_ee( y, 448, 480 )) {
                    dark = 17;
                    load();
                }

            } else {
                //clicked on grid

                //init new sector
                if(G.addSect == 1) {
                    S[numSect].ws = numWall;                         // clear wall start
                    S[numSect].we = numWall + 1;                     // add 1 to wall end
                    S[numSect].z1 = G.z1;
                    S[numSect].z2 = G.z2;
                    S[numSect].st = G.st;
                    S[numSect].ss = G.ss;
                    W[numWall].x1 = G.mx * G.scale;
                    W[numWall].y1 = G.my * G.scale;
                    W[numWall].x2 = G.mx * G.scale;
                    W[numWall].y2 = G.my * G.scale;
                    W[numWall].wt = G.wt;
                    W[numWall].u  = G.wu;
                    W[numWall].v  = G.wv;
                    numWall += 1;                                    // add 1 wall
                    numSect += 1;                                    // add this sector
                    G.addSect = 3;                                   // go to point 2
                }

                //add point 2
                else if (G.addSect == 3) {
                    if (S[numSect - 1].ws == numWall - 1 && G.mx * G.scale <= W[S[numSect - 1].ws].x1) {
                        numWall -= 1;
                        numSect -= 1;
                        G.addSect = 0;
                        std::cout << "walls must be counter clockwise" << std::endl;
                        return;
                    }

                    //point 2
                    W[numWall - 1].x2 = G.mx * G.scale;
                    W[numWall - 1].y2 = G.my * G.scale; //x2,y2
                    //automatic shading
                    float ang = atan2f( W[numWall - 1].y2 - W[numWall - 1].y1, W[numWall - 1].x2 - W[numWall - 1].x1 );
                    ang = (ang * 180) / 3.1415926535f; //radians to degrees
                    if(ang < 0) {
                        ang += 360;   //correct negative
                    }
                    int shade = ang;         //shading goes from 0-90-0-90-0
                    if(shade > 180) {
                        shade = 180 - (shade - 180);
                    }
                    if(shade > 90) {
                        shade = 90 - (shade - 90);
                    }
                    W[numWall - 1].shade = shade;

                    // check if sector is closed
                    if(W[numWall - 1].x2 == W[S[numSect - 1].ws].x1 && W[numWall - 1].y2 == W[S[numSect - 1].ws].y1) {
                        W[numWall - 1].wt = G.wt;
                        W[numWall - 1].u = G.wu;
                        W[numWall - 1].v = G.wv;
                        G.addSect = 0;
                    }
                    // not closed, add new wall
                    else {
                        // init next wall
                        S[numSect - 1].we += 1;                                  // add 1 to wall end
                        W[numWall].x1 = G.mx * G.scale;
                        W[numWall].y1 = G.my * G.scale;
                        W[numWall].x2 = G.mx * G.scale;
                        W[numWall].y2 = G.my * G.scale;
                        W[numWall - 1].wt = G.wt;
                        W[numWall - 1].u = G.wu;
                        W[numWall - 1].v = G.wv;
                        W[numWall].shade = 0;
                        numWall += 1;                                            // add 1 wall
                    }
                }
            }
        }

        //clear variables to move point
        for (int w = 0; w < 4; w++) {
            G.move[w] = -1;
        }

        if(G.addSect == 0 && GetMouse( 1 ).bHeld) {
            //move point hold id
            for(int s = 0; s < numSect; s++) {
                for(int w = S[s].ws; w < S[s].we; w++) {
                    int x1 = W[w].x1, y1 = W[w].y1;
                    int x2 = W[w].x2, y2 = W[w].y2;
                    int mx = G.mx * G.scale, my = G.my * G.scale;
                    if (mx < x1 + 3 && mx > x1 - 3 && my < y1 + 3 && my > y1 - 3) {
                        G.move[0] = w;
                        G.move[1] = 1;
                    }
                    if (mx < x2 + 3 && mx > x2 - 3 && my < y2 + 3 && my > y2 - 3) {
                        G.move[2] = w;
                        G.move[3] = 2;
                    }
                }
            }
        }

        if(GetMouse( 0 ).bReleased) {
            dark = 0;
        }
    }

    void mouseMoving( int x, int y ) {
        if(x < 580 && G.addSect == 0 && G.move[0] > -1) {
            int Aw = G.move[0], Ax = G.move[1];
            int Bw = G.move[2], Bx = G.move[3];
            if(Ax == 1) {
                W[Aw].x1 = ((x + 16) >> 5) << 5;
                W[Aw].y1 = ((GLSH - y + 16) >> 5) << 5;
            }
            if(Ax == 2) {
                W[Aw].x2 = ((x + 16) >> 5) << 5;
                W[Aw].y2 = ((GLSH - y + 16) >> 5) << 5;
            }
            if(Bx == 1) {
                W[Bw].x1 = ((x + 16) >> 5) << 5;
                W[Bw].y1 = ((GLSH - y + 16) >> 5) << 5;
            }
            if(Bx == 2) {
                W[Bw].x2 = ((x + 16) >> 5) << 5;
                W[Bw].y2 = ((GLSH - y + 16) >> 5) << 5;
            }
        }
    }

    // adapt player variables (lookup value l, position in the world x, y or z value or orientation a)
    void movePlayer() {

        int dx = M.sin[P.a] * 10.0f;
        int dy = M.cos[P.a] * 10.0f;
        if (GetKey( olc::Key::M ).bHeld) {
            // move up, down, look up, look  down
            if (GetKey( olc::Key::A ).bPressed) { P.l += 1; }
            if (GetKey( olc::Key::D ).bPressed) { P.l -= 1; }
            if (GetKey( olc::Key::W ).bPressed) { P.z += 4; }
            if (GetKey( olc::Key::S ).bPressed) { P.z -= 4; }
        } else {
            // move forward, back, rotate left, right
            if (GetKey( olc::Key::A ).bPressed) { P.a -= 4; if (P.a <   0) { P.a += 360; } }
            if (GetKey( olc::Key::D ).bPressed) { P.a += 4; if (P.a > 359) { P.a -= 360; } }
            if (GetKey( olc::Key::W ).bPressed) { P.x += dx; P.y += dy; }
            if (GetKey( olc::Key::S ).bPressed) { P.x -= dx; P.y -= dy; }
        }
        // strafe left, right
//        if (GetKey( olc::Key::LEFT  ).bPressed) { P.x += dy; P.y += dx; }
        if (GetKey( olc::Key::LEFT  ).bPressed) { P.x -= dy; P.y += dx; }
//        if (GetKey( olc::Key::RIGHT ).bPressed) { P.x -= dy; P.y -= dx; }
        if (GetKey( olc::Key::RIGHT ).bPressed) { P.x += dy; P.y -= dx; }
    }

    void display( float fElapsedTime ) {
        float fDisplayThreshold = 0.01f;
        T.fr1 += fElapsedTime;
        if(T.fr1 - T.fr2 >= fDisplayThreshold) {                  //only draw 20 frames/second
            draw2D();
            darken();

            T.fr2 -= fDisplayThreshold;
        }
    }

    void init() {
        initGlobals();

        T.fr1 = T.fr2 = 0.0f;

        //init player
        P.x = 32 * 9;
        P.y = 48;
        P.z = 30;
        P.a = 0;
        P.l = 0;  //init player variables

        //store sin/cos in degrees
        for(int x = 0; x < 360; x++) {                 //precalulate sin cos in degrees
            M.cos[x] = cos( x / 180.0 * 3.1415926535f );
            M.sin[x] = sin( x / 180.0 * 3.1415926535f );
        }

        //define textures
        Textures[ 0].name = T_00; Textures[ 0].h = T_00_HEIGHT; Textures[ 0].w = T_00_WIDTH;
        Textures[ 1].name = T_01; Textures[ 1].h = T_01_HEIGHT; Textures[ 1].w = T_01_WIDTH;
        Textures[ 2].name = T_02; Textures[ 2].h = T_02_HEIGHT; Textures[ 2].w = T_02_WIDTH;
        Textures[ 3].name = T_03; Textures[ 3].h = T_03_HEIGHT; Textures[ 3].w = T_03_WIDTH;
        Textures[ 4].name = T_04; Textures[ 4].h = T_04_HEIGHT; Textures[ 4].w = T_04_WIDTH;
        Textures[ 5].name = T_05; Textures[ 5].h = T_05_HEIGHT; Textures[ 5].w = T_05_WIDTH;
        Textures[ 6].name = T_06; Textures[ 6].h = T_06_HEIGHT; Textures[ 6].w = T_06_WIDTH;
        Textures[ 7].name = T_07; Textures[ 7].h = T_07_HEIGHT; Textures[ 7].w = T_07_WIDTH;
        Textures[ 8].name = T_08; Textures[ 8].h = T_08_HEIGHT; Textures[ 8].w = T_08_WIDTH;
        Textures[ 9].name = T_09; Textures[ 9].h = T_09_HEIGHT; Textures[ 9].w = T_09_WIDTH;
        Textures[10].name = T_10; Textures[10].h = T_10_HEIGHT; Textures[10].w = T_10_WIDTH;
        Textures[11].name = T_11; Textures[11].h = T_11_HEIGHT; Textures[11].w = T_11_WIDTH;
        Textures[12].name = T_12; Textures[12].h = T_12_HEIGHT; Textures[12].w = T_12_WIDTH;
        Textures[13].name = T_13; Textures[13].h = T_13_HEIGHT; Textures[13].w = T_13_WIDTH;
        Textures[14].name = T_14; Textures[14].h = T_14_HEIGHT; Textures[14].w = T_14_WIDTH;
        Textures[15].name = T_15; Textures[15].h = T_15_HEIGHT; Textures[15].w = T_15_WIDTH;
        Textures[16].name = T_16; Textures[16].h = T_16_HEIGHT; Textures[16].w = T_16_WIDTH;
        Textures[17].name = T_17; Textures[17].h = T_17_HEIGHT; Textures[17].w = T_17_WIDTH;
        Textures[18].name = T_18; Textures[18].h = T_18_HEIGHT; Textures[18].w = T_18_WIDTH;
        Textures[19].name = T_19; Textures[19].h = T_19_HEIGHT; Textures[19].w = T_19_WIDTH;
        Textures[20].name = T_20; Textures[20].h = T_20_HEIGHT; Textures[20].w = T_20_WIDTH;
    }

    int nOldMouseX, nOldMouseY;    // cache previous mouse position (to detect mouse movement)
    bool bInfoFlag = false;

public:

    bool OnUserCreate() override {

        // initialize your assets here
        init();
        load();

        nOldMouseX = GetMouseX();
        nOldMouseY = GetMouseY();

        return true;
    }

    float fTotalTime = 0.0f;
    float fCachedTime = 0.0f;;

    bool OnUserUpdate( float fElapsedTime ) override {

        fTotalTime += fElapsedTime;
        bool bTick = false;
        if (fTotalTime - fCachedTime > 1.0f) {
            bTick = true;
            fCachedTime += 1.0f;
        }

        if (bInfoFlag) {
            if (bTick) {
                std::cout << int( fTotalTime ) << std::endl;
            }
        }

        if (GetKey( olc::Key::I ).bPressed) {
            bInfoFlag = !bInfoFlag;
        }

        Clear( olc::DARK_GREEN );

        // grab mouse coordinates
        int nMouseX = GetMouseX();
        int nMouseY = GetMouseY();
        // the code appears to work with absolute coordinates, so multiply PGE coordinates by pixelScale
        int nUseMouseX = nMouseX * pixelScale;
        int nUseMouseY = nMouseY * pixelScale;
        // check if mouse moved since prev. frame
        if (nMouseX != nOldMouseX || nMouseY != nOldMouseY) {
            // if so, call mouseMoving()
            mouseMoving( nUseMouseX, nUseMouseY );
        }
        // reset previouis mouse coordinates
        nOldMouseX = nMouseX;
        nOldMouseY = nMouseY;
        // call mouse handler
        mouse( nUseMouseX, nUseMouseY );
        // call keyboard handler
        movePlayer();
        // display what you got
        display( fElapsedTime );

        if (bInfoFlag) {
            // display test info on the player and mouse pos
            DrawString( 2, SH - 40, "Plr: ("  + std::to_string( P.x ) +
                                    ", "      + std::to_string( P.y ) +
                                    ", "      + std::to_string( P.z ) +
                                    ")" );
            DrawString( 2, SH - 30, "ang: "   + std::to_string( P.a ) );
            DrawString( 2, SH - 20, "lkp: "   + std::to_string( P.l ) );
            DrawString( 2, SH - 10, "Mse: ("  + std::to_string( nUseMouseX ) +
                                    ", "      + std::to_string( nUseMouseY ) +
                                    ")" );
        }

        return true;
    }

    bool OnUserDestroy() override {

        // your clean up code here
        return true;
    }
};


int main()
{
	Grid2D_port demo;
	if (demo.Construct( SW, SH, pixelScale, pixelScale )) {
		demo.Start();
	} else {
        std::cout << "ERROR: main() --> Failure to construct window ..." << std::endl;
	}

	return 0;
}

