const memoryjs = require('bindings')('memoryjs.node');
const nodeProcess = require('process');
const assert = require('assert');

let processes = memoryjs.getProcessesSync()


try {
  let args = new memoryjs.ProcessEntry();
} catch(e) {
  assert(e instanceof TypeError);
  assert.strictEqual(e.message, 'Must be of type PROCESSENTRY32');
}

try {
  console.log(memoryjs.getProcessesSync(55));
} catch(e) {
  assert(e instanceof Error);
  assert.strictEqual(e.message, 'sync call takes no arguments');
}

try {
  console.log(memoryjs.getProcesses(5));
} catch(e) {
  assert(e instanceof Error);
  assert.strictEqual(e.message, 'first argument must be a function');
}

try {
  memoryjs.getProcesses((error, values) => {
    console.log(error, values.length, 'callback');
  });
} catch(e) {
  console.log(e);
}

async function testAwait() {
  try {

    let bleh = memoryjs.getProcesses()
    console.log(bleh);
    console.log((await bleh).length, 'await');
  } catch(e) {
    console.log(e);
  }
}

testAwait();

let thisProc = processes.filter(x => x.th32ProcessID === nodeProcess.pid)[0];
assert.strictEqual(thisProc.szExeFile, 'node.exe');
thisProc.openProcess();
thisProc.closeProcess();

memoryAddon = thisProc.modules.filter(x => x.szModule === 'memoryjs.node');
