const memoryjs = require('bindings')('memoryjs.node');
console.log(memoryjs);
console.log(memoryjs.getProcessesNapi());

try {
  console.log(memoryjs.getProcessesNapi(55));
} catch(e) {
  console.log(e);
}

try {
  console.log(memoryjs.getProcessesNapi(5,5));
} catch(e) {
  console.log(e);
}

try {
  memoryjs.getProcessesNapi((error, values) => {
    console.log(error, values.length);
  });
} catch(e) {
  console.log(e);
}