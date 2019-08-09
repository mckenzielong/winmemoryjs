const memoryjs = require('bindings')('memoryjs.node');
console.log(memoryjs);
console.log(memoryjs.getProcessesSync());

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