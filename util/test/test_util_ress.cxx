#include "WireCellUtil/Ress.h"

#include <iostream>

int main(int argc, char* argv[])
{
    using namespace std;
    using namespace WireCell::Ress;

    const int N_CELL = 40;
    const int N_ZERO = 30;
    const int N_WIRE = int(N_CELL * 0.7);

    // initialize C vector: NCELL cells with NZERO zeros. (true charge in each cell)
    vector_t blobs = vector_t::Random(N_CELL) * 50 + vector_t::Constant(N_CELL, 150);
    vector_t r = N_CELL / 2 * (vector_t::Random(N_ZERO) + vector_t::Constant(N_ZERO, 1));
    for (int i = 0; i < N_ZERO; i++) {
        blobs(int(r(i))) = 0;
    }

    // Response/connection/geometry matrix: N_WIRE rows and N_CELL columns
    matrix_t response = matrix_t::Zero(N_WIRE, N_CELL);
    for (int i = 0; i < N_WIRE; i++) {
        vector_t t = vector_t::Random(N_CELL);
        for (int j = 0; j < N_CELL; j++) {
            response(i, j) = int(t(j) + 1);
        }
    }

    // measured charge on wires.
    vector_t measured = response * blobs;

    cout << "geometry matrix:" << endl;
    cout << response << endl << endl;

    cout << "true charge of each cell:" << endl;
    cout << blobs.transpose() << endl << endl;

    Params params{lasso};
    auto beta = solve(response, measured, params);

    cout << beta.transpose() << endl << endl;

    auto predicted = predict(response, blobs);
    double mr = mean_residual(measured, predicted);

    cout << "average residual charge difference per wire: " << mr << endl;

    int nbeta = beta.size();

    int n_zero_true = 0;
    int n_zero_beta = 0;
    int n_zero_correct = 0;

    for (int i = 0; i < nbeta; i++) {
        if (fabs(blobs(i)) < 0.1) n_zero_true++;
        if (fabs(beta(i)) < 5) n_zero_beta++;
        if (fabs(blobs(i)) < 0.1 && (fabs(blobs(i) - beta(i)) < 10)) n_zero_correct++;
    }
    cout << "true zeros: " << n_zero_true << endl;
    cout << "fitted zeros: " << n_zero_beta << endl;
    cout << "correct fitted zeros: " << n_zero_correct << endl;

    cout << endl;
}
