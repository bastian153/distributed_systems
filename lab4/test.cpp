#include <iostream>
#include <mpi.h>



int
main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);  
    int id, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int n[10];
    if(id == 0){
        for(unsigned int i = 0; i < 10; i++){
            n[i] = i;
        }
    }

    MPI_Bcast(n, 10, MPI_INT, 0, MPI_COMM_WORLD);
    if(id == 1){
        for(int i = 0; i < 10; i++){
            std::cout << i << std::endl; 
        }
    }

    MPI_Finalize();
    return 0;
}
