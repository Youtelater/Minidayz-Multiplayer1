const fs = require('fs');
const vm = require('vm');

console.log("[1/3] Reading data.deobfuscated.js...");
const startTime = Date.now();
try {
    let code = fs.readFileSync('data.deobfuscated.js', 'utf8');
    console.log(`      -> Successfully read ${code.length} characters in ${(Date.now() - startTime) / 1000}s`);

    console.log("[2/3] Evaluating JavaScript environment (V8 engine)...");
    const evalStart = Date.now();
    const sandbox = {};
    vm.createContext(sandbox);
    
    // Wrap the raw object in parentheses so Node parses it as an expression, not a code block
    const wrappedCode = `var __data = (${code});`;
    vm.runInContext(wrappedCode, sandbox);
    
    const gameData = sandbox.__data;
    console.log(`      -> Evaluated successfully in ${(Date.now() - evalStart) / 1000}s`);

    console.log("[3/3] Generating clean_data.json...");
    const writeStart = Date.now();
    fs.writeFileSync('clean_data.json', JSON.stringify(gameData));
    console.log(`      -> Done! clean_data.json created in ${(Date.now() - writeStart) / 1000}s`);

} catch (err) {
    console.error("\n[ERROR] Something went wrong:", err.message);
}
