const memoryjs = require('bindings')('memoryjs.node');
console.log(memoryjs);
console.log(memoryjs.getProcessesNapi());