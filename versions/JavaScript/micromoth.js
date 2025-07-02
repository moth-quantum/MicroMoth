function QuantumCircuit(n, m) {
    this.num_qubits = n;
    this.num_clbits = m;
    this.name = '';
    this.data = [];
}

(QuantumCircuit.prototype).x = function (q) {
    this.data.push(['x', q]);
    return this;
}