#include <iostream>
#include <algorithm>
#include "mpi.h"

#include "grid.h"
#include "saena.hpp"


using namespace std;

int main(int argc, char* argv[]){

    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    int nprocs, rank;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    unsigned long i;
//    int assert1, assert2, assert3;

    if(argc != 3)
    {
        if(rank == 0)
        {
            cout << "Usage: ./Saena <MatrixA> <rhs_vec>" << endl;
            cout << "Matrix file should be in triples format." << endl;
        }
        MPI_Finalize();
        return -1;
    }
    // *************************** get number of rows ****************************

//    char* Vname(argv[2]);
//    struct stat vst;
//    stat(Vname, &vst);
//    unsigned int Mbig = vst.st_size/8;  // sizeof(double) = 8
//    unsigned int num_rows_global = stoul(argv[3]);

    // *************************** initialize the matrix ****************************

    char* file_name(argv[1]);

    // todo: set nnz_g for every example.
    unsigned int nnz_g = 393;
    auto initial_nnz_l = (unsigned int) (floor(1.0 * nnz_g / nprocs)); // initial local nnz
    if (rank == nprocs - 1)
        initial_nnz_l = nnz_g - (nprocs - 1) * initial_nnz_l;

    auto* I = (unsigned int*) malloc(sizeof(unsigned int) * initial_nnz_l);
    auto* J = (unsigned int*) malloc(sizeof(unsigned int) * initial_nnz_l);
    auto* V = (double*) malloc(sizeof(double) * initial_nnz_l);
    setIJV(file_name, I, J, V, nnz_g, initial_nnz_l, comm);

//    if(rank==0)
//        for(i=0; i<initial_nnz_l; i++)
//            std::cout << I[i] << "\t" << J[i] << "\t" << V[i] << std::endl;

    // timing the setup phase

//    saena::matrix A (file_name, comm);
    saena::matrix A(comm);
    A.set(I, J, V, initial_nnz_l);

    double t1 = MPI_Wtime();

    A.assemble();

    double t2 = MPI_Wtime();
    double t_dif, average, min, max;
    t_dif = t2 - t1;
    MPI_Reduce(&t_dif, &min, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
    MPI_Reduce(&t_dif, &max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;
//    if (rank==0)
//        cout << "\nMatrix assemble:\nmin: " << min << "\nave: " << average << "\nmax: " << max << endl << endl;

//    free(I); free(J); free(V);

    unsigned int num_local_row = A.get_num_local_rows();

    // *************************** read the vector and set rhs ****************************

    MPI_Status status;
    MPI_File fh;
    MPI_Offset offset;

    char* Vname(argv[2]);
    int mpiopen = MPI_File_open(comm, Vname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    if(mpiopen){
        if (rank==0) cout << "Unable to open the rhs vector file!" << endl;
        MPI_Finalize();
        return -1;
    }

    // define the size of v as the local number of rows on each process
    std::vector <double> v(num_local_row);
    double* vp = &(*(v.begin()));

    // vector should have the following format: first line shows the value in row 0, second line shows the value in row 1
    offset = A.get_internal_matrix()->split[rank] * 8; // value(double=8)
    MPI_File_read_at(fh, offset, vp, num_local_row, MPI_DOUBLE, &status);

//    int count;
//    MPI_Get_count(&status, MPI_UNSIGNED_LONG, &count);
    //printf("process %d read %d lines of triples\n", rank, count);
    MPI_File_close(&fh);

    // set rhs
    std::vector<double> rhs(num_local_row);
    A.get_internal_matrix()->matvec(&*v.begin(), &*rhs.begin());
//    rhs.assign(num_local_row, 0);
//    if(rank==0)
//        for(i = 0; i < rhs.size(); i++)
//            cout << rhs[i] << endl;

//    for(i=0; i<num_local_row; i++)
//        rhs[i] = i + 1 + A.split[rank];

    // *************************** set u0 ****************************
    // there are 3 options for u0:
    // 1- zero
    // 2- random
    // 3- flat from homg

    std::vector<double> u(num_local_row);
    u.assign(num_local_row, 0); // initial guess = 0
    //    randomVector2(u); // initial guess = random
    //    if(rank==1) cout << "\ninitial guess u" << endl;
    //    if(rank==1)
    //        for(auto i:u)
    //            cout << i << endl;

/*
    // u0 is generated as flat from homg: u0 = eigenvalues*ones. check homg010_u0flat.m file
    MPI_Status status3;
    MPI_File fh3;
    MPI_Offset offset3;

    char* Uname(argv[3]);
    int mpiopen3 = MPI_File_open(comm, Uname, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh3);
    if(mpiopen3){
        if (rank==0) cout << "Unable to open the U vector file!" << endl;
        MPI_Finalize();
        return -1;
    }

    // vector should have the following format: first line shows the value in row 0, second line shows the value in row 1
    offset3 = A.split[rank] * 8; // value(double=8)
    MPI_File_read_at(fh3, offset3, &*u.begin(), num_local_row, MPI_DOUBLE, &status3);
    MPI_File_close(&fh3);
*/

//    if(rank==0)
//        for(i = 0; i < u.size(); i++)
//            cout << u[i] << endl;

    // *************************** AMG - Setup ****************************

    t1 = MPI_Wtime();

//    int max_level             = 2;
    int vcycle_num            = 1;
    double relative_tolerance = 1e-10;
    std::string smoother      = "jacobi";
    int preSmooth             = 2;
    int postSmooth            = 2;

    saena::options opts(vcycle_num, relative_tolerance, smoother, preSmooth, postSmooth);
//    saena::options opts((char*)"options001.xml");
//    saena::options opts;
    saena::amg solver(&A);

    t2 = MPI_Wtime();
    t_dif = t2 - t1;
    MPI_Reduce(&t_dif, &min, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
    MPI_Reduce(&t_dif, &max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;
//    if (rank==0)
//        cout << "\nSolve phase:\nmin: " << min << "\nave: " << average << "\nmax: " << max << endl << endl;

//    MPI_Barrier(comm);
//    for(int i=0; i<maxLevel; i++)
//        if(rank==0) cout << "size = " << maxLevel << ", current level = " << grids[i].currentLevel << ", coarse level = " << grids[i].coarseGrid->currentLevel
//                         << ", num_local_rowbig = " << grids[i].A->Mbig << ", num_local_row = " << grids[i].A->M << ", Ac.Mbig = " << grids[i].Ac.Mbig << ", Ac.M = " << grids[i].Ac.M << endl;
//    MPI_Barrier(comm);

    // *************************** AMG - Solve ****************************

    t1 = MPI_Wtime();

    solver.solve(u, rhs, &opts);

    t2 = MPI_Wtime();
    t_dif = t2 - t1;
    MPI_Reduce(&t_dif, &min, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
    MPI_Reduce(&t_dif, &max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;
//    if (rank==0)
//        cout << "\nSolve phase:\nmin: " << min << "\nave: " << average << "\nmax: " << max << endl << endl;

    // *************************** Residual ****************************

/*
    std::vector<double> res(num_local_row);
    Saena1.residual(&A, u, rhs, res, comm);
    double dot;
    Saena1.dotProduct(res, res, &dot, comm);
    dot = sqrt(dot);

    double rhsNorm;
    Saena1.dotProduct(rhs, rhs, &rhsNorm, comm);
    rhsNorm = sqrt(rhsNorm);

    double relativeResidual = dot / rhsNorm;
    if(rank==0) cout << "relativeResidual = " << relativeResidual << endl;
*/

    // *************************** tests ****************************

//    std::vector<double> res(num_local_row);
//    Saena1.residual(&A, u, rhs, res, comm);
//    Saena1.writeVectorToFile(v, num_local_rowbig, "V", comm);

    // write the solution of overall multigrid to file
//    Saena1.writeVectorToFile(u, num_local_rowbig, "V", comm);
//    int Saena1::writeVectorToFile(std::vector<T>& v, unsigned long vSize, std::string name, MPI_Comm comm) {

    // write the solution of only jacobi to file
//    std::vector<double> uu;
//    uu.assign(num_local_row, 0);
//    for(i=0; i<10; i++)
//        A.jacobi(uu, rhs, comm);
//    Saena1.writeVectorToFile(uu, num_local_rowbig, "U", comm);

//    std::vector<double> resCoarse(grids[1].A->M);
//    grids[0].R.matvec(&*rhs.begin(), &*resCoarse.begin(), comm);
//    for(i=0; i<resCoarse.size(); i++)
//        resCoarse[i] = -resCoarse[i];
//    if(rank==3)
//        for(auto i:resCoarse)
//            cout << -i << endl;

//    if(rank==0){
//        cout << "nnz_l = " << grids[0].R.nnz_l << ", nnz_l_local = " << grids[0].R.nnz_l_local << ", nnz_l_remote = " << grids[0].R.nnz_l_remote << endl;
//        for(i=0; i<grids[0].R.entry.size(); i++)
//            cout << grids[0].R.entry[i].row << "\t" << grids[0].R.entry[i].col << "\t" << grids[0].R.entry[i].val << endl;
//    }

//    grids[0].P.matvec(&*resCoarse.begin(), &*u.begin(), comm);
//    if(rank==0)
//        for(i=0; i<u.size(); i++)
//            cout << u[i] << endl;

/*
    // write norm of residual for mutile solve iterations
//    std::vector <long> v1;
//    std::vector <float> v2;
//    std::vector <double> v3;
    double dot;
    std::vector<double> res(num_local_row);
    std::vector<double> res_norm;
//    string name;
    for(i=0; i<10; i++){
//        name = "V";
//        name += std::to_string(i);
//        name += "_";
        Saena1.AMGSolve(grids, u, rhs, comm);
        Saena1.residual(&A, u, rhs, res, comm);
        Saena1.dotProduct(res, res, &dot, comm);
        res_norm.push_back(sqrt(dot));
//        Saena1.writeVectorToFiled(u, num_local_rowbig, name, comm);
//        Saena1.test(v1);
    }
    Saena1.writeVectorToFiled(res_norm, res_norm.size(), "res_norm", comm);
*/

//    Saena1.writeMatrixToFileA(grids[1].A, "Ac", comm);
//    Saena1.writeMatrixToFileP(&grids[0].P, "P", comm);
//    Saena1.writeMatrixToFileR(&grids[0].R, "R", comm);

    // *************************** write residual or the solution to a file ****************************

//    double dot;
//    std::vector<double> res(num_local_row);
//    Saena1.residual(&A, u, rhs, res, comm);
//    Saena1.dotProduct(res, res, &dot, comm);
//    if(rank==0) cout << "initial residual = " << sqrt(dot) << endl;

//    A.jacobi(u, rhs, comm);
//    int max = 20;
//    double tol = 1e-12;
//    Saena1.solveCoarsest(&A, u, rhs, max, tol, comm);
//    A.jacobi(u, rhs, comm);

//    Saena1.residual(&A, u, rhs, res, comm);
//    Saena1.dotProduct(res, res, &dot, comm);
//    if(rank==0) cout << "final residual = " << sqrt(dot) << endl;

/*
    char* outFileNameTxt = "u.bin";
    MPI_Status status2;
    MPI_File fh2;
    MPI_Offset offset2;
    MPI_File_open(comm, outFileNameTxt, MPI_MODE_CREATE| MPI_MODE_WRONLY, MPI_INFO_NULL, &fh2);
    offset2 = A.split[rank] * 8; // value(double=8)
    // write the solution
    MPI_File_write_at(fh2, offset2, &*u.begin(), num_local_row, MPI_DOUBLE, &status2);
    // write the residual
//    MPI_File_write_at(fh2, offset2, &*res.begin(), num_local_row, MPI_DOUBLE, &status2);
    MPI_File_close(&fh2);
*/

    // *************************** use jacobi to find the answer x ****************************

/*
//    for(unsigned int i=0; i<num_local_row; i++)
//        v[i] = i + 1 + A.split[rank];

    std::vector<double> res(num_local_row);
    Saena1.residual(&A, u, rhs, res, comm);
    double dot;
    Saena1.dotProduct(res, res, &dot, comm);
    double initialNorm = sqrt(dot);
    if(rank==0) cout << "\ninitial norm(res) = " << initialNorm << endl;

    // initial x for Ax=b
//    std::vector <double> x(num_local_row);
//    double* xp = &(*(x.begin()));
//    x.assign(num_local_row, 0);
    // u first points to the initial guess, after doing jacobi it is the approximate answer for the system
    // vp points to the right-hand side
    int vv = 20;
    for(int i=0; i<vv; i++){
        A.jacobi(u, rhs, comm);
        Saena1.residual(&A, u, rhs, res, comm);
        Saena1.dotProduct(res, res, &dot, comm);
//        if(rank==0) cout << sqrt(dot) << endl;
        if(rank==0) cout << sqrt(dot)/initialNorm << endl;
    }

//    for(auto i:u)
//        cout << i << endl;

//    Saena1.residual(&A, u, rhs, res, comm);
//    if(rank==0)
//        for(auto i:res)
//            cout << i << endl;
//    Saena1.dotProduct(res, res, &dot, comm);
//    if(rank==0) cout << sqrt(dot)/initialNorm << endl;
*/

    // *************************** write the result of jacobi (or its residual) to file ****************************

/*
    char* outFileNameTxt = "jacobi_saena.bin";
    MPI_Status status2;
    MPI_File fh2;
    MPI_Offset offset2;
    MPI_File_open(comm, outFileNameTxt, MPI_MODE_CREATE| MPI_MODE_WRONLY, MPI_INFO_NULL, &fh2);
    offset2 = A.split[rank] * 8; // value(double=8)
    // write the solution
    MPI_File_write_at(fh2, offset2, &*u.begin(), num_local_row, MPI_DOUBLE, &status2);
    // write the residual
//    MPI_File_write_at(fh2, offset2, &*res.begin(), num_local_row, MPI_DOUBLE, &status2);
//    int count2;
//    MPI_Get_count(&status2, MPI_UNSIGNED_LONG, &count2);
    //printf("process %d wrote %d lines of triples\n", rank, count2);
    MPI_File_close(&fh2);
*/

    // *************************** matvec ****************************

/*
    std::vector <double> w(num_local_row);
    double* wp = &(*(w.begin()));
    int time_num = 4; // 4 of them are used to time 3 phases in matvec. check the print section to see how they work.
    double time[time_num]; // array for timing matvec
    fill(&time[0], &time[time_num], 0);
    // warming up
    for(int i=0; i<ITERATIONS; i++){
        num_local_rowatvec(vp, wp, time);
        v = w;
    }
    fill(&time[0], &time[time_num], 0);
    MPI_Barrier(comm);
    t1 = MPI_Wtime();
    for(int i=0; i<ITERATIONS; i++){
        num_local_rowatvec(vp, wp, time);
        v = w;
//        for(int j=0; j<time_num; j++)
//            time[j] += time[j]/ITERATIONS;
    }
    MPI_Barrier(comm);
    t2 = MPI_Wtime();
    //end of timing matvec
    if (rank==0){
        cout << "Saena matvec total time: " << (time[0]+time[3])/ITERATIONS << endl;
        cout << "phase 0: " << time[0]/ITERATIONS << endl;
        cout << "phase 1: " << (time[3]-time[1]-time[2])/ITERATIONS << endl;
        cout << "phase 2: " << (time[1]+time[2])/ITERATIONS << endl;
    }
    if (rank==0)
        cout << "Matvec in Saena took " << (t2 - t1)/ITERATIONS << " seconds!" << endl;
*/

    // *************************** write the result of matvec to file ****************************

/*
    char* outFileNameTxt = "matvec_result_saena.bin";
    MPI_Status status2;
    MPI_File fh2;
    MPI_Offset offset2;
    MPI_File_open(comm, outFileNameTxt, MPI_MODE_CREATE| MPI_MODE_WRONLY, MPI_INFO_NULL, &fh2);
    offset2 = A.split[rank] * 8; // value(double=8)
    MPI_File_write_at(fh2, offset2, vp, num_local_row, MPI_UNSIGNED_LONG, &status2);
    int count2;
    MPI_Get_count(&status2, MPI_UNSIGNED_LONG, &count2);
    //printf("process %d wrote %d lines of triples\n", rank, count2);
    MPI_File_close(&fh2);
*/

    // *************************** finalize ****************************

//    MPI_Barrier(comm); cout << rank << "\t*******end*******" << endl;
    A.destroy();
    solver.destroy();
    MPI_Finalize();
    return 0;
}
