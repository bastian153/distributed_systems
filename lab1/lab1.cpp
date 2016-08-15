#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <fstream>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <time.h>

// Just makes the code clearer.
using Lines = std::vector<std::string>;
std::mutex m; 
int globalChunk = 0;

struct Result
{
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

// Removes all non letter characters from the input file and stores the result in a Lines container
Lines
strip(std::ifstream& file)
{
    Lines result;
    result.reserve(50000); // If reading is too slow try increasing this value

    std::string workString;

    while(std::getline(file,workString))
    {
        //Strip non alpha characters
        workString.erase(std::remove_if(workString.begin(), workString.end(), 
            [] (char c) { return !std::isalpha(c); } 
        ), workString.end());
        result.push_back(workString);
        workString.clear();
    }
    return result;
}

// CHANGE This Code (you can add more functions)-----------------------------------------------------------------------------
int 
min(int const&first, int const&second) 
{
	if (first < second)
		return first;
	return second;
}


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

		P[i] = (R > i) ? min(R - i, P[i_mirror]) : 0;

		// Attempt to expand palindrome centered at i
		while (tolower(T[i + 1 + P[i]]) == tolower(T[i - 1 - P[i]]))
			P[i]++;

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




void
findPalindrome(Lines const& lines, Result &globalMax, int start, int end)
{
    Result localMax = Result(0,0,0);
    for(int i = start; i < end; i++)
    {
        Result loopMax = findMaxPalindrome(lines[i], i);
        if(localMax < loopMax)
            localMax = loopMax;
    }


    {
        std::lock_guard<std::mutex> lock(m);
        if(globalMax < localMax)
            globalMax = localMax;
    }
}







// PART A
Result
FindPalindromeStatic(Lines const& lines, int numThreads)
{

    std::vector<std::thread> threads;
    Result max(0,0,0); 
    int chunkSize = lines.size() / numThreads;
    int begin = 0;
    int end = chunkSize + 1;

    // Worker Threads
    for(int i = 0; i < numThreads - 1; i++)
    {
        threads.emplace_back(findPalindrome, std::ref(lines), std::ref(max), begin, end);
        begin = end;
        end = end + chunkSize > lines.size() ? lines.size() : end + chunkSize;
    }

    // Main Thread
    findPalindrome(lines, max, begin, end);

    // Synchronize threads
    for(int i = 0; i < numThreads - 1; i++)
        threads[i].join();

    return max;//  You could also call the constructor of Result
}




int 
getNextChunk(int &globalChunk, int numberOfChunks)
{
    int nextChunk;
    {
        std::lock_guard<std::mutex> lock(m);
        nextChunk = globalChunk;
        if(globalChunk < numberOfChunks)
            globalChunk = globalChunk + 1;
    }
    return nextChunk;
}




void
findPalindromeDynamic(Lines const& lines, Result &globalMax,
                        int numberOfChunks, int chunkSize)
{
    Result localMax(0,0,0);
    int nextChunk;
    
    while(nextChunk < numberOfChunks)
    {
        {
            std::lock_guard<std::mutex> lock(m);
            nextChunk = globalChunk;
            if(globalChunk < numberOfChunks)
                globalChunk = globalChunk + 1;
        }
        int begin = nextChunk * chunkSize; 
        int end = (nextChunk + 1) * chunkSize > lines.size() ? lines.size() : (nextChunk + 1) * chunkSize;

        for(int i = begin; i < end; i++)
        {
            Result loopMax = findMaxPalindrome(lines[i], i);
            if(localMax < loopMax)
                localMax = loopMax;
        }
    }
   
    {
        std::lock_guard<std::mutex> lock(m);
        if(globalMax < localMax)
            globalMax = localMax;
    }
}




// PART B
Result
FindPalindromeDynamic(Lines const& lines, int numThreads, int chunkSize)
{
    Result max(0,0,0);
    std::vector<std::thread> threads;
    int numberOfChunks = lines.size() / chunkSize + 1; 

    // Worker Threads
    for(int i = 0; i < numThreads - 1; i++)
    {
        threads.emplace_back(findPalindromeDynamic, std::ref(lines), 
             std::ref(max), numberOfChunks, chunkSize);
    }

    // Main Threads
    findPalindromeDynamic(lines, max, numberOfChunks, chunkSize);

    // Synchronize Threads
    for(int i = 0; i < numThreads - 1; i++)
        threads[i].join(); 

    return max;
}

// DONT CHANGE THIS -----------------------------------------------------------------------------------------------------------------

int
main(int argc, char* argv[])
{
    if(argc != 4)
    {
        std::cout << "ERROR: Incorrect number of arguments. Format is: <filename> <numThreads> <chunkSize>" << std::endl;
        return 0;
    }

    std::ifstream theFile(argv[1]);
    if(!theFile.is_open())
    {
        std::cout << "ERROR: Could not open file " << argv[1] << std::endl;
        return 0;
    }
    int numThreads = std::atoi(argv[2]); 
    int chunkSize  = std::atoi(argv[3]); 

    std::cout << "Process " << argv[1] << " with " << numThreads << " threads using a chunkSize of " << chunkSize << " for dynamic scheduling\n" << std::endl;

    Lines lines = strip(theFile);

    //Part A
    Result aResult = FindPalindromeStatic(lines, numThreads);
    std::cout << "PartA: " << aResult.lineNumber << " " << aResult.firstChar << " " << aResult.length << ":\t" << lines.at(aResult.lineNumber).substr(aResult.firstChar, aResult.length) << std::endl;
    //Part B
    Result bResult = FindPalindromeDynamic(lines, numThreads, chunkSize);
    std::cout << "PartB: " << bResult.lineNumber << " " << bResult.firstChar << " " << bResult.length << ":\t" << lines.at(bResult.lineNumber).substr(bResult.firstChar, bResult.length) << std::endl;
    return 0;
}


