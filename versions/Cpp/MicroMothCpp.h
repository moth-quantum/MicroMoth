// (C) Copyright Moth Quantum 2025.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

// (C) Copyright IBM 2023.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#ifdef MICROMOTHCPP_H
#define MICROMOTHCPP_H

#include <iostream>
#include <math.h>
#include <vector>
#include <string>
#include <complex>
#include <map>
using namespace std;

#define ERROR(MSG) error(MSG)
#define RED "\033[31m"
#define RESET "\033[0m"

void error(const string alert) {
    cout << RED << alert << RESET << endl;
    // abort();
}

class QuantumCircuit {
    public:

        int nQubits, nBits;
        vector<vector<string>> data;

        QuantumCircuit() {};
        QuantumCircuit(int n, int m = 0) {
            setup(n, m);
        }
        // ~QuantumCircuit() {};

        void setup (int n, int m = 0) {
            nQubits = n;
            nBits = m;
            if ( !(nQubits == nBits || nBits == 0 ) ) {
                ERROR("Only the cases nQubits = nBits or nBits = 0 are supported.");
            }
        }

        void initialise (vector<double> p) {
            vector<string> init;

            // First, double-check the size of the input vector is valid.
            int t = pow(2, nQubits); // 2 ^ nQubits
            if ( !(p.size() == t || p.size() == t * 2) ) {
                ERROR("initialise: initialize circuit. Please insert a vector {} with either "+to_string(t)+" or "+to_string(t*2)+" doubles");");
            }

            data.clear();
        }

    private:
    // Add the T-gate, the SWAP-gate and the measurement-all function.

}


#endif