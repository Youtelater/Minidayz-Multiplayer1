const fs = require('fs');
const vm = require('vm');

console.log("[1/4] Reading data.deobfuscated.js (this may take a moment)...");
const startTime = Date.now();
const code = fs.readFileSync('data.deobfuscated.js', 'utf8');
console.log(`      -> File read successfully in ${((Date.now() - startTime) / 1000).toFixed(2)}s`);

console.log("[2/4] Setting up Node.js sandbox environment...");
const sandbox = {};
vm.createContext(sandbox);

console.log("[3/4] Evaluating JavaScript object structure (please wait, CPU intensive)...");
const evalStart = Date.now();
vm.runInContext(code, sandbox);
console.log(`      -> Evaluated successfully in ${((Date.now() - evalStart) / 1000).toFixed(2)}s`);

console.log("      -> Locating game data variable...");
const keys = Object.keys(sandbox);
const dataKey = keys.find(k => k !== 'console' && k !== 'setTimeout'); 
const gameData = sandbox[dataKey] || sandbox;

console.log("[4/4] Writing clean_data.json (converting object to JSON)...");
const writeStart = Date.now();
fs.writeFileSync('clean_data.json', JSON.stringify(gameData));
console.log(`      -> JSON written successfully in ${((Date.now() - writeStart) / 1000).toFixed(2)}s`);

console.log(`\nAll done! Total time: ${((Date.now() - startTime) / 1000).toFixed(2)} seconds.`);
