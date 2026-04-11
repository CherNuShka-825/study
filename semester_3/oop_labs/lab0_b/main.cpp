#include "CSV.h"


int main() {
    using namespace std;
    ifstream file_in;
    file_in.open("../input.txt", ios::in);
    if (!file_in) {
        return -1;
    }
    ofstream file_out;
    file_out.open("../output.csv", ios::out);
    if (!file_out) {
        return -1;
    }
    CSV idn;
    idn.fill_csv(file_in, file_out);
    return 0;
}
