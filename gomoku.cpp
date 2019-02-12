#include <stdlib.h> /* srand, rand */
#include <iostream> /*cout, cin*/
#include <stdio.h>
#include <omp.h>
#include <cmath>
#include <string>
#include <fstream>

using namespace std;

const double TUNING_VALUE = 1.3;
const int BOARD_SIZE = 3;
const int LENGTH_TO_WIN = 3;

class TreeNode
{
	private:
		TreeNode **moves;
		long long int plays, wins;
		int move;
		
		bool p1Win, p2Win, draw; //This node ends the game
		bool leaf;
		
		bool boardP1[BOARD_SIZE][BOARD_SIZE];
		bool boardP2[BOARD_SIZE][BOARD_SIZE];
		
	public:
	    //Constructor
		TreeNode()
		{
			plays = 0;
			wins = 0;
			move = 0;
			leaf = true;
			
			//Initialize empty board
			for(int x=0;x<BOARD_SIZE;x++)
			{
				for(int y=0;y<BOARD_SIZE;y++)
				{
					boardP1[x][y]=false;
					boardP2[x][y]=false;
				}
			}
		}
		
		TreeNode(int m, bool prevBoardP1[BOARD_SIZE][BOARD_SIZE], bool prevBoardP2[BOARD_SIZE][BOARD_SIZE], int playAt)
		{
			plays = 0;
			wins = 0;
			move = m;
			leaf = true;
			
			//Copy previous board
			std::copy(&prevBoardP1[0][0], &prevBoardP1[0][0]+BOARD_SIZE*BOARD_SIZE,&boardP1[0][0]);
			std::copy(&prevBoardP2[0][0], &prevBoardP2[0][0]+BOARD_SIZE*BOARD_SIZE,&boardP2[0][0]);
			
			//Determine if this board is a victory for either player or a draw
			bool * v = isVictory(boardP1,boardP2,playAt,move);
			if(v[0]&&!v[1])
			{
				p1Win=true;
				p2Win=false;
				draw=false;
			}
			else if(v[1]&&!v[0])
			{
				p1Win=false;
				p2Win=true;
				draw=false;
			}
			else if(v[0]&&v[1])
			{
				p1Win=false;
				p2Win=false;
				draw=true;
			}
			else
			{
				p1Win=false;
				p2Win=false;
				draw=false;
			}
			
			//Free memory
			//delete[] v;
			//Shouldn't delete because it is a reference to static, not a copy
			
			//Add current play to board
			for(int y=0;y<BOARD_SIZE;y++)
			{
				for(int x=0;x<BOARD_SIZE;x++)
				{
					if(!boardP1[x][y] && !boardP2[x][y])
					{
						if(playAt>0)
						{
							playAt--;
						}
						else if(playAt==0)
						{
							//Play piece here
							if(move%2==0)
							{
								//This is player 2's move
								boardP2[x][y]=true;
							}
							else
							{
								//This is player 1's move
								boardP1[x][y]=true;
							}
							playAt--;
						}
					}
				}
			}
			
			//cout << "Board Initialized:" << endl;
			//printBoard(boardP1,boardP2);
		}
		
		/*
		*  Fill in the moves array. Either create new nodes or use existing ones
		*  if tranforming the board in some way (or not) would result in an existing node
		*  somewhere on the tree.
		*/
		void initChildren()
		{
			moves = new TreeNode*[BOARD_SIZE*BOARD_SIZE-move];
			
			for(int j=0;j<BOARD_SIZE*BOARD_SIZE-move;j++)
			{
				moves[j]= new TreeNode(move+1,boardP1,boardP2,j);
			}
			
			leaf = false;
		}
		
		/*
		*  Pick child node with the highest value to go to next.
		*  (Also increment plays on selected so other threads are likely to go elsewhere?)
		*/
		TreeNode * select()
		{
			int maxIndex=0;
			double maxValue = 0;
			
			for(int j=0;j<BOARD_SIZE*BOARD_SIZE-move;j++)
			{
				double v = moves[j]->getValue(plays);
				if(v>maxValue)
				{
					maxIndex=j;
					maxValue=v;
				}
			}
			
			return moves[maxIndex];
		}
	
		/* Add result of a playout to this node.
		*  {1,0}: Player 1 won, {0,1}: Player 2 won, {1,1}: Draw
		*/
		void addResult(bool result[])
		{
			if(move>0)
			{
				if(move%2==0)
				{
					//This is player 2's move
					if(result[1] && !result[0])
						wins++;
				}
				else
				{
					//This is player 1's move
					if(result[0] && !result[1])
						wins++;
				}
			}
		}
		
		void addPlay()
		{
			plays++;
		}
		
		/*
		*  Play a random game of Gomoku starting from this TreeNode's state.
		*  Returns who won in same format as isVictory (without {0,0} as an option)
		*/
		bool * playOut()
		{
			//TODO use better source of random
			cout << "Playing..." << endl;
			
			bool curP1[BOARD_SIZE][BOARD_SIZE];
			bool curP2[BOARD_SIZE][BOARD_SIZE];
			
			std::copy(&boardP1[0][0], &boardP1[0][0]+BOARD_SIZE*BOARD_SIZE,&curP1[0][0]);
			std::copy(&boardP2[0][0], &boardP2[0][0]+BOARD_SIZE*BOARD_SIZE,&curP2[0][0]);
			
			static bool cont[2] = {false,false};
			bool * result = cont;
			
			int m = move;
			
			while(true)
			{
				//Pick a random remaining space to play in
				m++;
				
				//Tie breaker- otherwise we get a mod by 0
				/*
				if(BOARD_SIZE*BOARD_SIZE-m==0)
				{
					static bool tie[2] = {true,true};
				    return tie;
				}*/
				
				int playAt = rand()%(BOARD_SIZE*BOARD_SIZE-m+1);
				result = isVictory(curP1,curP2,playAt,m);
				
				//If someone won or draw, stop simulating
				if(result[0]!=0 || result[1]!=0)
					break;
				
				//Update board
				for(int y=0;y<BOARD_SIZE;y++)
				{
					for(int x=0;x<BOARD_SIZE;x++)
					{
						if(!curP1[x][y] && !curP2[x][y])
						{
							if(playAt>0)
							{
								playAt--;
							}
							else if(playAt==0)
							{
								//Play piece here
								if(m%2==0)
								{
									//This is player 2's move
									curP2[x][y]=true;
									
								}
								else
								{
									//This is player 1's move
									curP1[x][y]=true;
								}
								playAt--;
							}
						}
					}
				}
				
				//printBoard(curP1,curP2);
				//cout << result[0] << " " << result[1] << " move: " << m << endl;
			}
			
			printBoard(curP1,curP2);
			cout << result[0] << " " << result[1] << endl;
			
			return result;
			
		}
		
		/*
		*  Calculate the value of this node relative to the parent.
		*  TODO Add a little randomness for ties and thread variation
		*/
		double getValue(int parentVisits) 
		{
			return wins/(double)plays + TUNING_VALUE * sqrt(log(parentVisits)/(double)plays);
		}
		
		/*
		*  Check if board is a victory for either player.
		*  {0,0}: no victory
		*  {0,1}: p1 wins
		*  {1,0}: p2 wins
		*  {1,1}: draw
		*/
		static bool * isVictory(bool boardP1[BOARD_SIZE][BOARD_SIZE], bool boardP2[BOARD_SIZE][BOARD_SIZE], int playAt, int move)
		{
			if(move==BOARD_SIZE*BOARD_SIZE)
			{
				static bool tie[2] = {true,true};
				return tie;
			}
			
			for(int y=0;y<BOARD_SIZE;y++)
			{
				for(int x=0;x<BOARD_SIZE;x++)
				{
					if(!boardP1[x][y] && !boardP2[x][y])
					{
						if(playAt>0)
						{
							playAt--;
						}
						else if(playAt==0)
						{
							//Play piece here
							if(move%2==0)
							{
								//This is player 2's move
								//Check if line through here
								if(isVictoryForPlayer(boardP2,x,y))
								{
									static bool win2[2] = {false,true};
									return win2;
								}
								
							}
							else
							{
								//This is player 1's move
								//Check if line through here
								if(isVictoryForPlayer(boardP1,x,y))
								{
									static bool win1[2] = {true,false};
									return win1;
								}
							}
							playAt--;
						}
					}
				}
			}
			static bool cont[2] = {false,false};
			return cont;
		}
		
		static bool isVictoryForPlayer(bool board[BOARD_SIZE][BOARD_SIZE], int x, int y)
		{
			int curX, curY;
			int length;
			
			//Check vertical
			curX=x;
			curY=y;
			length=1;
			
			curY++;
			while(curY<BOARD_SIZE && board[curX][curY]==true)
			{
				length+=1;
				curY++;
			}
			
			curY=y-1;
			while(curY>=0 && board[curX][curY]==true)
			{
				length+=1;
				curY--;
			}
			
			if(length>=LENGTH_TO_WIN) return true;
			
			//Check horizontal
			curX=x;
			curY=y;
			length=1;
			
			curX++;
			while(curX<BOARD_SIZE && board[curX][curY]==true)
			{
				length+=1;
				curX++;
			}
			
			curX=x-1;
			while(curX>=0 && board[curX][curY]==true)
			{
				length+=1;
				curX--;
			}
			
			if(length>=LENGTH_TO_WIN) return true;
			
			//Check diagonal
			curX=x;
			curY=y;
			length=1;
			
			curX++;
			curY++;
			while(curX<BOARD_SIZE && curY<BOARD_SIZE && board[curX][curY]==true)
			{
				length+=1;
				curX++;
				curY++;
			}
			
			curX=x-1;
			curY=y-1;
			while(curX>=0 && curY>=0 && board[curX][curY]==true)
			{
				length+=1;
				curX--;
				curY--;
			}
			
			if(length>=LENGTH_TO_WIN) return true;
			
			//Check other diagonal
			curX=x;
			curY=y;
			length=1;
			
			curX++;
			curY--;
			while(curX<BOARD_SIZE && curY>=0 && board[curX][curY]==true)
			{
				length+=1;
				curX++;
				curY--;
			}
			
			curX=x-1;
			curY=y+1;
			while(curX>=0 && curY<BOARD_SIZE && board[curX][curY]==true)
			{
				length+=1;
				curX--;
				curY++;
			}
			
			if(length>=LENGTH_TO_WIN) return true;
			
			return false;
		}
		
		/*
		*  Recursive function that drives tree learning
		*  Gets called on each subsequent selected node
		*  The last node does a playout and returns the result to above nodes
		*/
		bool * train()
		{
			addPlay();
			bool * result;
			
			//Check if this node is already a winner or loser
			//If it is, just use that
			if(p1Win)
			{
				cout << "Existing win for p1." << endl;
				printBoard(boardP1,boardP2);
				static bool win1[2] = {true,false};
				addResult(win1);
				return win1;
			}
			else if(p2Win)
			{
				cout << "Existing win for p2." << endl;
				static bool win2[2] = {false,true};
				addResult(win2);
				return win2;
			}
			else if(draw)
			{
				cout << "Existing draw." << endl;
				static bool tie[2] = {true,true};
				addResult(tie);
				return tie;
			}
			
			if(leaf)
			{
				//Reached end of tree
				
				if(plays > 1)
				{
					//Initialize children and pick one to do playout
					initChildren();
					TreeNode* next = select();
				    result = next->train();
				}
				else
				{
					//Unplayed leaf node is more of a template, doesn't really exist
					//Do a playout
					result = playOut();
				}
			}
			else
			{
				//Continue down the tree
				
				//Select child and call train recursively
				TreeNode* next = select();
				result = next->train();
			}
			
			
			addResult(result);
			return result;
		}
	
		static void printBoard(bool boardP1[BOARD_SIZE][BOARD_SIZE], bool boardP2[BOARD_SIZE][BOARD_SIZE])
		{
			for(int j=0;j<BOARD_SIZE;j++)
				cout << "+---";
			cout << endl;
			for(int x=0;x<BOARD_SIZE;x++)
			{
				cout << "| ";
				
				for(int y=0;y<BOARD_SIZE;y++)
				{
					if(boardP1[x][y])
						cout << "X";
					else if(boardP2[x][y])
						cout << "O";
					else
						cout << " ";
					cout << " | ";
				}
				
				cout << endl;
				
				for(int j=0;j<BOARD_SIZE;j++)
					cout << "+---";
				
				cout << "+" << endl;
			}
		}
		
		
		void writeToFile(ofstream& fout)
		{
			//Write data for this node
			fout << "{" << endl;
			fout << plays << endl;
			fout << wins << endl;
			fout << move << endl;
			fout << p1Win << endl;
			fout << p2Win << endl;
			fout << draw << endl;
			fout << leaf << endl;
			
			if(!leaf)
			{
				for(int j=0;j<BOARD_SIZE*BOARD_SIZE-move;j++)
				{
					moves[j]->writeToFile(fout);
				}
			}
			
			
			fout << "}" << endl;
		}
	
};

int main(int argc, char *argv[])
{
	//Pointer to root of tree, may be loaded from file or made blank
	TreeNode *root;
	
	//TODO load existing tree if passed as argument
	
	srand(time(NULL));
	
	//Else create new tree
	root = new TreeNode();
	root->initChildren();
	
	
	//Play game
	for(int j=0;j<100;j++)
		root->train();
	
	
	//Write tree to file TODO
	ofstream fout;
	fout.open("tree.out");
	root->writeToFile(fout);
	
	fout.close();
	
	
	return 0;
}