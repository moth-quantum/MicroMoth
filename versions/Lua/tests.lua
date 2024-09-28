
dofile("MicroMoth.lua")

function test_swap00 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 1 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Initial State for SWAP 00")
	
	qc.swap(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 1 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for SWAP 00")
end

function test_swap01 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.x(0)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 1 and statevec[3][1] == 0 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Initial State for SWAP 01")
	
	qc.swap(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 1 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for SWAP 01")
end

function test_swap10 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.x(1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 1 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Initial State for SWAP 10")
	
	qc.swap(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 1 and statevec[3][1] == 0 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for SWAP 10")
end


function test_swap11 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.x(0)
	qc.x(1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 1
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Initial State for SWAP 11")
	
	qc.swap(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 1
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for SWAP 11")
end

print("Doing Tests...")

test_swap00()
test_swap01()
test_swap10()
test_swap11()

print("All Tests Passed!")
