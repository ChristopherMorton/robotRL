// RobotRL includes
#include "display.h"
#include "shutdown.h"
#include "listeners.h"
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
   GAME_START,
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

char **display_array;

void initDisplayArray()
{
   log("Init display_array");
   display_array = new char*[30];
   for (int i = 0; i < 30; ++i)
      display_array[i] = new char[80];
}

void clearDisplay()
{
   r_window->clear(sf::Color::Black);

   for (int i = 0; i < 30; ++i)
      for (int j = 0; j < 80; ++j)
         display_array[i][j] = ' ';
}

int writeChar( unsigned int c, sf::Color color, int x, int y )
{
   display_array[y][x] = (char)c;
}

int writeString( std::string s, sf::Color color, int x, int y )
{
   for( int i = 0; i < s.length(); ++i ) {
      if (x+i >= 80) break;

      display_array[y][x+i] = s[i];
   }
}

void drawDisplay()
{
   sf::Text c;
   c.setFont(font);
   c.setCharacterSize(16);
   c.setColor(sf::Color::White);

   for (int i = 0; i < 30; ++i) {
      for (int j = 0; j < 80; ++j) {
         c.setString( display_array[i][j] );
         c.setPosition(j*10, i*20);
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
   if (key_press.code == sf::Keyboard::Q)
      shutdown(1,1);

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
         // Display main menu

         // Display options

         writeString("New Game", sf::Color::White, 0, 0);
         writeString("0.........1.........2.........3.........4.........5.........6.........7.........8.........9.........", sf::Color::Yellow, 0, 1);

         drawDisplay();

      } else if (app_state == IN_GAME) {

         new_time = clock.getElapsedTime().asMilliseconds();
         dt = new_time - old_time;
         old_time = new_time;

         event_manager->handleEvents();

         gui_manager->begin();
   
         r_window->display();

         gui_manager->end();

         cursor_manager->drawCursor();

         r_window->display();
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
