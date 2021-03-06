#include "stdafx.h"

#include<iomanip>
#include<Windows.h>       //GetKeyState
#include<stdlib.h>		 //SysClear                                     // IN THE PLAYFIELD DIGITS MEAN THE FOLLOWING:
#include<iostream>														// 1,2,3,4,5,6,7 = ACTIVE TETROMINO PIECE
#include<string>														// (ACTIVE PIECE + 50) = STATIONARY TETROMINO PIECE
#include<stdlib.h>    // srand, rand									// 9 = THE BORDER
#include<time.h>     // Random from System Clock                        // Seven Different digits for every piece because
#include<thread>	//system thread sleep (Game tick)                   // each piece has a unique color

using namespace std;


bool playing = false; // Global boolean to check for audio playback

void initializeTetromino(int nexttetromino[4][4]);                                          //Creates a random Tetromino
void Draw(int pf[][21], int w, int h, int level, int score, int nextpiece[4][4]); //Draws the current state of PLayField
void SpawnNew(int nexttetromino[4][4], int currentpiece[4][4], int pf[][21], bool& GameOver);	//Spawns a new tetromino piece and updates current orientation stored
void initializePlayfield(int pf[][21],int w, int h);                 //Sets up the Playfield 2D array
void updatePlayField(int pf[][21], int w, int h, int& score);        //Collision Logic + Motion - Downwards
void CopyToNext(int localtetromino[4][4], int nexttetromino[4][4]);  //Copies to Global Tetromino Piece(used in intializeTetromini() Function)
bool checkactive(int pf[][21],int w, int h);                         //Checks for active piece in playfield
bool checkgameover(int pf[][21], int w);			  	    		 //Checks if upper most row is occupied
void handleinput(int pf[][21], int w, int h, int&score);             //Collision Logic + Motion - For Sides
void handlerotation(int pf[][21], int w, int h, int currentpiece[4][4]); //Rotation Logic + Collision Detection
void checklines(int pf[][21], int w, int h, int& score, int level);  //Checks for complete lines and updates playfield
int findminxpiece(int a[4][4]);                                      //Returns minimum y position of piece
int findminypiece(int a[4][4]);                                      //Returns minimum x position of piece
int findminxfield(int pf[][21], int w, int h);                       //Returns minimum x position of active piece in field
int findminyfield(int pf[][21], int w , int h);                      //Returns minimum y position of actice piece in field

int main()
{
	PlaySound(TEXT("background.wav"), NULL, SND_LOOP | SND_ASYNC); //Playing awesome tunes :D

	//initialize random seed:
	srand(time(NULL));
	
	const int pfheight = 21;                         //Height of playfield
	const int pfwidth = 12;                          //Width of playfield
	int pf[pfwidth][pfheight] = { 0 };               //Playfield array
	bool GameOver = false;                           //GameOver flag
	bool Spawn = true;                               //Spawn new piece flag
	int speed = 15;                                  //Current speed of active piece (Current level)
	int speedcount = 0;                              //Counter after which motion of active piece is initiated
	bool lockrotation = false;                       //To lock rotation if Z is pressed and held
	int piecestotal = 0;                             //Counter for total pieces
	int level = 1;
	int score = 0;
	int nexttetromino[4][4]; // Next piece that will be spawned
	int currentpiece[4][4]; // Keeps current piece and its orientation


	initializePlayfield(pf, pfwidth, pfheight);
	
	initializeTetromino(nexttetromino);
	
	
   
	while (!GameOver) //MAIN LOOP
	{
		

		//INPUT
		handleinput(pf, pfwidth, pfheight, score);				       //Collision Detection and user defined motion
		
		if (lockrotation == false)                                     // Works once for each key press and hold
		{
			handlerotation(pf, pfwidth, pfheight, currentpiece);                      //Collision Detection and rotation
		}
		
		if ((GetKeyState('z') & 0x8000) || (GetKeyState('Z') & 0x8000))
		{
			lockrotation = true;                                        // If the Z key is held, flag true
		}
		else
			lockrotation = false;                                       
		
		//GAME STUFF
		if (Spawn == true)                                          //If there is no active piece then spawn a new one.
		{
			
			SpawnNew(nexttetromino, currentpiece, pf, GameOver);
			initializeTetromino(nexttetromino);                                  //Intialize a new random tetromino

			piecestotal++;                                                       //Count total number of pieces (used for leveling)
		}
		
		if (piecestotal % 16 == 0 && speed > 0)                                 // For every 15 pieces level up
		{
			piecestotal++;
			speed = speed - 1;                                                  //Increase the speed
			level++;
		}
		
		//DRAWING

		Draw(pf, pfwidth, pfheight, level, score, nexttetromino);



		//GAME TICK + CONSOLE REFRESH + LOGIC CHECK
		
		this_thread::sleep_for(25ms);                                     // Small Step = 1 Game Tick
		speedcount++;                                                     //Increment counter
		
		if (speed <= speedcount)                                          //If sufficient ticks have passed:
		{
			speedcount = 0;                                               //Reset the counter
			updatePlayField(pf, pfwidth, pfheight, score);                //Collision detection and downward motion
		}

		
		{ //set cursor to 0,0 so the console can be overwritten by next frame
			static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			COORD coord = { (SHORT)0, (SHORT)0 };
			SetConsoleCursorPosition(hOut, coord);
		}                                                   


		//CHECKS
		checklines(pf, pfwidth, pfheight, score, level);              //Check if lines are complete and updatefield & score
		Spawn = checkactive(pf, pfwidth, pfheight);                  //Check if to spawn a new piece
		
		if (!GameOver)
		{
			GameOver = checkgameover(pf, pfwidth);						 //Check if Game is over
		}
		
		if (GameOver == true) { Spawn = false; }					 //Set spawn to false if game is over

	}

	cout << "GAME OVER!" << endl;

    return 0;
}

void handlerotation(int pf[][21], int w, int h, int currentpiece[4][4])
{
	
	//Finding miny
	int miny = findminyfield(pf, w, h);

	//Finding min x
	int minx = findminxfield(pf, w, h);

	int currentorientation[4][4] = { 0 };                      //Initialzing currentorientation array
	int j = 0, i = 0;

	if ((GetKeyState('z') & 0x8000) || (GetKeyState('Z') & 0x8000))

	{
		//ROTATION 1. REFLECT THE ROWS 2. TRANSPOSE = 90 degree tilt of square array.
		
		int temp[4][4] = { 0 }, c = 0;                    //temporary for reversal
		
		for (int j = 0; j < 4; j++)                       //reversing elements in a row
		{
			c = 3;
			for (int i = 0; i < 4; i++)
			{
				temp[c][j] = currentpiece[i][j];             //Using global to refer to the position in the field
				c--;
			}
		}
		
		for (int j = 0; j < 4; j++)                       // transposing to current piece
		{
			for (int i = 0; i < 4; i++)
			{
				currentorientation[i][j] = temp[j][i];            //switching rows and columns.
			}
	   }



		int minxpiece = findminxpiece(currentorientation);                  //Getting minx of current piece

				
		
       // Calculating the offset in y (number of empty rows that will need to be ignored so the pieces are alligned)
   	   // Offset in x will be minpiece x, always.
		
		int offsetx = minxpiece, offsety = 0;
		bool empty = true;


		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				if (currentorientation[x][y] == 1 || currentorientation[x][y] == 2 || currentorientation[x][y] == 3 || currentorientation[x][y] == 4 || currentorientation[x][y] == 5 || currentorientation[x][y] == 6 || currentorientation[x][y] == 7)
				{
					empty = false;                       //if a single active piece is found in the row, the row is not empty
					break;                               // then break the loop
				}
			}
			if (empty == false) { break; }              // Since we need the empty rows from the top only to offset
			                                            // no need to continue, break the outer loop too
			offsety++;
		}

		
		
		
		
		bool collision = false;
		
		j = 0;
		for (int y = miny; y >= miny - 3; y--)      //Collision detection.
		{
			i = 0;
			for (int x = minx; x <= minx + 3; x++)
			{
				if ((currentorientation[i][j] == 1 || currentorientation[i][j] == 2 || currentorientation[i][j] == 3 || currentorientation[i][j] == 4 || currentorientation[i][j] == 5 || currentorientation[i][j] == 6 || currentorientation[i][j] == 7) && (pf[x - offsetx][y + offsety] == 9 || pf[x - offsetx][y + offsety] == 51 || pf[x - offsetx][y + offsety] == 52 || pf[x - offsetx][y + offsety] == 53 || pf[x - offsetx][y + offsety] == 54 || pf[x - offsetx][y + offsety] == 55 || pf[x - offsetx][y + offsety] == 56 || pf[x - offsetx][y + offsety] == 57))
				{
					collision = true;               //Checking if there is a piece/border in the position currentpiece
					break;                          //if there is collision break the loop
				}
				i++;
				if (collision == true) { break; }     //Even if a single piece is in collision, no need to continue
			}
			j++;
		}
		
		if (collision == false)
		{
			for (int i = 0; i < 4; i++)   //updating global (so we can save the current orientation for future rotations)
			{
				for (int j = 0; j < 4; j++)
				{
					currentpiece[i][j] = currentorientation[i][j];
				}
			}

			for (int y = miny; y >= miny - 3; y--)            // removing previous orientation
			{
				for (int x = minx; x <= minx + 3; x++)
				{
					if (pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7)
					{
						pf[x][y] = 0;                          // 0 is empty space in the play field
					}
				}
			}

			
			
			j = 0;
			for (int y = miny; y >= miny - 3; y--)      //printing new orientation on correct position
			{
				i = 0;
				for (int x = minx; x <= minx + 3; x++)
				{

					if (currentorientation[i][j] == 1 || currentorientation[i][j] == 2 || currentorientation[i][j] == 3 || currentorientation[i][j] == 4 || currentorientation[i][j] == 5 || currentorientation[i][j] == 6 || currentorientation[i][j] == 7)
					{
						pf[x - offsetx][y + offsety] = currentorientation[i][j];   //applying offset to playfield for allignment
					}
					i++;
				}
				j++;
			}

		}

	}


}

void handleinput(int pf[][21], int w, int h, int&score)
{
	bool keypress[3] = { 0 };           // 0 is left, 1 is down, 2 is right
	//Checking for key presses
	keypress[0] = GetKeyState('\x25') & 0x8000;
	keypress[2] = GetKeyState('\x27') & 0x8000;
	keypress[1] = GetKeyState('\x28') & 0x8000;

	//Finding length of tetromino
	int length = 0;
	for (int y = 0; y < h-1; y++)
		{
			for (int x = 1; x <= w-2; x++)
			{
				if (pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7)
				{
					length++;
					break;
				}
			}
		}
	


	//Finding miny
	int miny = findminyfield(pf, w, h);



	//Calculating maxy
	int maxy = miny - length + 1;



	//if left key is pressed
	if (keypress[0] == true)
	{
		bool fit = true;
		for (int y = maxy; y <= miny; y++)                   //Collision Detection
		{
			for (int x = 1; x <= w-2; x++)         
			{
				if ((pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7) && (pf[x-1][y] == 9 || pf[x-1][y] == 1 + 50 || pf[x-1][y] == 2 + 50 || pf[x-1][y] == 3 + 50 || pf[x-1][y] == 4 + 50 || pf[x-1][y] == 5 + 50 || pf[x-1][y] == 6 + 50 || pf[x-1][y] == 7 + 50))
				{
					fit = false;                            //If the next piece is border or stationary piece
					break;                                  //Break the loop
				}
			}
			if (fit == false) { break; }                    //If there is a single collision no need to reiterate
		}

		if (fit == true)
		{
			for (int y = maxy; y <= miny; y++)             //If there is no collision move the active pieces a stop left
			{
				for (int x = 1; x <= w - 2; x++)
				{
					if (pf[x][y] == 1)
					{
						pf[x][y] = 0;
						pf[x-1][y] = 1;                    
					}
					else if (pf[x][y] == 2)
					{
						pf[x][y] = 0;
						pf[x - 1][y] = 2;
					}
					else if (pf[x][y] == 3)
					{
						pf[x][y] = 0;
						pf[x - 1][y] = 3;
					}
					else if (pf[x][y] == 4)
					{
						pf[x][y] = 0;
						pf[x - 1][y] = 4;
					}
					else if (pf[x][y] == 5)
					{
						pf[x][y] = 0;
						pf[x - 1][y] = 5;
					}
					else if (pf[x][y] == 6)
					{
						pf[x][y] = 0;
						pf[x - 1][y] = 6;
					}
					else if (pf[x][y] == 7)
					{
						pf[x][y] = 0;
						pf[x - 1][y] = 7;
					}
				}
			}
		}
	}

	//if right key is pressed
	if (keypress[2] == true)
	{
		bool fit = true;
		for (int y = maxy; y <= miny; y++)           //Collision Detection for Right Side
		{
			for (int x = w-2; x > 0; x--)
			{
				if ((pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7) && (pf[x + 1][y] == 9 || pf[x + 1][y] == 1 + 50 || pf[x + 1][y] == 2 + 50 || pf[x + 1][y] == 3 + 50 || pf[x + 1][y] == 4 + 50 || pf[x + 1][y] == 5 + 50 || pf[x + 1][y] == 6 + 50 || pf[x + 1][y] == 7 + 50))
				{
					fit = false;                    //If right most next piece is border or stationary piece wont fit
					break;                          //Break the loop
				}
			}
			if (fit == false) { break; }            //If a single collision is found no need to continue
		}

		if (fit == true)                            //If the piece fits i.e there is no collision
		{
			for (int y = maxy; y <= miny; y++)
			{
				for (int x = w-2; x > 0; x--)       //Look for active pieces and move them a step right
				{
					if (pf[x][y] == 1)
					{
						pf[x][y] = 0;
						pf[x + 1][y] = 1;                    
					}
					else if (pf[x][y] == 2)
					{
						pf[x][y] = 0;
						pf[x + 1][y] = 2;
					}
					else if (pf[x][y] == 3)
					{
						pf[x][y] = 0;
						pf[x + 1][y] = 3;
					}
					else if (pf[x][y] == 4)
					{
						pf[x][y] = 0;
						pf[x + 1][y] = 4;
					}
					else if (pf[x][y] == 5)
					{
						pf[x][y] = 0;
						pf[x + 1][y] = 5;
					}
					else if (pf[x][y] == 6)
					{
						pf[x][y] = 0;
						pf[x + 1][y] = 6;
					}
					else if (pf[x][y] == 7)
					{
						pf[x][y] = 0;
						pf[x + 1][y] = 7;
					}
				}
			}
		}
	}

	//if down key is pressed
	if (keypress[1] == true)
	{
		updatePlayField(pf, w, h, score);           //Since downward motion is handled my UpdatePlayfield, call it again.
	}
}

bool checkgameover(int pf[][21], int w)
{
	bool over = false;
	for (int x = 1; x <= w - 2; x++)
	{
		if (pf[x][0] == 51 || pf[x][0] == 52 || pf[x][0] == 53 || pf[x][0] == 54 || pf[x][0] == 55 || pf[x][0] == 56 || pf[x][0] == 57) { over = true; }
	}
	return over;
}

bool checkactive(int pf[][21],int w, int h)
{
	bool spawn = true;
	for (int y = 0; y < h - 1; y++)
	{
		for (int x = 1; x <= w - 2; x++)
		{
			if (pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7)
			{
				spawn = false;
				break;
			}

		}
		if (spawn == false) break;
	}

	return spawn;
}

void initializePlayfield(int pf[][21],int w, int h)
{
	for (int y = 0; y < h ; y++)
	{
		if (y == h-1)
		{
			for (int x = 0; x < w; x++)
			{
				pf[x][y] = 9;                    //Filling the last row with 9s
			}
		}
		else
		{
			for (int x = 0; x < w; x++)
			{
				if (x == 0 || x == w - 1)
				{
					pf[x][y] = 9;              //Filling the first and last positions of row with 9 rest is 0(empty space)
				}
				else
				{
					pf[x][y] = 0;
				}
			}
		}
	}
}

void updatePlayField(int pf[][21], int w, int h, int&score)
{
	
	

	int minx = 11;	                                    //11 for comparison since this is always greater than any minx
	int width = 0;
	int dummyminx = 12;                                 //temp variable to compare for minimum x position in a row

	//Finding width
	for (int x = 1; x <= w - 2; x++)
	{
		for (int y = 0; y < h - 1; y++)
		{
			if (pf[x][y] == 1 || pf[x][y] == 2|| pf[x][y] == 3|| pf[x][y] == 4|| pf[x][y] == 5|| pf[x][y] == 6|| pf[x][y] == 7)
			{
				width++;
				break;
			}
		}
	}

	//Finding min x
	minx = findminxfield(pf, w, h);

	//cout << width << " " << minx;               //For debugging purposes
	
	
	//Checking for collision from underneath and moving down
	
	bool fit = true;                             //by default piece fits
	
	
	for (int y = h - 1; y > -1; y--)  //collision detection loop: Is any of the pieces in the width in collision?
	{
		for (int x = minx; x <( minx + width); x++)      //running from the min x position till entire width of tetromino
		{
			if ((pf[x][y] == 1|| pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7) && (pf[x][y + 1] == 9 || pf[x][y + 1] == 1+50 || pf[x][y + 1] == 2+50 || pf[x][y + 1] == 3+50 || pf[x][y + 1] == 4+50 || pf[x][y + 1] == 5+50 || pf[x][y + 1] == 6+50 || pf[x][y + 1] == 7+50 ))
			{
				fit = false;
				break;                              //if even one of the bottom piece doesnt fit, no need to reiterate.
			}
		}
		if (fit == false) { score++; break; }               //breaking the loop if a collision is found.
	}

	//Moving down
	if (fit == true)                               //if the piece fits
	{
		for (int y = h - 1; y > -1; y--)
		{
			for (int x = minx; x < (minx + width); x++)
			{
				if(pf[x][y] ==1)                         //Run a loop to find all the active pieces
				{
					pf[x][y] = 0;
					pf[x][y + 1] = 1;                    //Move them a step down.
				}
				else if (pf[x][y] == 2)
				{
					pf[x][y] = 0;
					pf[x][y + 1] = 2;
				}
				else if (pf[x][y] == 3)
				{
					pf[x][y] = 0;
					pf[x][y + 1] = 3;
				}
				else if (pf[x][y] == 4)
				{
					pf[x][y] = 0;
					pf[x][y + 1] = 4;
				}
				else if (pf[x][y] == 5)
				{
					pf[x][y] = 0;
					pf[x][y + 1] = 5;
				}
				else if (pf[x][y] == 6)
				{
					pf[x][y] = 0;
					pf[x][y + 1] = 6;
				}
				else if (pf[x][y] == 7)
				{
					pf[x][y] = 0;
					pf[x][y + 1] = 7;
				}
			}
		}
	}
	else
	{

		for (int y = h - 1; y > -1; y--)               //If the piece is in collision
		{
			for (int x = minx; x < (minx + width); x++)
			{
				if (pf[x][y] == 1)                         //Freeze the piece
				{
					pf[x][y] = 51;
				}
				else if (pf[x][y] == 2)
				{
					pf[x][y] = 52;
				}
				else if (pf[x][y] == 3)
				{
					pf[x][y] = 53;
				}
				else if (pf[x][y] == 4)
				{
					pf[x][y] = 54;
				}
				else if (pf[x][y] == 5)
				{
					pf[x][y] = 55;
				}
				else if (pf[x][y] == 6)
				{
					pf[x][y] = 56;
				}
				else if (pf[x][y] == 7)
				{
					pf[x][y] = 57;
				}
			}
		}
	}

	
	

	
}

void initializeTetromino(int nexttetromino[4][4])
{
	

	// generate random number between 1 and 7 (Seven tetriminos in total):
	int r = rand() % 7 + 1;

	if (r == 1) 
	{
		int localtetromino[4][4] =
		{
			{1,1,0,0},
			{0,1,1,0},
			{0,0,0,0},
			{0,0,0,0}
		};

		CopyToNext(localtetromino,nexttetromino);
	}
	else if (r == 2)
	{
		int localtetromino[4][4] =
		{
			{ 0,2,0,0 },
			{ 2,2,2,0 },
			{ 0,0,0,0 },
			{ 0,0,0,0 }
		};

		CopyToNext(localtetromino, nexttetromino);
	}

	else if (r == 3)
	{
		int localtetromino[4][4] =
		{
			{ 3,3,3,3 },
			{ 0,0,0,0 },
			{ 0,0,0,0 },
			{ 0,0,0,0 }
		};

		CopyToNext(localtetromino, nexttetromino);
	}

	else if (r == 4)
	{
		int localtetromino[4][4] =
		{
			{ 4,4,0,0 },
			{ 4,4,0,0 },
			{ 0,0,0,0 },
			{ 0,0,0,0 }
		};

		CopyToNext(localtetromino, nexttetromino);
	}

	else if (r == 5)
	{
		int localtetromino[4][4] =
		{
			{ 0,5,5,0 },
			{ 5,5,0,0 },
			{ 0,0,0,0 },
			{ 0,0,0,0 }
		};

		CopyToNext(localtetromino, nexttetromino);
	}
	
	
	else if (r == 6)
	{
		int localtetromino[4][4] =
		{
			{ 6,0,0,0 },
			{ 6,6,6,0 },
			{ 0,0,0,0 },
			{ 0,0,0,0 }
		};

		CopyToNext(localtetromino, nexttetromino);

	}

	else  
	{
		int localtetromino[4][4] =
		{
			{ 0,0,7,0 },
			{ 7,7,7,0 },
			{ 0,0,0,0 },
			{ 0,0,0,0 }
		};
		
		CopyToNext(localtetromino, nexttetromino);
	}

	}

void Draw(int pf[][21],int w,int h, int level,int score, int nextpiece[4][4])
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	
	cout << "                  Level: "<< level << "     Score: " << score <<endl;
	cout << endl;
	
	for (int y = 0; y < h; y++)
	{
		

		if (y == 3)
		{
			cout << "    Next Piece:        ";
		}
		else if (y == 5 || y == 6 || y ==7 || y ==8)
		{
			cout << "        ";
			for (int x = 0; x < 4; x++)
			{
				
				if (nextpiece[x][y-5] == 1)        //For each piece assign a unique color (active or stationed)
				{
					SetConsoleTextAttribute(hConsole, 12);
					cout << char(219);
				}
				else if (nextpiece[x][y-5] == 2)
				{
					SetConsoleTextAttribute(hConsole, 13);
					cout << char(219);
				}
				else if (nextpiece[x][y-5] == 3)
				{
					SetConsoleTextAttribute(hConsole, 11);
					cout << char(219);
				}
				else if (nextpiece[x][y-5] == 4)
				{
					SetConsoleTextAttribute(hConsole, 14);
					cout << char(219);
				}
				else if (nextpiece[x][y-5] == 5)
				{
					SetConsoleTextAttribute(hConsole, 10);
					cout << char(219);
				}
				else if (nextpiece[x][y-5] == 6)
				{
					SetConsoleTextAttribute(hConsole, 9);
					cout << char(219);
				}
				else if (nextpiece[x][y-5] == 7)
				{
					SetConsoleTextAttribute(hConsole, 6);
					cout << char(219);
				}
				else
					cout << " ";
			}
			
			cout << "           ";
		}

		else
		{
			cout << "                       ";
		}

		for (int x = 0; x < w; x++)
		{
			if (pf[x][y] == 9)
			{
				SetConsoleTextAttribute(hConsole, 7);
				cout << char(219);             // 9 is border
			}
			else if (pf[x][y] ==0)
			{
				cout << " ";            // 0 is empty space so translate it to " "
			}
			else if(pf[x][y]==1 || pf[x][y] == 1 + 50)        //For each piece assign a unique color (active or stationed)
			{
				SetConsoleTextAttribute(hConsole, 12);
				cout << char(219);
			}
			else if (pf[x][y] == 2 || pf[x][y] == 2 + 50)
			{
				SetConsoleTextAttribute(hConsole, 13);
				cout << char(219);
			}
			else if (pf[x][y] == 3 || pf[x][y] == 3 + 50)
			{
				SetConsoleTextAttribute(hConsole, 11);
				cout << char(219);
			}
			else if (pf[x][y] == 4 || pf[x][y] == 4 + 50)
			{
				SetConsoleTextAttribute(hConsole, 14);
				cout << char(219);
			}
			else if (pf[x][y] == 5 || pf[x][y] == 5 + 50)
			{
				SetConsoleTextAttribute(hConsole, 10);
				cout << char(219);
			}
			else if (pf[x][y] == 6 || pf[x][y] == 6 + 50)
			{
				SetConsoleTextAttribute(hConsole, 9);
				cout << char(219);
			}
			else if (pf[x][y] == 7 || pf[x][y] == 7+50)
			{
				SetConsoleTextAttribute(hConsole, 6);
				cout << char(219);
			}

			else 
			{
				cout << "x";                   //Debugging purpose; checking for garbage/anomalous values
			}
		}
		cout << endl;
	}

	
}

void SpawnNew(int nexttetromino[4][4],int currentpiece[4][4], int pf[][21], bool& GameOver)
{
	
	for (int y = 0; y < 4; y++)
	{
		for (int x = 5; x < 9; x++)
		{
			if((nexttetromino[x-5][y] == 1 || nexttetromino[x - 5][y] == 2 || nexttetromino[x - 5][y] == 3 || nexttetromino[x - 5][y] == 4 || nexttetromino[x - 5][y] == 5 || nexttetromino[x - 5][y] == 6 || nexttetromino[x - 5][y] == 7) && (pf[x][y] == 51 || pf[x][y] == 52 || pf[x][y] == 53 || pf[x][y] == 54 || pf[x][y] == 55 || pf[x][y] == 56 || pf[x][y] == 57))
			{
				GameOver = true;    //if piece does not fit right away game is over 
			}
		}
	}
	

	if (GameOver == false) // If it fits
	{

		for (int y = 0; y < 4; y++)
		{
			for (int x = 5; x < 9; x++)
			{
				if (nexttetromino[x - 5][y] == 1 || nexttetromino[x - 5][y] == 2 || nexttetromino[x - 5][y] == 3 || nexttetromino[x - 5][y] == 4 || nexttetromino[x - 5][y] == 5 || nexttetromino[x - 5][y] == 6 || nexttetromino[x - 5][y] == 7)
				{
					pf[x][y] = nexttetromino[x - 5][y];  //copying next tetromino to the playfield
				}
			}
		}

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				currentpiece[i][j] = nexttetromino[i][j];        //Now the piece is in playfield and becomes currentpiece
			}
		}
	}

}

void CopyToNext(int localtetromino[4][4], int nexttetromino[4][4])
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			nexttetromino[i][j] = localtetromino[j][i];
		}
	}
}

int findminxpiece(int a[4][4])
{
	int minx = 4;
	int dummyminx = 4;

	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			if (a[x][y] == 1 || a[x][y] == 2 || a[x][y] == 3 || a[x][y] == 4 || a[x][y] == 5 || a[x][y] == 6 || a[x][y] == 7)
			{
				dummyminx = x;
			}
			if (dummyminx < minx) { minx = dummyminx; }   //minimum position is taken

		}
		dummyminx = 3;
	}
	return minx;
}

int findminxfield(int pf[][21], int w, int h)
{
	int minx = 11;                    //11 for comparison since it wil always be greater than any first minx
	int dummyminx = 12;               // same as above
	
	for (int y = 0; y < h - 1; y++)
	{
		for (int x = 1; x <= w - 2; x++)
		{
			if (pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7)
			{
				dummyminx = x;
			}
			if (dummyminx < minx) { minx = dummyminx; }   //minimum position is taken

		}
		dummyminx = 12;
	}

	return minx;
}

int findminyfield(int pf[][21], int w, int h)
{
	int miny = 0;
	int dummyminy = 0;
	for (int y = 0; y < h - 1; y++)
	{
		for (int x = 1; x <= w - 2; x++)
		{
			if (pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7)
			{
				dummyminy = y;
			}
			if (dummyminy > miny) { miny = dummyminy; }   //minimum position is taken

		}
		dummyminy = 0;
	}

	return miny;
}

int findminypiece(int a[4][4])
{
	int miny = 0;
	int dummyminy = 0;
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			if (a[x][y] == 1 || a[x][y] == 2 || a[x][y] == 3 || a[x][y] == 4 || a[x][y] == 5 || a[x][y] == 6 || a[x][y] == 7)
			{
				dummyminy = y;
			}
			if (dummyminy > miny) { miny = dummyminy; }   //minimum position is taken

		}
		dummyminy = 0;
	}

	return miny;
}

void checklines(int pf[][21], int w, int h, int& score, int level)
{
	

	{
		for (int x = 1; x <= w - 2; x++)
		{
			if ((pf[x][4] == 51|| pf[x][4] == 52 || pf[x][4] == 53 || pf[x][4] == 54 || pf[x][4] == 55 || pf[x][4] == 56 || pf[x][4] == 57)&& playing == false)
			{
				PlaySound(TEXT("watchout.wav"), NULL, SND_FILENAME | SND_ASYNC);  //rKO outta no where :D

				playing = true;
			}
		}
	}
	
	
	
	bool full = true;
	int lines = 0;


	for (int y = h - 2; y > 0; y--)   
	{
		full = false;
		for (int x = 1; x <= w - 2; x++)
		{
			if (pf[x][y] == 0 || pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7)
			{
				full = false;   //if an active piece or a space is found line is not full
				break;          //no need to continue just break the loop
			}
			else
			{
				full = true;    // else it is full
			}
		}
		if (full == true) { lines++; } //count the line as full
	}




	for (int y = h - 2; y > 0; y--)
	{
		full = false;
		for (int x = 1; x <= w - 2; x++)
		{
			if (pf[x][y] == 0|| pf[x][y] == 1 || pf[x][y] == 2 || pf[x][y] == 3 || pf[x][y] == 4 || pf[x][y] == 5 || pf[x][y] == 6 || pf[x][y] == 7)
			{
				full = false;
				break;
			}
			else
			{
				full = true;
			}
		}
		if (full == true)
		{

			for (int x = 1; x <= w - 2; x++)
			{
				pf[x][y] = 0;
			}

			
			for (int j = y - 1; j > 0; j--)
			{
				for (int x = 1; x <= w - 2; x++)
				{
						if (pf[x][j] == 51)
						{
							pf[x][j] = 0;
							pf[x][j + 1] = 51;                    //Move them a down
						}
						else if (pf[x][j] == 52)
						{
							pf[x][j] = 0;
							pf[x][j + 1] = 52;                    //Move them a down
						}
						else if (pf[x][j] == 53)
						{
							pf[x][j] = 0;
							pf[x][j + 1] = 53;                    //Move them a down
						}
						else if (pf[x][j] == 54)
						{
							pf[x][j] = 0;
							pf[x][j + 1] = 54;                    //Move them a down
						}
						else if (pf[x][j] == 55)
						{
							pf[x][j] = 0;
							pf[x][j + 1] = 55;                    //Move them a down
						}
						else if (pf[x][j] == 56)
						{
							pf[x][j] = 0;
							pf[x][j + 1] = 56;                    //Move them a down
						}
						else if (pf[x][j] == 57)
						{
							pf[x][j] = 0;
							pf[x][j + 1] = 57;                    //Move them a down
						}
				}
			}
		}
	}
	cout << lines;
	
	if (lines == 1)                                      //Updating score for each line clear
	{
		score += 40 * (level);
	}
	else if (lines == 2)
	{
		score += 100 * (level);
	}
	else if (lines == 3)
	{
		score += 300 * level;
	}
	else if (lines == 4)
	{
		score += 1200 * level;
	}

}