// void scatterB(const std::vector<double>& B, std::vector<double>& Bcol,
//               int N, int localCols,
//               int rank, int p2, MPI_Comm gridComm, int x) {
//     if (x != 0) {
//         return;
//     }
//
//     if (rank == 0) {
//         std::vector<double> tmp(N * localCols);
//
//         for (int cy = 0; cy < p2; ++cy) {
//             packBStripe(B, tmp, N, cy, localCols);
//
//             int destCoords[2] = {0, cy};
//             int destRank;
//             MPI_Cart_rank(gridComm, destCoords, &destRank);
//
//             if (destRank == 0) {
//                 Bcol = tmp;
//             } else {
//                 MPI_Send(tmp.data(), N * localCols, MPI_DOUBLE, destRank, 200, gridComm);
//             }
//         }
//     } else {
//         MPI_Recv(Bcol.data(), N * localCols, MPI_DOUBLE, 0, 200, gridComm, MPI_STATUS_IGNORE);
//     }
// }