const memoryjs = require('bindings')('memoryjs.node');
const util = require('util');

console.log(memoryjs);
let processes = memoryjs.getProcessesSync()
for (let i = 0; i < 20; i++) {
  console.log(processes[i]);
}

try {
  let args = new memoryjs.ProcessEntry();
} catch(e) {
  console.log(e);
}

try {
  console.log(memoryjs.getProcessesSync(55));
} catch(e) {
  console.log(e);
}

try {
  console.log(memoryjs.getProcesses(5));
} catch(e) {
  console.log(e);
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

let notepad = processes.filter(x => x.szExeFile === 'notepad.exe')[0];
notepad.openProcess();
notepad.closeProcess();
