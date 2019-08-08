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
  memoryjs.getProcessesNapi((values) => {
    console.log(values);
  });
} catch(e) {
  console.log(e);
}