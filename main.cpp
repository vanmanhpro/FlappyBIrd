#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <deque>
#include <stdlib.h>
#include <time.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <sstream>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int GAMEOVER_WIDTH = 500;
const int GAMEOVER_HEIGHT = 95;

const int START_WIDTH = 331;
const int START_HEIGHT = 87;

const int WALL_WIDTH = 88;
const int WALL_HEIGHT = 490;
const int WALL_VELOCITY = 3;
const int WALL_GAP = 250;
const int WALL_SPACE = 150;

const int TOTAL_WALLS = 4;

enum LButtonFrame
{
    BUTTON_FRAME_MOUSE_OUT,
    BUTTON_FRAME_MOUSE_OVER_MOTION,
    BUTTON_FRAME_MOUSE_DOWN,
    BUTTON_FRAME_MOUSE_UP,
    TOTAL_FRAME
};

class LTexture
{
    public:
        LTexture();
        ~LTexture();
        bool loadFromFile( std::string path );
        bool loadFromRenderedText( std::string textureText, SDL_Color textColor );
        void free();
        void render( int x, int y,
                    SDL_Rect* clip = NULL,
                    double angle = 0.0,
                    SDL_Point* center = NULL,
                    SDL_RendererFlip flip = SDL_FLIP_NONE );
        int getWidth();
        int getHeight();
    private:
        SDL_Texture* mTexture;
        int mWidth;
        int mHeight;
};

class Bird
{
    public:
        static const int BIRD_WIDTH = 61;
        static const int BIRD_HEIGHT = 40;
        static const int BIRD_GRAVITY = 2;
        static const int BIRD_PUSH = 4;

        Bird();
        void handleEvent( SDL_Event& e );
        void fall();
        void move();
        void render();
        bool collapse();
    private:
        SDL_Rect mPosRect;
        int mPosX, mPosY;
        int mVelY;
};

class LButton
{
    public:
        LButton();
        void setArea( int x, int y, int w, int h );
        void handleEvent( SDL_Event* e );
        void render();
        int getCurrentMovement();
    private:
        SDL_Rect mArea;
        LButtonFrame mCurrentMovement;
};

bool init();
bool loadMedia();
bool close();
bool checkCollision( SDL_Rect a, SDL_Rect b);

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

LTexture BirdAnimationTexture;
LTexture gBackGroundTexture;
LTexture gWallSpriteSheetTexture;
LTexture gGameOverTexture;
LTexture gStartButtonTexture[ TOTAL_FRAME ];
LTexture gScoreTexture;
LButton  startButton;

TTF_Font *gFont = NULL;

Mix_Music *gMusic = NULL;
Mix_Chunk *gFlap = NULL;
Mix_Chunk *gCollide = NULL;

std::deque<SDL_Rect> walls;

bool init()
{
    SDL_Init( SDL_INIT_VIDEO );
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
    gWindow = SDL_CreateWindow(" Flappy Bird ",
                     SDL_WINDOWPOS_UNDEFINED,
                     SDL_WINDOWPOS_UNDEFINED,
                     SCREEN_WIDTH,
                     SCREEN_HEIGHT,
                     SDL_WINDOW_SHOWN);
    gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
    int imgFlags = IMG_INIT_PNG;
    IMG_Init( imgFlags );
    //Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 );
    if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
        {
            printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
            //success = false;
        }
    if( TTF_Init() == -1 )
        {
            printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
        }
}


bool LTexture::loadFromFile( std::string path )
{
    free();
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load( path.c_str() );
    if ( loadedSurface == NULL )
    {
        printf(" cannot loadSurface \n");
        return false;
    }

    SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0x80, 0xFF, 0) );

    newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
    if ( newTexture == NULL )
    {
        printf(" cannot create newTexture \n");
        return false;
    }
    mWidth = loadedSurface->w;
    mHeight = loadedSurface->h;
    SDL_FreeSurface( loadedSurface );
    mTexture = newTexture;

    return mTexture != NULL;
}

bool LTexture::loadFromRenderedText( std::string textureText, SDL_Color textColor)
{
    free();
	SDL_Surface* textSurface = TTF_RenderText_Solid( gFont, textureText.c_str(), textColor );
    mTexture = SDL_CreateTextureFromSurface( gRenderer, textSurface );
	mWidth = textSurface->w;
	mHeight = textSurface->h;
	SDL_FreeSurface( textSurface );
	return mTexture != NULL;
}

bool loadMedia()
{
    gFont = TTF_OpenFont( "lazy.ttf", 50 );
    if ( gFont == NULL ) printf( " cannot load Font : %s \n", SDL_GetError() );
    BirdAnimationTexture.loadFromFile("bird1.png");
    gBackGroundTexture.loadFromFile("theme1.png");
    gWallSpriteSheetTexture.loadFromFile("wall.png");
    gGameOverTexture.loadFromFile("gameover1.png");
    gStartButtonTexture[ BUTTON_FRAME_MOUSE_OUT ].loadFromFile("startbutton.png");
    gStartButtonTexture[ BUTTON_FRAME_MOUSE_OVER_MOTION ].loadFromFile("startbuttonmouseover.png");
    gMusic = Mix_LoadMUS( "beat.wav" );
	if( gMusic == NULL )
	{
		printf( "Failed to load beat music! SDL_mixer Error: %s\n", Mix_GetError() );
		//success = false;
	}
    gFlap = Mix_LoadWAV( "low.wav" );
    gCollide = Mix_LoadWAV( "scratch.wav" );
}

bool close()
{
    TTF_CloseFont( gFont );
	gFont = NULL;

	gScoreTexture.free();
    BirdAnimationTexture.free();
    Mix_FreeChunk( gFlap );
    Mix_FreeChunk( gCollide );
    gFlap = NULL;
    gCollide = NULL;

    //Free the music
    Mix_FreeMusic( gMusic );
    gMusic = NULL;

    SDL_DestroyRenderer( gRenderer );
    SDL_DestroyWindow( gWindow );
    gRenderer = NULL;
    gWindow = NULL;
    SDL_Quit();
    IMG_Quit();
    Mix_Quit();
    TTF_Quit();
}

LTexture::LTexture()
{
    mTexture = NULL;
    mWidth = 0;
    mHeight = 0;
}

LTexture::~LTexture()
{
    free();
}

int LTexture::getWidth()
{
    return mWidth;
}

int LTexture::getHeight()
{
    return mHeight;
}

void LTexture::free()
{
    if (mTexture != NULL)
    {
        SDL_DestroyTexture( mTexture );
        mTexture = NULL;
        mWidth = 0;
        mHeight = 0;
    }
}

void LTexture::render( int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip )
{
    SDL_Rect renderQuad = { x, y, mWidth, mHeight };
    if ( clip != NULL )
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    SDL_RenderCopyEx( gRenderer, mTexture, clip, &renderQuad, angle, center, flip );
}

Bird::Bird()
{
    mPosX = 30;
    mPosY = 30;
    mVelY = 0;
    mPosRect = { mPosX, mPosY, BIRD_WIDTH, BIRD_HEIGHT };
}

void Bird::handleEvent( SDL_Event& e )
{
    if ( e.type = SDL_KEYDOWN && e.key.repeat == 0 )
    {
        switch ( e.key.keysym.sym )
        {
            case SDLK_UP: mVelY = -BIRD_PUSH;
            Mix_PlayChannel( -1, gFlap, 1);
        }
    }
}

void Bird::move()
{
    mPosY += mVelY;
    if ( mPosY + BIRD_HEIGHT > SCREEN_HEIGHT )
    {
        mPosY = SCREEN_HEIGHT - BIRD_HEIGHT;
        mVelY = 0;
    }
    mPosRect.y = mPosY;
}

void Bird::fall()
{
    if ( mPosY + BIRD_HEIGHT < SCREEN_HEIGHT )
    {
        mVelY += BIRD_GRAVITY;
    }
}

void Bird::render()
{
    BirdAnimationTexture.render( mPosX, mPosY);
}

bool Bird::collapse()
{
    for( int i = 0; i < walls.size(); i++)
    {
        if ( checkCollision( mPosRect, walls.at(i) ) )
        {
            Mix_PlayChannel( -1, gCollide, 1);
            return true;
        }
        SDL_Rect temp = walls.at(i);
        temp.y -= WALL_HEIGHT + WALL_SPACE;
        if ( checkCollision( mPosRect, temp ) )
        {
            Mix_PlayChannel( -1, gCollide, 1);
            return true;
        }
    }
    return false;
}

LButton::LButton()
{
    mArea = { 0, 0, 0, 0 };
    mCurrentMovement = BUTTON_FRAME_MOUSE_OUT;
}

void LButton::setArea( int x, int y, int w, int h)
{
    mArea = { x, y, w, h };
}

int LButton::getCurrentMovement()
{
    return mCurrentMovement;
}

void LButton::render()
{
    gStartButtonTexture[ mCurrentMovement ].render( mArea.x, mArea.y );
}

void LButton::handleEvent( SDL_Event* e )
{
    if( e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP || e->type == SDL_MOUSEMOTION )
    {
        int x, y;
        SDL_GetMouseState( &x, &y );
        bool inside = true;
        if ( ( x < mArea.x ) ||
           ( x > mArea.x + mArea.w ) ||
           ( y < mArea.y ) ||
           ( y > mArea.y + mArea.h ) ) inside = false;
        if ( !inside )
        {
            mCurrentMovement = BUTTON_FRAME_MOUSE_OUT;
        }
        else
        switch ( e->type )
        {
            case SDL_MOUSEMOTION:
            mCurrentMovement = BUTTON_FRAME_MOUSE_OVER_MOTION;
            break;

            case SDL_MOUSEBUTTONDOWN:
            mCurrentMovement = BUTTON_FRAME_MOUSE_DOWN;
            break;

            case SDL_MOUSEBUTTONUP:
            mCurrentMovement = BUTTON_FRAME_MOUSE_UP;
        }
    }
}

void newWall()
{
    SDL_Rect temp = { SCREEN_WIDTH, WALL_SPACE + 30 + ( rand() % (SCREEN_HEIGHT - WALL_SPACE - 30) ), WALL_WIDTH, WALL_HEIGHT };
    walls.push_back( temp );
}

void moveAndRenderWalls( bool gameOver)
{
    if( !gameOver )
    {
        for( int i = 0; i < walls.size(); i++)
        {
            walls.at(i).x -= WALL_VELOCITY;
        }

        if( walls.at(0).x + WALL_WIDTH < 0 ) walls.pop_front();
        if( walls.at( walls.size() - 1 ).x + WALL_WIDTH + WALL_GAP < SCREEN_WIDTH ) newWall();
    }
    for( int i = 0; i < walls.size(); i++)
    {
        gWallSpriteSheetTexture.render( walls.at(i).x, walls.at(i).y );
        gWallSpriteSheetTexture.render( walls.at(i).x, walls.at(i).y - WALL_HEIGHT - WALL_SPACE );
    }
}

bool checkCollision( SDL_Rect a, SDL_Rect b )
{
    int leftA, leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;

    leftA = a.x;
    rightA = a.x + a.w;
    topA = a.y;
    bottomA = a.y + a.h;

    leftB = b.x;
    rightB = b.x + b.w;
    topB = b.y;
    bottomB = b.y + b.h;

    if( bottomA <= topB )
        return false;
    if( topA >= bottomB )
        return false;
    if( rightA <= leftB )
        return false;
    if( leftA >= rightB )
        return false;

    return true;
}

int main( int argc, char* args[] )
{

    srand (time(NULL));

    init();
    loadMedia();
    bool quit = false;
    bool gameOver = false;
    bool gameStart = false;

    Uint32 startTime = 0;
    std::stringstream scoreText;

    SDL_Color textColor = { 255, 100, 0 };

    SDL_Event e;

    int frameBird = 0, frameBackGround = 0;
    int scrollingOffSet = 0;
    int score = 0;

    startButton.setArea( ( SCREEN_WIDTH - START_WIDTH ) / 2, ( SCREEN_HEIGHT - START_HEIGHT ) / 2, START_WIDTH, START_HEIGHT );

    Mix_PlayMusic( gMusic, -1 );

    newWall();

    Bird bird;

    while (!quit)
    {
        SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear( gRenderer );
        if ( !gameStart )
        {
            startTime = SDL_GetTicks();
            while ( SDL_PollEvent( &e ) != 0 )
            {
                if ( e.type == SDL_QUIT )
                {
                    quit = true;
                    break;
                }
                startButton.handleEvent( &e );
            }

            switch ( startButton.getCurrentMovement() )
            {
                case BUTTON_FRAME_MOUSE_OVER_MOTION:
                startButton.render(); break;
                case BUTTON_FRAME_MOUSE_OUT:
                startButton.render(); break;
                case BUTTON_FRAME_MOUSE_DOWN:
                gameStart = true; break;
            }

            SDL_RenderPresent( gRenderer );
            continue;
        }

        while  ( SDL_PollEvent( &e ) != 0 )
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
                break;
            }
            // if gameOver then SDL_PollEvent hold 1 quest: close the window
            if ( gameOver ) continue;
            // derive bird movement from keyboard
            bird.handleEvent( e );
        }
        // if gameOver stop every movement
        //if ( gameOver ) continue;

        bird.move();

        if ( frameBird == 10 )
        {
            bird.fall();
            frameBird = 0;
            if ( !gameOver ) score++;
        }
        frameBird++;

        if ( !gameOver )scrollingOffSet -= 2;
        if ( scrollingOffSet < -gBackGroundTexture.getWidth() )
        {
            scrollingOffSet = 0;
        }

        gBackGroundTexture.render( scrollingOffSet, 0 );
        gBackGroundTexture.render( scrollingOffSet + gBackGroundTexture.getWidth(), 0 );

        moveAndRenderWalls( gameOver );
        bird.render();

        if ( gameOver )
        {
            //RenderGameOverImage
            gGameOverTexture.render( ( SCREEN_WIDTH - GAMEOVER_WIDTH ) / 2, ( SCREEN_HEIGHT - GAMEOVER_HEIGHT ) / 2 );
            startTime = SDL_GetTicks();
        }

        if (!gameOver)
        {
            scoreText.str( "" );
            scoreText << "Score: " << score;
            gScoreTexture.loadFromRenderedText( scoreText.str().c_str(), textColor);
            gScoreTexture.render( ( SCREEN_WIDTH - gScoreTexture.getWidth() ) / 2, 100 );
        }
        else
        {
            gScoreTexture.render( ( SCREEN_WIDTH - gScoreTexture.getWidth() ) / 2, 300 );
        }
        SDL_RenderPresent( gRenderer );

        if ( gameOver ||  bird.collapse() ) gameOver = true;
    }
    Mix_HaltMusic();
    close();
}
