#include "aux_functions.h"
#include "saena_matrix.h"

class saena_matrix;


void setIJV(char* file_name, index_t *I, index_t *J, value_t *V, nnz_t nnz_g, nnz_t initial_nnz_l, MPI_Comm comm){

    int rank, nprocs;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    if(rank==0) printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    if(rank==0) printf("ERROR: change datatypes for function setIJV!!!");
    if(rank==0) printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

    std::vector<unsigned long> data(2*initial_nnz_l); // 2 is for i and j and val, i and j are uint so both of them are like 1 ulong.
    unsigned long* datap;
    if(initial_nnz_l != 0)
        datap = &data[0];
    else
        printf("Error: initial_nnz_l is 0 in setIJV().\n");

    // *************************** read the matrix ****************************

    MPI_Status status;
    MPI_File fh;
    MPI_Offset offset;

    int mpiopen = MPI_File_open(comm, file_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    if (mpiopen) {
        if (rank == 0) std::cout << "Unable to open the matrix file!" << std::endl;
        MPI_Finalize();
    }

    offset = rank * (index_t) (floor(1.0 * nnz_g / nprocs)) * 24; // row index(long=8) + column index(long=8) + value(double=8) = 24
    MPI_File_read_at(fh, offset, datap, 3 * initial_nnz_l, MPI_UNSIGNED_LONG, &status);
    MPI_File_close(&fh);

    for(index_t i = 0; i<initial_nnz_l; i++){
        I[i] = data[3*i];
        J[i] = data[3*i+1];
        V[i] = reinterpret_cast<double&>(data[3*i+2]);
    }
}


// parallel norm
int pnorm(std::vector<value_t>& r, value_t &norm, MPI_Comm comm){

    double dot_l = 0;
    for(auto &i : r)
        dot_l += i * i;
    MPI_Allreduce(&dot_l, &norm, 1, par::Mpi_datatype<value_t>::value(), MPI_SUM, comm);
    norm = std::sqrt(norm);

    return 0;
}

// parallel norm
value_t pnorm(std::vector<value_t>& r, MPI_Comm comm){

    double dot_l = 0.0, norm = 0.0;
    for(auto &i : r)
        dot_l += i * i;
    MPI_Allreduce(&dot_l, &norm, 1, par::Mpi_datatype<value_t>::value(), MPI_SUM, comm);

    return std::sqrt(norm);
}


double print_time(double t_start, double t_end, const std::string &function_name, MPI_Comm comm){

    int rank, nprocs;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    double min, max, average;
    double t_dif = t_end - t_start;

    MPI_Reduce(&t_dif, &min, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
    MPI_Reduce(&t_dif, &max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;

    if (rank==0)
        std::cout << std::endl << function_name << "\nmin: " << min << "\nave: " << average << "\nmax: " << max << std::endl << std::endl;

    return average;
}

double print_time(double t_dif, const std::string &function_name, MPI_Comm comm, bool print_time /*= false*/, bool print_name /*= true*/, int optype /*= 0*/){
    // optype (operation type):
    // 0: average (default)
    // 1: min
    // 2: max

    int rank = 0, nprocs = 0;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    double timeval = 0.0;
    switch(optype){
        case 0:
        default:
            MPI_Reduce(&t_dif, &timeval, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
            timeval /= nprocs;
            break;
        case 1:
            MPI_Reduce(&t_dif, &timeval, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
            break;
        case 2:
            MPI_Reduce(&t_dif, &timeval, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
            break;
    }

    std::cout << std::setprecision(8);

    if (print_time && rank==0){
        if(print_name){
            std::cout << function_name << "\n" << timeval << std::endl;
        }else{
            std::cout << timeval << std::endl;
        }
    }

    return timeval;
}

double print_time_all(double t_dif, const std::string &function_name, MPI_Comm comm){

    int rank = 0, nprocs = 0;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    double min = 0.0, max = 0.0, average = 0.0;

    MPI_Reduce(&t_dif, &min, 1, MPI_DOUBLE, MPI_MIN, 0, comm);
    MPI_Reduce(&t_dif, &max, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;

    if (rank==0)
        cout << endl << function_name << "\nmin: " << min << "\nave: " << average << "\nmax: " << max << endl;

    return average;
}

double print_time_ave(double t_dif, const std::string &function_name, MPI_Comm comm, bool print_time /*= false*/, bool print_name /*= true*/){

    int rank = 0, nprocs = 0;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    double average = 0.0;
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;

    std::cout << std::setprecision(8);

    if (print_time && rank==0){
        if(print_name){
            std::cout << function_name << "\n" << average << std::endl;
        }else{
            std::cout << average << std::endl;
        }
    }

    return average;
}

double print_time_ave2(double t_dif, const std::string &function_name, MPI_Comm comm, bool print_time /*= false*/, bool print_name /*= true*/){

    int rank, nprocs;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    double average;
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;

    if (print_time && rank==0){
        if(print_name){
            std::cout << function_name << "\n" << std::setprecision(8) << average << std::endl;
        }else{
            std::cout << std::setprecision(8) << average << ", ";
        }
    }

    return average;
}

double print_time_ave_consecutive(double t_dif, MPI_Comm comm){

    int rank = 0, nprocs = 0;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    double average = 0.0;
    MPI_Reduce(&t_dif, &average, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    average /= nprocs;

    if (rank==0)
        std::cout << average << "+";

    return average;
}

double average_time(double t_dif, MPI_Comm comm){
    int nprocs;
    MPI_Comm_size(comm, &nprocs);

    double average;
    MPI_Reduce(&t_dif, &average, 1, par::Mpi_datatype<value_t>::value(), MPI_SUM, 0, comm);
    return average/nprocs;
}


double average_iter(index_t iter, MPI_Comm comm){
    int nprocs = 0;
    MPI_Comm_size(comm, &nprocs);

    index_t average = 0;
    MPI_Reduce(&iter, &average, 1, par::Mpi_datatype<index_t>::value(), MPI_SUM, 0, comm);
    return static_cast<double>(average)/nprocs;
}


int write_agg(std::vector<unsigned long>& v, std::string name, int level, MPI_Comm comm) {

    // Create txt files with name name0.csv for processor 0, name1.csv for processor 1, etc.
    // Then, concatenate them in terminal: cat name0.csv name1.csv > V.csv

    int nprocs, rank;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    std::ofstream outFileTxt;
    std::string outFileNameTxt = "/home/majidrp/Dropbox/Projects/Saena/build/agg/";
    outFileNameTxt += name;
    outFileNameTxt += "_lev";
    outFileNameTxt += std::to_string(level);
//    outFileNameTxt += std::to_string(rank);
    outFileNameTxt += ".csv";
    outFileTxt.open(outFileNameTxt);

    outFileTxt << "agg" << std::endl;

    if (rank == 0)
//        outFileTxt << vSize << std::endl;
        for (long i = 0; i < v.size(); i++) {
//        std::cout       << R->entry[i].row + 1 + R->splitNew[rank] << "\t" << R->entry[i].col + 1 << "\t" << R->entry[i].val << std::endl;
            outFileTxt << v[i] << std::endl;
        }

    outFileTxt.clear();
    outFileTxt.close();

    return 0;
}


int generate_rhs(std::vector<value_t>& rhs, index_t mx, index_t my, index_t mz, MPI_Comm comm) {

    int rank, nprocs;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);
    bool verbose_gen_rhs = true;

    if(rank==0) printf("change datatypes for function generate_rhs!!!");

    if(verbose_gen_rhs){
        MPI_Barrier(comm);
        printf("rank = %d, generate_rhs: start \n", rank);
        MPI_Barrier(comm);}

    int       i,j,k,xm,ym,zm,xs,ys,zs;
    double    Hx,Hy,Hz;
//    double    ***array;
    index_t node;

    Hx   = 1.0 / (double)(mx);
    Hy   = 1.0 / (double)(my);
    Hz   = 1.0 / (double)(mz);

    // split the 3D grid by only the z axis. So put the whole x and y grids on processors, but split z by the number of processors.
    xs = 0;
    xm = mx;
    ys = 0;
    ym = my;
    zm = (int)floor(mz / nprocs);
    zs = rank * zm;
    if(rank == nprocs - 1)
        zm = mz - ( (nprocs - 1) * zm);

    if(verbose_gen_rhs){
        MPI_Barrier(comm);
        printf("rank = %d, corners: \nxs = %d, ys = %d, zs = %d, xm = %d, ym = %d, zm = %d\n", rank, xs, ys, zs, xm, ym, zm);
        MPI_Barrier(comm);}

    double val;
    for (k=zs; k<zs+zm; k++) {
        for (j=ys; j<ys+ym; j++) {
            for (i=xs; i<xs+xm; i++) {
                node = mx * my * k + mx * j + i; // for 2D it should be = mx * j + i
                if(rank==0) printf("node = %u\n", node);
                val = 12 * SAENA_PI * SAENA_PI
                      * cos(2*SAENA_PI*(((double)i+0.5)*Hx))
                      * cos(2*SAENA_PI*(((double)j+0.5)*Hy))
                      * cos(2*SAENA_PI*(((double)k+0.5)*Hz))
                      * Hx * Hy * Hz;
                rhs.emplace_back(val);
            }
        }
    }

    if(verbose_gen_rhs){
        MPI_Barrier(comm);
        printf("rank = %d, generate_rhs: end \n", rank);
        MPI_Barrier(comm);}

    return 0;
}


int generate_rhs_old(std::vector<value_t>& rhs){

    index_t size = rhs.size();

    //Type of random number distribution
    std::uniform_real_distribution<value_t> dist(0, 1); //(min, max)
    //Mersenne Twister: Good quality random number generator
    std::mt19937 rng;
    //Initialize with non-deterministic seeds
    rng.seed(std::random_device{}());

    for (index_t i=0; i<size; i++){
        rhs[i] = dist(rng);
//        rhs[i] = (value_t)(i+1) / 100;
//        std::cout << i << "\t" << rhs[i] << std::endl;
    }

    return 0;
}


int read_from_file_rhs(value_t *v, const std::vector<index_t>& split, char *file, MPI_Comm comm){

    int rank = 0, nprocs = 0;
    MPI_Comm_size(comm, &nprocs);
    MPI_Comm_rank(comm, &rank);

    MPI_Status status;
    MPI_File fh;
    MPI_Offset offset;

    // check if file exists
    // ====================
    std::string filename(file);
    std::ifstream inFile_check(filename.c_str());
    if (!inFile_check.is_open()) {
        if (!rank) std::cout << "\nCould not open the rhs file <" << filename << ">" << std::endl;
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    inFile_check.close();

    size_t extIndex = filename.find_last_of('.');
    if(extIndex == string::npos || extIndex == filename.size() - 1){
        if (!rank) cout << "The rhs file name does not have an extension!" << endl;
        MPI_Abort(comm, 1);
    }

    std::string file_extension = filename.substr(extIndex+1, string::npos); // string::npos -> the end of the string
    std::string outFileName = filename.substr(0, extIndex) + ".bin";

//    if(rank==0) std::cout << "file_extension: " << file_extension << std::endl;
//    if(rank==0) std::cout << "outFileName: " << outFileName << std::endl;

    if(file_extension != "bin") {

        std::ifstream inFile_check_bin(outFileName.c_str());

        if (inFile_check_bin.is_open()) {
//            if (rank == 0)
//                std::cout <<"A binary file with the same name exists. Using that file instead of the txt file.\n\n";
            inFile_check_bin.close();
        } else {

            // write the file in binary by proc 0.
            if (!rank) {

//                std::cout << "First a binary file with name \"" << outFileName
//                          << "\" will be created in the same directory. \n\n";

                std::ifstream inFile(filename.c_str());

                // ignore comments
                while (inFile.peek() == '%') inFile.ignore(2048, '\n');

                // sz: size of the vector
                nnz_t sz = 0;
                inFile >> sz;

//                printf("sz = %ld \n", sz);

                std::ofstream outFile;
                outFile.open(outFileName.c_str(), std::ios::out | std::ios::binary);

                std::vector<cooEntry> entry_temp1;

                index_t a = 0, i = 0;
                value_t c = 0.0;

                entry_temp1.resize(sz);
                while (inFile >> a >> c) {
//                 for mtx format, rows and columns start from 1, instead of 0.
//                        std::cout << "a = " << a << ", value = " << c << std::endl;
                    entry_temp1[i++] = cooEntry(a - 1, 0, c);
//                    cout << entry_temp1[i] << endl;
                }

                std::sort(entry_temp1.begin(), entry_temp1.end());

                for (i = 0; i < sz; ++i) {
//                    std::cout << entry_temp1[i] << std::endl;
//                    outFile.write((char *) &entry_temp1[i].row, sizeof(index_t));
//                    outFile.write((char *) &entry_temp1[i].col, sizeof(index_t));
                    outFile.write((char *) &entry_temp1[i].val, sizeof(value_t));
                }

                inFile.close();
                outFile.close();
            }

        }

        // wait until the binary file being written by proc 0 is ready.
        MPI_Barrier(comm);
    }

    int mpiopen = MPI_File_open(comm, outFileName.c_str(), MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    if(mpiopen){
        if (rank==0) std::cout << "Unable to open the rhs vector file!" << std::endl;
        MPI_Finalize();
        return -1;
    }

    // check if the size of rhs match the number of rows of A
//    struct stat st;
//    stat(outFileName.c_str(), &st);
//    index_t rhs_size = st.st_size / sizeof(value_t);
//    if(rhs_size != A->Mbig){
//        if(!rank){
//            printf("Error: Size of RHS does not match the number of rows of the LHS matrix!\n");
//            printf("Number of rows of LHS = %d\n", A->Mbig);
//            printf("Size of RHS = %d\n", rhs_size);
//        }
//        MPI_Barrier(comm);
//        MPI_Finalize();
//        exit(EXIT_FAILURE);
//    }

    // define the size of v as the local number of rows on each process, before removing boundary nodes
    const index_t v_sz = split[rank + 1] - split[rank];

    if(v == nullptr){
        v = saena_aligned_alloc<value_t>(v_sz);
    }

//    v = static_cast<value_t*>(aligned_alloc(ALIGN_SZ, v_sz * sizeof(value_t)));
    assert(v);

    // vector should have the following format: first line shows the value in row 0, second line shows the value in row 1
    offset = split[rank] * sizeof(value_t);
    MPI_File_read_at(fh, offset, v, v_sz, MPI_DOUBLE, &status);

//    int count;
//    MPI_Get_count(&status, MPI_UNSIGNED_LONG, &count);
    //printf("process %d read %d lines of triples\n", rank, count);
    MPI_File_close(&fh);

//    print_vector(v, -1, "v", comm);

    return 0;
}

int repart_vector(value_t *&v, index_t &sz, vector<index_t> &split, MPI_Comm comm){
    // v: the vector to be repartitioned
    // split: the desired repartition

    int rank = 0, nprocs = 0;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nprocs);

    if(nprocs == 1){
        return 0;
    }

    bool verbose_repart_v = false;

    // ************** set variables **************

//    print_vector(split, rank_v, "split", comm);
//    print_vector(v, -1, "v", comm);

    // ************** repartition v, based on A.split **************

    std::vector<index_t> split_init(nprocs + 1);
    MPI_Allgather(&sz, 1, par::Mpi_datatype<index_t>::value(), &split_init[1], 1, par::Mpi_datatype<index_t>::value(), comm);

    split_init[0] = 0;
    for(int i = 1; i < nprocs+1; i++)
        split_init[i] += split_init[i - 1];

    index_t start = 0, end = 0, start_proc = 0, end_proc = 0;
    start = split[rank];
    end   = split[rank+1];
    start_proc = lower_bound2(&*split_init.begin(), &*split_init.end(), start);
    end_proc   = lower_bound2(&*split_init.begin(), &*split_init.end(), end);
    if(split_init[rank + 1] == split[rank + 1])
        end_proc--;

//    if(rank == rank_v) printf("\nstart_proc = %u, end_proc = %u \n", start_proc, end_proc);
    assert(start_proc <= end_proc);

    std::vector<int> rcount(nprocs, 0);
    std::vector<int> scount(nprocs, 0);
    std::vector<int> rdispls(nprocs);
    std::vector<int> sdispls(nprocs);

    if(start_proc < end_proc){
//        if(rank==1) printf("start_proc = %u, end_proc = %u\n", start_proc, end_proc);
//        if(rank==1) printf("split_init[start_proc+1] = %u, split[rank] = %u\n", split_init[start_proc+1], split[rank]);
        if(start_proc < nprocs)
            rcount[start_proc] = split_init[start_proc + 1] - split[rank];
        if(end_proc < nprocs)
            rcount[end_proc]   = split[rank+1] - split_init[end_proc];

        const int max_proc = min(nprocs, end_proc);
        for(int i = start_proc+1; i < max_proc; i++){
//            if(rank==ran) printf("split_init[i+1] = %lu, split_init[i] = %lu\n", split_init[i+1], split_init[i]);
            rcount[i] = split_init[i + 1] - split_init[i];
        }

    }else if(start_proc == end_proc){
//        grids[0].rcount[start_proc] = split[start_proc + 1] - split[start_proc];
        if (start_proc < nprocs)
            rcount[start_proc] = split[rank + 1] - split[rank];
    }

#ifdef __DEBUG1__
//    print_vector(grids[0].rcount, -1, "grids[0].rcount", comm);
//    print_vector(split_init, rank_v, "split_init", comm);
//    print_vector(split, rank_v, "split", comm);
    if(verbose_repart_v){MPI_Barrier(comm); if(!rank) printf("repart_vec: step 3\n"); MPI_Barrier(comm);}
#endif

    start = split_init[rank];
    end   = split_init[rank + 1];
    start_proc = lower_bound2(&*split.begin(), &*split.end(), start);
    end_proc   = lower_bound2(&*split.begin(), &*split.end(), end);
//    if( (split_init[rank] != split_init[rank + 1]) && (split_init[rank + 1] == split[rank + 1]) )
    if( (split_init[rank] != split_init[rank + 1]) && (split_init[rank + 1] == split[rank + 1]) )
        end_proc--;

//    if(rank == rank_v) printf("\nstart_proc = %d, end_proc = %d, start = %d, end = %d\n", start_proc, end_proc, start, end);
    assert(start_proc <= end_proc);

    if(end_proc > start_proc){
//        if(rank==1) printf("start_proc = %u, end_proc = %u\n", start_proc, end_proc);
//        if(rank==1) printf("split_init[rank+1] = %u, split[end_proc] = %u\n", split_init[rank+1], split[end_proc]);
        if(start_proc < nprocs)
            scount[start_proc] = split[start_proc+1] - split_init[rank];
        if(end_proc < nprocs)
            scount[end_proc] = split_init[rank + 1] - split[end_proc];

        const int max_proc = min(nprocs, end_proc);
        for(int i = start_proc+1; i < max_proc; ++i){
            scount[i] = split[i+1] - split[i];
        }
    } else if(start_proc == end_proc) {
        if (start_proc < nprocs)
            scount[start_proc] = split_init[rank + 1] - split_init[rank];
    }

#ifdef __DEBUG1__
//    print_vector(grids[0].scount, -1, "grids[0].scount", comm);
    if(verbose_repart_v){MPI_Barrier(comm); if(!rank) printf("repart_vec: step 4\n"); MPI_Barrier(comm);}
#endif

    rdispls[0] = 0;
    for(int i = 1; i < nprocs; i++)
        rdispls[i] = rcount[i-1] + rdispls[i-1];

    sdispls[0] = 0;
    for(int i = 1; i < nprocs; i++)
        sdispls[i] = sdispls[i-1] + scount[i-1];

#ifdef __DEBUG1__
//    print_vector(grids[0].rdispls, -1, "grids[0].rdispls", comm);
//    print_vector(grids[0].sdispls, -1, "grids[0].sdispls", comm);
    if(verbose_repart_v){MPI_Barrier(comm); if(!rank) printf("repart_vec: step 5\n"); MPI_Barrier(comm);}
#endif

    // check if repartition is required. it is not required if the number of rows on all processors does not change.
    unsigned char repartition_local = 1, repartition = 1;
    if(start_proc == end_proc)
        repartition_local = 0;
    MPI_Allreduce(&repartition_local, &repartition, 1, MPI_UNSIGNED_CHAR, MPI_LOR, comm);  
//    printf("rank = %d, repartition_local = %d, repartition = %d \n", rank, repartition_local, repartition);

//    print_vector(v, -1, "v inside repartition", comm);

    // todo: replace Alltoall with a for loop of send and recv.
    if(repartition){
        value_t *v_tmp = nullptr;
        swap(v_tmp, v);
        sz = split[rank + 1] - split[rank];
        v = saena_aligned_alloc<value_t>(sz);
        MPI_Alltoallv(&v_tmp[0], &scount[0], &sdispls[0], par::Mpi_datatype<value_t>::value(),
                      &v[0],     &rcount[0], &rdispls[0], par::Mpi_datatype<value_t>::value(), comm);
        saena_free(v_tmp);
    }

#ifdef __DEBUG1__
//    print_vector(v, -1, "v after repartition", comm);
    if(verbose_repart_v){MPI_Barrier(comm); if(!rank) printf("repart_vec: end\n"); MPI_Barrier(comm);}
#endif
    return 0;
}
