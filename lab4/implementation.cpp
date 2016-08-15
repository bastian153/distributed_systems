#include "mpi.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>

// Don't CHANGE This Code (you can add more functions)-----------------------------------------------------------------------------

void
CreateElectionArray(int numberOfProcesses,int electionArray[][2])
{

    std::vector<int> permutation;
    for(int i = 0; i < numberOfProcesses; ++i)
        permutation.push_back(i);
    std::random_shuffle(permutation.begin(), permutation.end());

    for(int i = 0; i < numberOfProcesses; ++i)
    {
        electionArray[i][0] = permutation[i];
        int chance = std::rand() % 4; // 25 % chance of inactive
        electionArray[i][1] = chance != 0; // 50% chance, 
    }

    //Check that there is at least one active
    bool atLeastOneActive = false;
    for(int i = 0; i < numberOfProcesses; ++i)
    {
        if(electionArray[i][1] == 1)
            atLeastOneActive = true;
    }
    if(!atLeastOneActive)
    {
        electionArray[std::rand() % numberOfProcesses][1] = 1;
    }
}

void
PrintElectionArray(int numberOfProcesses, int electionArray[][2])
{
    for(int i = 0; i < numberOfProcesses; ++i)
    {
        std::printf("%-3d ", electionArray[i][0]);
    }
    std::cout << std::endl;
    for(int i = 0; i < numberOfProcesses; ++i)
    {
        std::printf("%-3d ", electionArray[i][1]);
    }
    std::cout << std::endl;
}
void
PrintElectionResult(int winnerMPIRank, int round, int numberOfProcesses, int electionArray[][2])
{
    std::cout << "Round " << round << std::endl;
    std::cout << "ELECTION WINNER IS " << winnerMPIRank << "(" << electionArray[winnerMPIRank][0] << ")  !!!!!\n";
    std::cout << "Active nodes where: ";
    for(int i = 0; i < numberOfProcesses; ++i)
    {
        if(electionArray[i][1] == 1)
            std::cout << i << "(" << electionArray[i][0] << "), ";
    }
    std::cout << std::endl;
    PrintElectionArray(numberOfProcesses, electionArray);
    for(int i = 0; i < numberOfProcesses*4-2; ++i)
        std::cout << "_";
    std::cout << std::endl;
}

// CHANGE This Code (you can add more functions)-----------------------------------------------------------------------------
int
numberOfActives(int electionArray[][2], int size)
{
    int count = 0;
    for(int i = 0; i < size; i++){
        if(electionArray[i][1] == 1){
            count = count + 1;
        }
    }
    return count;
}


void
swapArray(int* first, int* second, int size)
{
    for(int i = 0; i < size; i++){
        int temp = first[i];
        first[i] = second[i];
        second[i] = temp;
    }
}


int
findBeforeNeighbor(int electionArray[][2], int size, int id)
{
    int electionId = electionArray[id][0];
    int before = electionId - 1 < 0 ? size - 1 : electionId - 1;
    int index = -1;

    for(int i = 0; i < size; i++){
        if(electionArray[i][0] == before){
            index = i;
            break;
        }
    }

    while(electionArray[index][1] == 0){
        electionId = electionArray[index][0];
        before = electionId - 1 < 0 ? size - 1 : electionId - 1;

        for(int i = 0; i < size; i++){
            if(electionArray[i][0] == before){
                index = i;
                break;
            }
        }
    }

    return index;
}


int 
findNextNeighbor(int electionArray[][2], int size, int id)
{
    int electionId = electionArray[id][0];
    int next = (electionId + 1) % size;
    int index = -1;

    for(int i = 0; i < size; i++){
        if(electionArray[i][0] == next){
            index = i;
            break;
        }
    }

    while(electionArray[index][1] == 0){
        electionId = electionArray[index][0];
        next = (electionId + 1) % size;

        for(int i = 0; i < size; i++){
            if(electionArray[i][0] == next){
                index = i;
                break;
            }
        }
    }

    return index;
}



int
election(int electionArray[][2], int size, int id, int numActive)
{
    // Find node before and after in election ring
    int prevNode = findBeforeNeighbor(electionArray, size, id);
    int nextNode = findNextNeighbor(electionArray, size, id);


    // Fill array with -1
    int sendArray[size];
    std::fill_n(sendArray, size, -1);
    int recvArray[size];
    std::fill_n(recvArray, size, -1);


    // Set Active and Inactive Nodes
    sendArray[electionArray[id][0]] = id;
    for(int i = 0; i < size; i++){
        int electionId = electionArray[i][0];
        if(electionArray[i][1] == 0){
            sendArray[electionId] = -2;
            recvArray[electionId] = -2;
        }
    }

    // Iterate through the active nodes, updating the election
    for(int i = 0; i < numActive; i++){
        MPI_Request request;
        MPI_Isend(&sendArray[0], size, MPI_INT, nextNode,
                  0, MPI_COMM_WORLD, &request);
        MPI_Recv(&recvArray[0], size, MPI_INT, prevNode,
                  0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Wait(&request, MPI_STATUS_IGNORE);

        swapArray(sendArray, recvArray, size);

        int myElectionId = electionArray[id][0];
        sendArray[myElectionId] = id;
    }

    // Last swap to undo changes
    swapArray(sendArray, recvArray, size);
    // Find the winner
    for(int i = size - 1; i >= 0; i--){
        if(recvArray[i] != -2)
            return recvArray[i];
    }
    // Will never be the case, but just in case
    return 0;
}


int
main(int argc, char* argv[])
{
    int processId;
    int numberOfProcesses;
    int winner = -1;

    // Setup MPI
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    MPI_Comm_size( MPI_COMM_WORLD, &numberOfProcesses);

    // Two arguments, the program name, the number of rounds, and the random seed
    if(argc != 3)
    {
        if(processId == 0)
        {
            std::cout << "ERROR: Incorrect number of arguments. Format is: <numberOfRounds> <seed>" << std::endl;
        }
        MPI_Finalize();
        return 0;
    }

    const int numberOfRounds = std::atoi(argv[1]);
    const int seed           = std::atoi(argv[2]);
    std::srand(seed); // Set the seed

    auto electionArray = new int[numberOfProcesses][2]; // Bcast with &electionArray[0][0]...

    for(int round = 0; round < numberOfRounds; ++round)
    {
        if(processId == 0){
            CreateElectionArray(numberOfProcesses, electionArray);
        }

        // ....... Your SPMD program goes here ............
        MPI_Bcast(&electionArray[0][0], numberOfProcesses * 2, 
                  MPI_INT, 0, MPI_COMM_WORLD);

        // If not active and not master process, wait for next election
        if(processId != 0 && electionArray[processId][1] == 0)
            continue;

        // Check the number of active process is only 1
        int numActive = numberOfActives(electionArray, numberOfProcesses);
        if(numActive == 1 && electionArray[processId][1] == 1){
            winner = processId;
        } else {
            if(electionArray[processId][1] == 1)
                winner = election(electionArray, numberOfProcesses, 
                                  processId, numActive);
            else
                winner = -1;
        }


        // If the winner is not the master, send a message
        if(winner != 0 && winner == processId){
            MPI_Send(&winner, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        
        // Master process displays the results
        if(processId == 0)
        {
            MPI_Status status;
            if(winner != processId)
                MPI_Recv(&winner, 1, MPI_INT, MPI_ANY_SOURCE,
                         0, MPI_COMM_WORLD, &status);
            PrintElectionResult(winner, round, numberOfProcesses, electionArray);
        }
    }

    delete[] electionArray;

    MPI_Finalize();

    return 0;
}
