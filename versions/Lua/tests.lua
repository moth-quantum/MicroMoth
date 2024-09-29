
dofile("MicroMoth.lua")

function test_cx00 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.cx(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 1 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for CX 00")
end

function test_cx01 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.x(0)
	qc.cx(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 1
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for CX 01")
end

function test_cx10 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.x(1)
	qc.cx(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 1 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for CX 10")
end

function test_cx11 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.x(0)
	qc.x(1)
	qc.cx(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 1 and statevec[3][1] == 0 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for CX 11")
end

function test_cx_bell ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase
	
	local half_prob = 1/math.sqrt(2)

	qc.set_registers(2)
	qc.h(1)
	qc.cx(1,0)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == half_prob and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == half_prob
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for CX Bell")

end

function test_swap00 ()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
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
	qc.swap(0,1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 1
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for SWAP 11")
end


function test_rx0()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_desired_phase

	qc.set_registers(1)
	qc.rx(math.pi/4, 0)
	statevec = simulate(qc, "statevector")
	-- tostring here is used due to some unexpected error when checking two equal floating point numbers
	is_desired_state = tostring(statevec[1][1]) == "0.92387953251129" and tostring(statevec[2][1]) == "0.0"
	has_desired_phase = tostring(statevec[1][2]) == "0.0" and tostring(statevec[2][2]) == "-0.38268343236509"
	assert(is_desired_state and has_desired_phase , "Invalid Final State for RX 0")

end

function test_rx1()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_desired_phase

	qc.set_registers(1)
	qc.x(0)
	qc.rx(math.pi/4, 0)
	statevec = simulate(qc, "statevector")
	-- tostring here is used due to some unexpected error when checking two equal floating point numbers
	is_desired_state = tostring(statevec[1][1]) == "0.0" and tostring(statevec[2][1]) == "0.92387953251129"
	has_desired_phase = tostring(statevec[1][2]) == "-0.38268343236509" and tostring(statevec[2][2]) == "0.0"
	assert(is_desired_state and has_desired_phase , "Invalid Final State for RX 1")
end

function test_crx00()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.crx(math.pi/4, 0, 1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 1 and statevec[2][1] == 0 and statevec[3][1] == 0 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for CRX 00")
end

function test_crx01()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_desired_phase

	qc.set_registers(2)
	qc.x(0)
	qc.crx(math.pi/4, 0, 1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and tostring(statevec[2][1]) == "0.92387953251129" and statevec[3][1] == 0 and tostring(statevec[4][1]) == "0.0"
	has_desired_phase = statevec[1][2] == 0 and tostring(statevec[2][2]) == "0.0" and statevec[3][2] == 0 and tostring(statevec[4][2]) == "-0.38268343236509"
	assert(is_desired_state and has_desired_phase , "Invalid Final State for CRX 01")
end

function test_crx10()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_no_phase

	qc.set_registers(2)
	qc.x(1)
	qc.crx(math.pi/4, 0, 1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and statevec[2][1] == 0 and statevec[3][1] == 1 and statevec[4][1] == 0
	has_no_phase = statevec[1][2] == 0 and statevec[2][2] == 0 and statevec[3][2] == 0 and statevec[4][2] == 0
	assert(is_desired_state and has_no_phase , "Invalid Final State for CRX 10")
end

function test_crx11()
	local qc = QuantumCircuit()
	local statevec, is_desired_state, has_desired_phase

	qc.set_registers(2)
	qc.x(0)
	qc.x(1)
	qc.crx(math.pi/4, 0, 1)
	statevec = simulate(qc, "statevector")
	is_desired_state = statevec[1][1] == 0 and tostring(statevec[2][1]) == "0.0" and statevec[3][1] == 0 and tostring(statevec[4][1]) == "0.92387953251129"
	has_desired_phase = statevec[1][2] == 0 and tostring(statevec[2][2]) == "-0.38268343236509" and statevec[3][2] == 0 and tostring(statevec[4][2]) == "0.0"
	assert(is_desired_state and has_desired_phase , "Invalid Final State for CRX 11")
end

print("Doing Tests...")


test_cx00()
test_cx01()
test_cx10()
test_cx11()

test_cx_bell()

test_swap00()
test_swap01()
test_swap10()
test_swap11()

test_rx0()
test_rx1()

test_crx00()
test_crx01()
test_crx10()
test_crx11()


print("All Tests Passed!")
