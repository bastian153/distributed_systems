#include <omp.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>


#define LINES 100000
#define LENGTH 15


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




int
main(int argc, char** argv)
{
    if(argc != 3)
    {
        std::cout << "ERROR: Incorrect number of arguments. Format is: <filename> <numThreads> " << std::endl;
    }

    int threadId;
    int numThreads = std::atoi(argv[2]);
    int sizePerProcess = LINES / numThreads;
    Result max(0,0,0);

    char file[LINES][LENGTH];
    std::ifstream openFile(argv[1]);
    if(!openFile.is_open()){
        std::cout << "Error: Could not open file " << argv[1] << std::endl;
        return 0;
    }
    std::string fileLine;
    int iter = 0;
    while(std::getline(openFile, fileLine) && iter < 100000){
        strcpy(file[iter], fileLine.c_str());
        iter = iter + 1;
    }

    // Part A: Static
    int i;
    Result localMax(0,0,0);
    Result loopMax;
    #pragma omp parallel private(threadId, localMax, loopMax) num_threads(numThreads)
    {
        threadId = omp_get_thread_num();
        #pragma omp for schedule(static)
        for(i = 0; i < LINES; i++){
            loopMax = findMaxPalindrome(file[i], i);
            if(localMax < loopMax)
                localMax = loopMax;
        }

        #pragma omp critical
        {
            if(max < localMax){
                max = localMax;
            }
        }
    }
    // End of Part A: Static
    DoOutput(max);

	Result dynamicMax(0,0,0);
    // Part B: Dynamic
    #pragma omp parallel private(threadId, localMax, loopMax) num_threads(numThreads)
    {
        threadId = omp_get_thread_num();

        #pragma omp for schedule(dynamic, 1000)
        for(i = 0; i < LINES; i++){
            loopMax = findMaxPalindrome(file[i], i);
            if(localMax < loopMax)
                localMax = loopMax;
        }

        #pragma omp critical
        {
            if(dynamicMax < localMax){
                dynamicMax = localMax;
            }
        }
    }
    // End of Part B: Dynamic
    DoOutput(dynamicMax);

    return 0;
}
