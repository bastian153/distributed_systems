#include "mpi.h"

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <iostream>
#include <algorithm>

#define LINES 100000
#define LENGTH 15
// Don't CHANGE This Code (you can add more functions)-----------------------------------------------------------------------------

struct Result
{
    Result()
    : lineNumber(0), firstChar(0), length(0)
    {}

    Result(int lineNumber_, int firstChar_, int length_)
    : lineNumber(lineNumber_), firstChar(firstChar_), length(length_)
    {}

    // This allows you to compare results with the < (less then) operator, i.e. r1 < r2
    bool
    operator<(Result const& o)
    {
        // Line number can't be equal
        return length < o.length || 
            (length == o.length && lineNumber >  o.lineNumber) ||
            (length == o.length && lineNumber == o.lineNumber  && firstChar > o.firstChar);
    }

    int lineNumber, firstChar, length;
};

void
DoOutput(Result r)
{
    std::cout << "Result: " << r.lineNumber << " " << r.firstChar << " " << r.length << std::endl;
}

// CHANGE This Code (you can add more functions)-----------------------------------------------------------------------------
std::string 
preProcess(std::string s) 
{
	int n = s.length();
	if (n == 0) return "^$";
	std::string ret = "^";
	for (int i = 0; i < n; i++)
		ret += "#" + s.substr(i, 1);

	ret += "#$";
	return ret;
}


Result
findMaxPalindrome(std::string const&line, int const& lineNumber)
{
	std::string T = preProcess(line);
	int n = T.length();
	int *P = new int[n];
	int C = 0, R = 0;
	for (int i = 1; i < n - 1; i++) {
		int i_mirror = 2 * C - i; // equals to i' = C - (i-C)

		P[i] = (R > i) ? std::min(R - i, P[i_mirror]) : 0;

		// Attempt to expand palindrome centered at i
		while (T[i + 1 + P[i]] == T[i - 1 - P[i]]){
			P[i]++;
        }

		// If palindrome centered at i expand past R,
		// adjust center based on expanded palindrome.
		if (i + P[i] > R) {
			C = i;
			R = i + P[i];
		}
	}

	// Find the maximum element in P.
	int maxLen = 0;
	int centerIndex = 0;
	for (int i = 1; i < n - 1; i++) {
		if (P[i] > maxLen) {
			maxLen = P[i];
			centerIndex = i;
		}
	}
	delete[] P;
    // Line, Char, Length
    return Result(lineNumber, (centerIndex - 1 - maxLen) / 2, maxLen);
}


Result
calculateMax(int* lnList, int* fcList, int* lengthList, int size)
{
    Result max(lnList[0], fcList[0], lengthList[0]);
    for(int i = 1; i < size; i++){
        Result item(lnList[i], fcList[i], lengthList[i]);
        if(max < item)
            max = item;
    }
    return max;
}


void
Gather(int id, int size, int data, int* list)
{   
    int left = id == 0 ? size - 1: id - 1;
    int right = (id + 1) % size;
    if(id == 0){
        list[0] = data;
        for(int i = 0; i < size - 1; i++){
           int recv;
           MPI_Recv(&recv, 1, MPI_INT, right, 0,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
           list[i+1] = recv;
        }
    } else {
        int sendCount = size - id - 1;
        int recvCount = size - id - 1;
        int recvList[recvCount];
        // Receive from right first
        for(int i = 0; i < recvCount; i++){
            int recv;
            MPI_Recv(&recv, 1, MPI_INT, right, 0, 
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            recvList[i] = recv;
        }
        // Send to current data to the left
        MPI_Send(&data, 1, MPI_INT, left, 0, MPI_COMM_WORLD);

        // Send the rest of the data to the left
        for(int i = 0; i < sendCount; i++){
            int send = recvList[i];
            MPI_Send(&send, 1, MPI_INT, left, 0, MPI_COMM_WORLD);
        }
    }
}




int
main(int argc, char* argv[])
{
    int processId;
    int numberOfProcesses;

    // Setup MPI
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    MPI_Comm_size( MPI_COMM_WORLD, &numberOfProcesses);

    // Two arguments, the program name and the input file. The second should be the input file
    if(argc != 2)
    {
        if(processId == 0)
        {
            std::cout << "ERROR: Incorrect number of arguments. Format is: <filename>" << std::endl;
        }
        MPI_Finalize();
        return 0;
    }

    // ....... Your SPMD program goes here ............
    // Read the file if it is the root
    char file[LINES * LENGTH];
    if(processId == 0) {
         std::ifstream openFile(argv[1]);
         if(!openFile.is_open()){
             std::cout << "Error: Could not open file " << argv[1] << std::endl;
             return 0;
         }
         std::string fileLine;
         int i = 0;
         while(std::getline(openFile, fileLine) && (i/LENGTH) < LINES){
             int j;
             for(j = 0; j < fileLine.size(); j++)
                 file[i+j] = fileLine.at(j);
             if(j != 15)
                 file[i+j] = '\0';
             i = i + LENGTH;
         }
    }

    // Create array for subprocesses
    int sizePerProcess = LINES / numberOfProcesses;
    char fileSlice[sizePerProcess * LENGTH];

    // Scatter chunks to different processes
    MPI_Scatter(&file[0], sizePerProcess * LENGTH, MPI_CHAR,
                &fileSlice[0], sizePerProcess * LENGTH, MPI_CHAR,
                0, MPI_COMM_WORLD);

    // Find the local max palindrome for each process
    Result processMax(0,0,0);
    int lineNumber = sizePerProcess * processId;
    for(int i = 0; i < sizePerProcess * LENGTH; i += LENGTH){
       char line[LENGTH];
       for(int j = 0; j < LENGTH; j++){
           line[j] = fileSlice[i+j];
           if(line[j] == '\0')
               break;
       }
       Result functionMax = findMaxPalindrome(std::string(line), lineNumber);
       if(processMax < functionMax)
           processMax = functionMax;
       lineNumber = lineNumber + 1;
    }

    int lineNumberList[numberOfProcesses];
    int firstCharList[numberOfProcesses];
    int lengthList[numberOfProcesses];

    // Send the information to the master process 
    MPI_Barrier(MPI_COMM_WORLD);
    Gather(processId, numberOfProcesses,
           processMax.lineNumber, lineNumberList);
    Gather(processId, numberOfProcesses,
           processMax.firstChar, firstCharList);
    Gather(processId, numberOfProcesses,
           processMax.length, lengthList);
    MPI_Barrier(MPI_COMM_WORLD);

    // ... Eventually.. 
    if(processId == 0)
    {
        Result max = calculateMax(lineNumberList, firstCharList, 
                                  lengthList, numberOfProcesses);
        DoOutput(max);
    } 

    MPI_Finalize();
    return 0;
}
