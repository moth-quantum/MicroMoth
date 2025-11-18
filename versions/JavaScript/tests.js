// must use node for testing

import assert from 'assert';
import QuantumCircuit, { simulate } from './micromoth.js';

const COLORS = {
	green: '\x1b[32m',
	red: '\x1b[31m',
	reset: '\x1b[0m',
};

function testMeasureAllWithoutAddingClassicalBits() {
	const qc = new QuantumCircuit(2);
	qc.h(0);
	qc.cx(0, 1);
	qc.measure_all();
	const counts = simulate(qc, 1000);

	const keys = Object.keys(counts);
	assert(keys.includes('00'));
	assert(keys.includes('11'));
}

function testMeasureAllButAddingClassicalBits() {
	const qc = new QuantumCircuit(2, 2);
	qc.h(0);
	qc.cx(0, 1);
	qc.measure_all();
	const counts = simulate(qc, 1000);

	const keys = Object.keys(counts);
	assert(keys.includes('00'));
	assert(keys.includes('11'));
}

(() => {
	let hasError = false;

	const tests = [
		testMeasureAllWithoutAddingClassicalBits,
		testMeasureAllButAddingClassicalBits,
	];

	for (const [testI, currentTest] of tests.entries()) {
		console.log('=============================================');

		process.stdout.write(`Testing ${testI + 1}: `);

		try {
			currentTest();
			console.log(`${COLORS.green}PASSED${COLORS.reset}`);
		} catch (err) {
			hasError = true;
			console.log(`${COLORS.red}FAILED${COLORS.reset}`);
			console.error(err);
		}
	}

	if (hasError) {
		process.exit(1);
	}
})();
