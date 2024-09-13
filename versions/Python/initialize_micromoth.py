# First we strip comments and blank lines from the top-level version of MicroMoth.py 

f = open("../../micromoth.py", "r")
file = f.read().split('\n')
f.close()

for j,line in enumerate(file):
    file[j] = line.split("#")[0]
    if line.count("'''")==2:
        file[j] = line.split("'''")[0]
        
new_file = []
for j,line in enumerate(file):
    if (line!='') and (not line.isspace()):
        new_file.append(line)
        
new_file = '\n'.join(new_file)

f = open("micromoth.py", "w")
f.write( new_file )
f.close()

print('Comments removed from top-level version of MicroMoth.py.')

# Then we test to see if it works!

from micromoth import *

shots = int(1e6)

def test_trig():
    assert( sin(pi/2)==1.0 )
    assert( cos(2*pi)==1.0 )
        
def test_x():
    qc = QuantumCircuit(1)
    qc.x(0)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.0,0.0],[1.0,0.0]] )
    qc = QuantumCircuit(2)
    qc.x(1)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.0,0.0],[0.0,0.0],[1.0,0.0],[0.0,0.0]] )
    qc = QuantumCircuit(2)
    qc.x(0)
    qc.x(1)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.0,0.0],[0.0,0.0],[0.0,0.0],[1.0,0.0]] )
    
def test_h():
    qc = QuantumCircuit(2)
    qc.h(0)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.70710678118, 0.0], [0.70710678118, 0.0], [0.0, 0.0], [0.0, 0.0]] )
    qc = QuantumCircuit(2)
    qc.h(1)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.70710678118, 0.0], [0.0, 0.0], [0.70710678118, 0.0], [0.0, 0.0]] )
    qc = QuantumCircuit(2)
    qc.h(0)
    qc.h(1)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.49999999999074046, 0.0], [0.49999999999074046, 0.0], [0.49999999999074046, 0.0], [0.49999999999074046, 0.0]] )
        
def test_rx():
    qc = QuantumCircuit(1)
    qc.rx(pi/4,0)
    assert(simulate(qc,get='statevector')==[[0.9238795325112867, 0.0], [0.0, -0.3826834323650898]])
    qc = QuantumCircuit(2)
    qc.rx(pi/4,0)
    qc.rx(pi/8,1)
    assert(simulate(qc,get='statevector')==[[0.9061274463528878, 0.0], [0.0, -0.37533027751786524], [0.0, -0.18023995550173696], [-0.0746578340503426, 0.0]])
    qc.h(0)
    qc.h(1)
    assert(simulate(qc,get='statevector')==[[0.4157348061435736, -0.27778511650465676], [0.4903926401925336, 0.0975451610062577], [0.4903926401925336, -0.0975451610062577], [0.4157348061435736, 0.27778511650465676]])
    
def test_rz():
    
    # arbitrary angles
    tx = 2.8777603974458796
    tz = 0.5589019778800038

    # an rz rotatation using h*rx*h
    qcx = QuantumCircuit(1)
    qcx.rx(tx,0)
    qcx.h(0)
    qcx.rx(tz,0)
    qcx.h(0)
    ketx = simulate(qcx,get='statevector')

    # a plain rz rotation
    qcz = QuantumCircuit(1)
    qcz.rx(tx,0)
    qcz.rz(tz,0)
    simulate(qcz,get='statevector')
    ketz = simulate(qcz,get='statevector')
    
    # check they are the same
    for j in range(2):
        for k in range(2):
            assert round(ketx[j][k],3)==round(ketz[j][k],3)
                
def test_ry():
    qc = QuantumCircuit(1)
    qc.ry(pi/8,0)
    assert(simulate(qc,get='statevector')==[[0.9807852803850672, -6.938893903907228e-17], [0.19509032201251536, 0.0]])
    
def test_cx():
    qc = QuantumCircuit(2)
    qc.h(0)
    qc.cx(0,1)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.70710678118, 0.0], [0.0, 0.0], [0.0, 0.0], [0.70710678118, 0.0]] )
    qc = QuantumCircuit(2)
    qc.x(0)
    qc.cx(0,1)
    qc.cx(1,0)
    qc.cx(0,1)
    assert( simulate(qc,shots=shots,get='statevector')==[[0.0, 0.0], [0.0, 0.0], [1.0, 0.0], [0.0, 0.0]] )

def test_memory():
    qc = QuantumCircuit(2,2)
    qc.h(0)
    qc.h(1)
    qc.measure_all()
    m = simulate(qc,shots=shots,get='memory')
    assert( len(m)==shots )
    p00 = 0
    for out in m:
      p00 += round(out=='00')/shots
    assert( round(p00,2)==0.25 )
    qc = QuantumCircuit(1,1)
    qc.h(0)
    qc.measure(0,0)
    m = simulate(qc,shots=shots,get='memory')
    assert( len(m)==shots )
    p0 = 0
    for out in m:
      p0 += round(out=='0')/shots
    assert( round(p0,1)==0.5 )

def test_counts():
    qc = QuantumCircuit(2,2)
    qc.h(0)
    qc.h(1)
    qc.measure(0,0)
    qc.measure(1,1)
    c = simulate(qc,shots=shots,get='counts')
    for out in c:
      p = float(c[out])/shots
      assert( round(p,2)==0.25 )
        
def test_probs():
    qc = QuantumCircuit(2,2)
    qc.h(0)
    qc.h(1)
    p = simulate(qc,shots=shots,get='probabilities_dict')
    for out in p:
      assert( round(p[out],2)==0.25 )      
        
def test_add():
    for n in [1,2]:
        qc = QuantumCircuit(n,n)
        meas = QuantumCircuit(n,n)
        for j in range(n):
            qc.h(j)
            meas.measure(j,j)
        c = simulate(qc+meas,shots=shots,get='counts')
        for out in c:
            p = float(c[out])/shots
            assert( round(p,2)==round(1.0/2**n,2) )
    
def test_multiqubit():
    qc = QuantumCircuit(7,7)
    qc.h(0)
    qc.cx(0,2)
    qc.cx(2,1)
    qc.h(5)
    qc.cx(5,3)
    qc.cx(3,4)
    qc.cx(3,6)
    ket = simulate(qc,get='statevector')
    check = True
    for string in ['0000000','0000111','1111000','1111111']:
        check = check and round(ket[int(string,2)][0],2)==0.50
    assert( check )
    for j in range(7):
        qc.measure(j,j)
    counts  = simulate(qc,shots=shots,get='counts')
    check = True
    for string in ['0000000','0000111','1111000','1111111']:
        p = float(counts[string])/shots
        check = check and round(p,2)==0.25
    assert( check )
    print(qc.qasm())
        
def test_reorder ():
    qc = QuantumCircuit(2,2)
    qc.x(0)
    qc.measure(0,1)
    qc.measure(1,0)
    counts = simulate(qc,shots=shots,get='counts')
    assert counts['01']==shots
    qc = QuantumCircuit(5,4)
    qc.x(1)
    qc.x(3)
    qc.x(4)
    qc.measure(1,0)
    qc.measure(3,1)
    qc.measure(4,2)
    qc.measure(0,3)
    counts = simulate(qc,shots=shots,get='counts')
    assert counts['0111']==shots
    
def test_noise ():
    qc = QuantumCircuit(2,2)
    p = simulate(qc,get='probabilities_dict',noise_model=[0.1,0.2])
    correct_p = {'00': 0.7200000000000001,'01': 0.08000000000000002,'10': 0.18000000000000002,'11': 0.020000000000000004}
    for out in correct_p:
        assert p[out]==correct_p[out]
    
test_trig()
test_x()
test_h()
test_rx()
test_rz()
test_ry
test_cx()
test_memory()
test_counts()
test_add()
test_multiqubit()
#test_reorder()
test_noise ()

print('Tests passed!')
