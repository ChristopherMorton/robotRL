// RobotRL includes
#include "display.h"
#include "menu.h"
#include "game.h"
#include "shutdown.h"
#include "listeners.h"
#include "defs.h"
#include "log.h"

// SFML includes
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

// lib includes
#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_WindowEventManager.hpp"
#include "SFML_TextureManager.hpp"
#include "IMGuiManager.hpp"
#include "IMCursorManager.hpp"
#include "IMButton.hpp"

// C includes
#include <stdio.h>

// C++ includes
#include <deque>
#include <fstream>

using namespace sf;

// Global app-state variables

sf::RenderWindow *r_window;

// Managers
IMGuiManager* gui_manager;
IMCursorManager* cursor_manager;
SFML_TextureManager* texture_manager;
SFML_WindowEventManager* event_manager;

// Config
sf::Font font;

// Save state

// App state - i.e. which parts of the app are active
enum AppState {
   PRELOAD,
   LOADING,
   POSTLOAD,
   MAIN_MENU,
   IN_GAME,
   SHUT_DOWN
};
   
AppState app_state;

// Specific data for the menu sections

void resetView()
{
   r_window->setView( r_window->getDefaultView() );
}


///////////////////////////////////////////////////////////////////////////////
// Gui
///////////////////////////////////////////////////////////////////////////////

void loadTestLevel()
{
}

///////////////////////////////////////////////////////////////////////////////
// Asset Loading
///////////////////////////////////////////////////////////////////////////////

#define PROGESS_CAP 100

#define ASSET_TYPE_END 0
#define ASSET_TYPE_TEXTURE 1
#define ASSET_TYPE_SOUND 2

struct Asset
{
   int type;
   string path;

   Asset(int t, string& s) { type=t; path=s; }
};

deque<Asset> asset_list;

int loadAssetList()
{
   /* Actually read AssetList.txt 
    */ 
   string type, path;
   ifstream alist_in;
   alist_in.open("res/AssetList.txt");

   while (true) {
      // Get next line
      alist_in >> type >> path;
      if (alist_in.eof())
         break;

      if (alist_in.bad()) {
         log("Error in AssetList loading - alist_in is 'bad' - INVESTIGATE");
         break;
      }

      if (type == "IMAGE")
         asset_list.push_back( Asset( ASSET_TYPE_TEXTURE, path ) );
   }

   return 0;
}

int loadAsset( Asset& asset )
{
   if (asset.type == ASSET_TYPE_TEXTURE)
      texture_manager->addTexture( asset.path );

   return 0;
}

// preload gets everything required by the loading animation
int preload()
{
   log("Preload");

   app_state = LOADING;
   return 0;
}

int progressiveInit()
{
   static int progress = 0;
   log ("Progressive init");

   switch (progress) {
      default:
         return 0;
   }

   return 1;
}

// progressiveLoad works through the assets a bit at a time
int progressiveLoad()
{
   return 0;



   static int asset_segment = 0;
   int progress = 0;

   switch (asset_segment) {
      case 0:
         // Need to load asset list first
         loadAssetList();
         asset_segment = 1;
         return -1;
      case 1:
         while (!asset_list.empty())
         {
            loadAsset( asset_list.front() );
            asset_list.pop_front();
            if (progress++ > PROGESS_CAP)
               return -1;
         }
         asset_segment = 2;
         return -1;
      case 2:
         if (progressiveInit() == 0)
            asset_segment = 3;
         return -1;
      default:
         app_state = POSTLOAD;
   }

   return 0;
}

// postload clears the loading structures and drops us in the main menu
int postload()
{
   log("Postload");
   //delete(asset_list);

   app_state = MAIN_MENU;
   return 0;
}

int loadingAnimation(int dt)
{
   r_window->clear(sf::Color::Black);
   r_window->display();
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Display array
//////////////////////////////////////////////////////////////////////

struct ColorChar {
   sf::Color fg;
   sf::Color bg;
   unsigned int u_c;

   ColorChar( sf::Color fore, sf::Color back, unsigned int c) {
      fg = fore; bg = back; u_c = c; 
   }
   ColorChar() {
      fg = sf::Color::White;
      bg = sf::Color::Black;
      u_c = ' ';
   }
};

ColorChar **display_array;

void initDisplayArray()
{
   log("Init display_array");

   display_array = new ColorChar*[30];
   for (int i = 0; i < 30; ++i)
      display_array[i] = new ColorChar[80];
}

void clearDisplay()
{
   r_window->clear(sf::Color::Black);

   for (int i = 0; i < 30; ++i) {
      for (int j = 0; j < 80; ++j) {
         display_array[i][j].fg = sf::Color::White;
         display_array[i][j].bg = sf::Color::Black;
         display_array[i][j].u_c = ' ';
      }
   }
}

int writeChar( unsigned int c, sf::Color fg, sf::Color bg, int x, int y )
{
   display_array[y][x] = ColorChar( fg, bg, c );
   return 0;
}

int writeString( std::string s, sf::Color fg, sf::Color bg, int x, int y )
{
   for( unsigned int i = 0; i < s.length(); ++i ) {
      if (x+i >= 80) break;

      writeChar( s[i], fg, bg, x+i, y );
   }
   return 0;
}

int colorInvert( int x_base, int y_base, int x_end, int y_end )
{
   for( int y = y_base; y <= y_end; ++y ) {
      for( int x = x_base; x <= x_end; ++x ) { 
         Color fore = display_array[y][x].fg;
         fore.r = 255 - fore.r;
         fore.g = 255 - fore.g;
         fore.b = 255 - fore.b;
         display_array[y][x].fg = fore;
         Color back = display_array[y][x].bg;
         back.r = 255 - back.r;
         back.g = 255 - back.g;
         back.b = 255 - back.b;
         display_array[y][x].bg = back;
      }
   }
   return 0;
}

int colorSwitch( int x_base, int y_base, int x_end, int y_end )
{
   for( int y = y_base; y <= y_end; ++y ) {
      for( int x = x_base; x <= x_end; ++x ) { 
         Color fore = display_array[y][x].fg;
         display_array[y][x].fg = display_array[y][x].bg;
         display_array[y][x].bg = fore;
      }
   }
   return 0;
}

int dim( int x_base, int y_base, int x_end, int y_end )
{
   for( int y = y_base; y <= y_end; ++y ) {
      for( int x = x_base; x <= x_end; ++x ) { 
         Color fore = display_array[y][x].fg;
         fore.r = fore.r / 2;
         fore.g = fore.g / 2;
         fore.b = fore.b / 2;
         display_array[y][x].fg = fore;
         Color back = display_array[y][x].bg;
         back.r = back.r / 2;
         back.g = back.g / 2;
         back.b = back.b / 2;
         display_array[y][x].bg = back;
      }
   }
   return 0;
}

void drawDisplay()
{
   sf::Text c;
   c.setFont(font);
   c.setCharacterSize(16);

   sf::RectangleShape backing(sf::Vector2f(10, 20));

   for (int i = 0; i < 30; ++i) {
      for (int j = 0; j < 80; ++j) {
         ColorChar cc = display_array[i][j];

         if (cc.bg != sf::Color::Black) {
            // Draw backing rectangle
            backing.setFillColor( cc.bg );
            backing.setPosition(j*10, i*20);
            r_window->draw(backing);
         }

         c.setString( cc.u_c );
         c.setPosition(j*10, i*20);
         c.setColor( cc.fg );
         r_window->draw(c);
      }
   }

   r_window->display();
}

//////////////////////////////////////////////////////////////////////
// Listeners
//////////////////////////////////////////////////////////////////////

// Window
bool MainWindowListener::windowClosed( )
{
   shutdown(1,1);
   return true;
}

bool MainWindowListener::windowResized( const sf::Event::SizeEvent &resized )
{

   return true;
}

bool MainWindowListener::windowLostFocus( )
{

   return true;
}

bool MainWindowListener::windowGainedFocus( )
{

   return true;
}

// Mouse
bool MainMouseListener::mouseMoved( const sf::Event::MouseMoveEvent &mouse_move )
{
   return true;
}

bool MainMouseListener::mouseButtonPressed( const sf::Event::MouseButtonEvent &mouse_button_press )
{
   log("Clicked");
   return true;
}

bool MainMouseListener::mouseButtonReleased( const sf::Event::MouseButtonEvent &mouse_button_release )
{
   log("Un-Clicked");
   return true;
}

bool MainMouseListener::mouseWheelMoved( const sf::Event::MouseWheelEvent &mouse_wheel_move )
{
   return true;
}

// Key
bool MainKeyListener::keyPressed( const sf::Event::KeyEvent &key_press )
{
   int mod = (key_press.alt?MOD_ALT:0) 
           | (key_press.shift?MOD_SHIFT:0) 
           | (key_press.control?MOD_CTRL:0);

   if (app_state == MAIN_MENU) {
      if (sendKeyToMenu( key_press.code, mod ) == 1) // Game is ready to play
         app_state = IN_GAME;
   }
   else if (app_state == IN_GAME) {
      sendKeyToGame( key_press.code, mod );
   }

   return true;
}

bool MainKeyListener::keyReleased( const sf::Event::KeyEvent &key_release )
{
   return true;
}

///////////////////////////////////////////////////////////////////////////////
// Execution starts here
///////////////////////////////////////////////////////////////////////////////

int runApp()
{
   // Setup the window
   shutdown(1,0);
   r_window = new RenderWindow(sf::VideoMode(800, 600, 32), "Robot Jones", Style::None);

   // Setup various resource managers
   gui_manager = &IMGuiManager::getSingleton();
   cursor_manager = &IMCursorManager::getSingleton();
   texture_manager = &SFML_TextureManager::getSingleton();
   event_manager = &SFML_WindowEventManager::getSingleton();

   SFML_GlobalRenderWindow::set( r_window );
   gui_manager->setRenderWindow( r_window );

   texture_manager->addSearchDirectory( "res/" ); 

   // Setup event listeners
   MainWindowListener w_listener;
   MainMouseListener m_listener;
   MainKeyListener k_listener;
   event_manager->addWindowListener( &w_listener, "main" );
   event_manager->addMouseListener( &m_listener, "main" );
   event_manager->addKeyListener( &k_listener, "main" );

   // We need to load a loading screen
   app_state = PRELOAD;

   // Timing!
   sf::Clock clock;
   unsigned int old_time, new_time, dt;
   old_time = 0;
   new_time = 0;

   // Set Font
   if (!font.loadFromFile("res/LiberationMono-Regular.ttf"))
   {
       log("Couldn't load font 'LiberationMono-Regular.tff'");
       exit(0);
   }

   // Setup display
   initDisplayArray();

   // Loading
   preload();
   while (progressiveLoad())
      loadingAnimation(clock.getElapsedTime().asMilliseconds());
   postload();

   // Now setup some things using our new resources
   //cursor_manager->createCursor( IMCursorManager::DEFAULT, texture_manager->getTexture( "FingerCursor.png" ), 0, 0, 40, 60);
   //cursor_manager->createCursor( IMCursorManager::CLICKING, texture_manager->getTexture( "FingerCursorClick.png" ), 0, 0, 40, 60);

   // Initialise some GUIs - first run-through is init
   progressiveInit();

//////////////////////////////////////////////////////////////////////
// Main Loop
//////////////////////////////////////////////////////////////////////
   log("Entering main loop");
   while (shutdown() == 0)
   {
      event_manager->handleEvents();

      clearDisplay();

      if (app_state == MAIN_MENU) { 

         displayMenu();

      } else if (app_state == IN_GAME) {
   
         playGame();
         displayGame();
      }
   }

   log("End main loop");
   
   r_window->close();

   return 0;
}


int main(int argc, char* argv[])
{
   return runApp();
}
