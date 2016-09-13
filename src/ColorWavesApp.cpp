// Briley Cantwell 9/12/2016
// ColorWaves is a 2D simulation where each pixel's red, green, and blue values behave like standing waves.
// Each pixel's color accelerates towards the average color of the surrounding pixels.
// The executable (.exe) is available in the "Release" folder

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"

#include "cinder/TriMesh.h"
#include "cinder/Camera.h"
#include "Resources.h"
#include <math.h>

using namespace ci;
using namespace ci::app;
using namespace std;


int squaresize = 10; // the number of pixels in each square's length
float kfactor = 0.4f; // the willingness of each pixel to change color

// return random integer, inclusive of low and high 
int randomInt(int low, int high)
{
    return low + (rand() % (high - low + 1));
}

// int to string
string toString(int in)
{
	return to_string(static_cast<long double>(in));
}

// float to string
string toString(float in)
{
	return to_string(static_cast<long double>(in));
}

// string to int
int toInt(string str)
{
	return atoi(str.c_str());
}

// string to float
float toFloat(string str)
{
	return atof(str.c_str());
}

// Particle represents one square
struct Particle
{
	Color color; // current color of the Particle
	Color vel; // current rate-of-change (velocity) of the color of the Particle
};

class ColorWavesApp : public AppNative {
  public:
	void prepareSettings( Settings* settings );
	void setup();
	void mouseDown( MouseEvent event );
	void keyDown( KeyEvent event );
	void update();
	void draw();
	void restart();

	Particle** map; // 2D array of all particles on the screen (map[x][y])
	int mapx; // width of screen in number of particles
	int mapy; // height of screen in number of particles

	bool keystates[30]; // stores true if key is down, false otherwise
	bool keypresses[30]; // stores true if key has just been hit, false otherwise

	int screenx; // width of screen in pixels
	int screeny; // height of screen in pixels

	int colormode;	// 1-parallel: particles update simultaneously.
					// 2-sequential: particles update from left-to-right, top-to-bottom.

	bool grayscale; // true if all particles appear in grayscale, false if in color.

	int colorToUse; // Color created by clicking the screen. 1-red, 2-orange, 3-yellow, 4-green, 5-blue, 6-violet, 7-random

	bool menuUp; // true if the menu bar is showing, false if hidden
	int menux[5]; // horizontal click positions for the menu bar
	int menuy[11]; // vertical click positions for the menu bar

	Font fo; // Times New Roman font
	Color black; // black

	gl::Texture tex[3]; // storing textures for particles and instructions/menu
	Area area4; // stored for convenient drawing
};

void ColorWavesApp::prepareSettings( Settings* settings )
{
	settings->setFullScreen(); // full screen
	settings->setFrameRate(60.0f); // capped at 60 frames per second
}

void ColorWavesApp::setup()
{
	srand(static_cast<unsigned int>(time(0))); // random seed set to start time

	screenx = this->getWindowWidth(); // width of screen in pixels
	screeny = this->getWindowHeight(); // height of screen in pixels

	gl::lineWidth(3.0f); // draws lines thick and clear

	mapx = screenx/squaresize; // width of screen in number of particles
	mapy = screeny/squaresize; // height of screen in number of particles

	map = new Particle*[mapx]; // creates 2D matrix of Particles with 'mapx' width and 'mapy' height
	for( int i = 0; i < mapx; i++ )
		map[i] = new Particle[mapy];

	for( int i = 0; i < mapx; i++ ) // initializes all Particles to white with no color velocity
	{
		for( int k = 0; k < mapy; k++ )
		{
			map[i][k].color.set(1.0f, 1.0f, 1.0f);
			map[i][k].vel.set(0.0f, 0.0f, 0.0f);
		}
	}

	menuUp = true; // menu is showing by default

	colorToUse = 7; // random color by default

	fo = Font("Times New Roman", 32); // initialize font
	black = Color( 0, 0, 0 ); // initialize font color

	for( int i = 0; i < 30; i++ )
	{	keystates[i] = false; keypresses[i] = false; } // all keys are initially up (not pressed)

	colormode = 1; // parallel by default
	grayscale = false; // draw in color by default

	area4 = Area(0,0,1,1); // 1x1 pixel stored for convenient drawing

	// loading textures:
	tex[0] = gl::Texture( loadImage( loadResource(part_, "IMAGE" ) ) ); // 1x1 white pixel
	tex[1] = gl::Texture( loadImage( loadResource(instr_, "IMAGE" ) ) ); // instructions / menu
	tex[2] = gl::Texture( loadImage( loadResource(qui_, "IMAGE" ) ) ); // "quit" button

	// set horizontal click positions for the menu bar:
	menux[0] = screenx-256+26.0f/512.0f*256;
	menux[1] = screenx-256+139.0f/512.0f*256;
	menux[2] = screenx-256+251.0f/512.0f*256;
	menux[3] = screenx-256+364.0f/512.0f*256;
	menux[4] = screenx-256+477.0f/512.0f*256;

	// set vertical click positions for the menu bar
	menuy[0] = 223.0f/1024.0f*512;
	menuy[1] = 322.0f/1024.0f*512;
	menuy[2] = 422.0f/1024.0f*512;
	menuy[3] = 455.0f/1024.0f*512;
	menuy[4] = 556.0f/1024.0f*512;
	menuy[5] = 618.0f/1024.0f*512;
	menuy[6] = 718.0f/1024.0f*512;
	menuy[7] = 737.0f/1024.0f*512;
	menuy[8] = 837.0f/1024.0f*512;
	menuy[9] = 854.0f/1024.0f*512;
	menuy[10] = 955.0f/1024.0f*512;
}

void ColorWavesApp::mouseDown( MouseEvent event )
{
	if ( menuUp && event.getX() > screenx-256 && event.getY() < 512 ) // if something in the menu bar was clicked
	{
		int x = event.getX();
		int y = event.getY();
		if ( y > menuy[0] && y < menuy[1] )
		{
			for( int i = 1; i < 5; i++ ) // clicked red, orange, yellow, or green drawing color
			{
				if ( x > menux[i-1] && x < menux[i] )
				{	colorToUse = i; return; }
			}
		}
		else if ( y > menuy[1] && y < menuy[2] )
		{
			if ( x > menux[2] )
			{
				if ( x < menux[4] ) // clicked random drawing color
					colorToUse = 7;
			}
			else if ( x > menux[1] ) // clicked violet drawing color
				colorToUse = 6;
			else if ( x > menux[0] ) // clicked blue drawing color
				colorToUse = 5;
		}

		// color or grayscale
		else if ( y > menuy[3] && y < menuy[4] )
		{
			if ( x > menux[0] && x < menux[2] ) // clicked color drawing
				grayscale = false;
			else if ( x > menux[2] && x < menux[4] ) // clicked grayscale drawing
				grayscale = true;
		}

		// colormode
		else if ( y > menuy[5] && y < menuy[6] )
		{
			if ( x > menux[0] && x < menux[2] ) // clicked parallel update mode
				colormode = 1;
			else if ( x > menux[2] && x < menux[4] ) // clicked sequential update mode
				colormode = 2;
		}

		// clear to white
		else if ( y > menuy[7] && y < menuy[8] && x > menux[0] && x < menux[4] )
			restart();

		// toggle menu bar showing
		else if ( y > menuy[9] && y < menuy[10] && x > menux[0] && x < menux[4] )
			menuUp = false;


		return;
	}
	if ( menuUp && event.getX() > screenx-128 && event.getY() > screeny-32 ) // clicked "quit" button
		quit();

	// if none of the above triggered, user clicked somewhere drawable: creating a wave
	int xx = event.getX()/squaresize;
	int yy = event.getY()/squaresize;

	Color randc;
	if ( colorToUse == 7 ) // random color
	{
		randc = Color( randomInt(0,100)/100.0f, randomInt(0,100)/100.0f, randomInt(0,100)/100.0f );
		if ( randomInt(0,2) == 1 ) // randomly zero out a color component. Makes colors more... colorful.
			randc.r = 0.0f;
		else if ( randomInt(0,1) == 1 )
			randc.g = 0.0f;
		else
			randc.b = 0.0f;
	}
	else if ( colorToUse == 1 ) // red
		randc = Color(1,0,0);
	else if ( colorToUse == 2 ) // orange
		randc = Color(1.0f,0.5f,0.0f);
	else if ( colorToUse == 3 ) // yellow
		randc = Color(1.0f,1.0f,0);
	else if ( colorToUse == 4 ) // green
		randc = Color(0,1,0);
	else if ( colorToUse == 5 ) // blue
		randc = Color(0,0,1);
	else if ( colorToUse == 6 ) // violet
		randc = Color(0.5f,0.0f,1.0f);

	// assign color value with zero velocity to clicked particle
	map[xx][yy].color = randc;
	map[xx][yy].vel.set(0.0f, 0.0f, 0.0f);

	// assign same values to surrounding particles as well, and ensure we do not try to assign out-of-bounds indices.
	if ( xx > 0 )
	{
		map[xx-1][yy].color = randc;
		map[xx-1][yy].vel.set(0.0f, 0.0f, 0.0f);
	}
	if ( xx < mapx-1 )
	{
		map[xx+1][yy].color = randc;
		map[xx+1][yy].vel.set(0.0f, 0.0f, 0.0f);
	}
	if ( yy > 0 )
	{
		map[xx][yy-1].color = randc;
		map[xx][yy-1].vel.set(0.0f, 0.0f, 0.0f);
	}
	if ( yy < mapy-1 )
	{
		map[xx][yy+1].color = randc;
		map[xx][yy+1].vel.set(0.0f, 0.0f, 0.0f);
	}
	if ( xx > 0 && yy > 0 )
	{
		map[xx-1][yy-1].color = randc;
		map[xx-1][yy-1].vel.set(0.0f, 0.0f, 0.0f);
	}
	if ( xx < mapx-1 && yy < mapy-1 )
	{
		map[xx+1][yy+1].color = randc;
		map[xx+1][yy+1].vel.set(0.0f, 0.0f, 0.0f);
	}
	if ( yy > 0 && xx < mapx-1 )
	{
		map[xx+1][yy-1].color = randc;
		map[xx+1][yy-1].vel.set(0.0f, 0.0f, 0.0f);
	}
	if ( yy < mapy-1 && xx > 0 )
	{
		map[xx-1][yy+1].color = randc;
		map[xx-1][yy+1].vel.set(0.0f, 0.0f, 0.0f);
	}

}

void ColorWavesApp::keyDown( KeyEvent event )
{
	if ( event.getCode() == event.KEY_1 )
	{
		colorToUse = 1; // red
	}
	if ( event.getCode() == event.KEY_2 )
	{
		colorToUse = 2; // orange
	}
	if ( event.getCode() == event.KEY_3 )
	{
		colorToUse = 3; // yellow
	}
	if ( event.getCode() == event.KEY_4 )
	{
		colorToUse = 4; // green
	}
	if ( event.getCode() == event.KEY_5 )
	{
		colorToUse = 5; // blue
	}
	if ( event.getCode() == event.KEY_6 )
	{
		colorToUse = 6; // violet
	}
	if ( event.getCode() == event.KEY_r )
	{
		colorToUse = 7; // random
	}
	if ( event.getCode() == event.KEY_ESCAPE )
	{
		quit();
	}
	if ( event.getCode() == event.KEY_w )
	{
		restart(); // clear to white
	}
	if ( event.getCode() == event.KEY_g )
	{
		grayscale = true;
	}
	if ( event.getCode() == event.KEY_c )
	{
		grayscale = false;
	}
	if ( event.getCode() == event.KEY_p )
	{
		colormode = 1; // parallel
	}
	if ( event.getCode() == event.KEY_s )
	{
		colormode = 2; // sequential
	}
	if ( event.getCode() == event.KEY_m )
	{
		menuUp = !menuUp; // toggle menu showing
	}
}

void ColorWavesApp::update()
{
	float avr, avg, avb;
	int num;
	for( int i = 0; i < mapx; i++ )
	{
		for( int k = 0; k < mapy; k++ )
		{
			// cycles through every Particle from left-to-right, top-to-bottom.
			avr = 0.0f;
			avg = 0.0f;
			avb = 0.0f;
			num = 0;

			// accumulate all adjacent Particles' color values to affect this Particle's color velocity
			if ( i > 0 )
			{
				avr += map[i-1][k].color.r - map[i][k].color.r;
				avg += map[i-1][k].color.g - map[i][k].color.g;
				avb += map[i-1][k].color.b - map[i][k].color.b;
				num++;
			}
			if ( i < mapx-1 )
			{
				avr += map[i+1][k].color.r - map[i][k].color.r;
				avg += map[i+1][k].color.g - map[i][k].color.g;
				avb += map[i+1][k].color.b - map[i][k].color.b;
				num++;
			}
			if ( k > 0 )
			{
				avr += map[i][k-1].color.r - map[i][k].color.r;
				avg += map[i][k-1].color.g - map[i][k].color.g;
				avb += map[i][k-1].color.b - map[i][k].color.b;
				num++;
			}
			if ( k < mapy-1 )
			{
				avr += map[i][k+1].color.r - map[i][k].color.r;
				avg += map[i][k+1].color.g - map[i][k].color.g;
				avb += map[i][k+1].color.b - map[i][k].color.b;
				num++;
			}

			// update this Particle's color velocity according to the average color of adjacent Particles:
			map[i][k].vel.operator+=( Color((avr/num)*kfactor, (avg/num)*kfactor, (avb/num)*kfactor ));

			if ( colormode == 2 ) // sequential color-update-mode: update a Particle's color as soon as its velocity is updated.
			{
				map[i][k].color += map[i][k].vel;
				// do not let color values exceed 1.0f or drop below 0.0f:
				if ( map[i][k].color.r > 1.0f )
				{
					map[i][k].color.r = 1.0f;
					map[i][k].vel.r = 0.0f;
				}
				if ( map[i][k].color.r < 0.0f )
				{
					map[i][k].color.r = 0.0f;
					map[i][k].vel.r = 0.0f;
				}
				if ( map[i][k].color.g > 1.0f )
				{
					map[i][k].color.g = 1.0f;
					map[i][k].vel.g = 0.0f;
				}
				if ( map[i][k].color.g < 0.0f )
				{
					map[i][k].color.g = 0.0f;
					map[i][k].vel.g = 0.0f;
				}
				if ( map[i][k].color.b > 1.0f )
				{
					map[i][k].color.b = 1.0f;
					map[i][k].vel.b = 0.0f;
				}
				if ( map[i][k].color.b < 0.0f )
				{
					map[i][k].color.b = 0.0f;
					map[i][k].vel.b = 0.0f;
				}
			}
		}
	}
	
	if ( colormode == 1 ) // parallel color-update-mode: only update a Particle's color after ALL Particle's velocities are updated.
	{
	for( int i = 0; i < mapx; i++ )
	{
		for( int k = 0; k < mapy; k++ )
		{
			map[i][k].color += map[i][k].vel;
			// do not let color values exceed 1.0f or drop below 0.0f:
			if ( map[i][k].color.r > 1.0f )
			{
				map[i][k].color.r = 1.0f;
				map[i][k].vel.r = 0.0f;
			}
			if ( map[i][k].color.r < 0.0f )
			{
				map[i][k].color.r = 0.0f;
				map[i][k].vel.r = 0.0f;
			}
			if ( map[i][k].color.g > 1.0f )
			{
				map[i][k].color.g = 1.0f;
				map[i][k].vel.g = 0.0f;
			}
			if ( map[i][k].color.g < 0.0f )
			{
				map[i][k].color.g = 0.0f;
				map[i][k].vel.g = 0.0f;
			}
			if ( map[i][k].color.b > 1.0f )
			{
				map[i][k].color.b = 1.0f;
				map[i][k].vel.b = 0.0f;
			}
			if ( map[i][k].color.b < 0.0f )
			{
				map[i][k].color.b = 0.0f;
				map[i][k].vel.b = 0.0f;
			}
		}

	}
	}


}

void ColorWavesApp::draw()
{
	gl::clear( Color( 1.0f, 1.0f, 1.0f ) ); // clear the screen to a white background.

	for( int i = 0; i < mapx; i++ )
	{
		for( int k = 0; k < mapy; k++ )
		{
			if ( grayscale )
			{
				// calculate average color component value and apply that value for all 3 components: red, green, and blue.
				float col = (map[i][k].color.r+map[i][k].color.g+map[i][k].color.b)/3.0f;
				gl::color( col, col, col );
			}
			else
				// not grayscale: draw color as is.
				gl::color( map[i][k].color );
			gl::draw( tex[0], area4, Rectf( i*squaresize, k*squaresize, (i+1)*squaresize, (k+1)*squaresize ) );
		}
	}

	if ( menuUp )
	{
		// draw the menu textures and selection highlights:
		gl::color(1,1,1);
		gl::draw( tex[1], Area(0,0,512,1024), Rectf(screenx-256,0,screenx,512));
		gl::draw( tex[2], Area(0,0,256,64), Rectf(screenx-128,screeny-32,screenx,screeny));
		gl::color(0.0f, 0.6f, 1.0f);
		if ( colorToUse < 5 ) // top row of colors
			gl::drawStrokedRect( Rectf(menux[colorToUse-1], menuy[0], menux[colorToUse], menuy[1]) );
		else if ( colorToUse == 7 ) // random color
			gl::drawStrokedRect( Rectf(menux[2], menuy[1], menux[4], menuy[2]) );
		else // bottom two colors
			gl::drawStrokedRect( Rectf(menux[colorToUse-5], menuy[1], menux[colorToUse-4], menuy[2]) );

		if ( !blackandwhite ) // color
			gl::drawStrokedRect( Rectf(menux[0], menuy[3], menux[2], menuy[4]) );
		else // grayscale
			gl::drawStrokedRect( Rectf(menux[2], menuy[3], menux[4], menuy[4]) );

		if ( colormode == 1 ) // parallel
			gl::drawStrokedRect( Rectf(menux[0], menuy[5], menux[2], menuy[6]) );
		else //if ( colormode == 2 ) // sequential
			gl::drawStrokedRect( Rectf(menux[2], menuy[5], menux[4], menuy[6]) );
	}
}

CINDER_APP_NATIVE( ColorWavesApp, RendererGl )

void ColorWavesApp::restart()
{
	// reset all Particles to white with zero color velocity.
	for( int i = 0; i < mapx; i++ )
	{
		for( int k = 0; k < mapy; k++ )
		{
			map[i][k].color.set(1.0f, 1.0f, 1.0f);
			map[i][k].vel.set(0.0f, 0.0f, 0.0f);
		}
	}



}

