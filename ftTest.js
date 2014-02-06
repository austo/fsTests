var ft = require('./build/Release/ft'),
  path = require('path'),
  numIter = process.argv[2] || 1,
  filename = process.env['FILENAME'] ?
    path.resolve(__dirname, process.env['FILENAME']) :
    path.join(__dirname, 'bigfile.txt');

var start;

function listIteration(it) {
  return function (err, data) {
    if (err) {
      console.log(err);
    }
    else {
      console.log('ITERATION %d', it + 1);
      console.log(' -> length %d',  data.length);
    }
    if (it === numIter - 1) {
      var end = Date.now();
      console.log('elapsed time: %dms', end - start);
    }
  };
}

start = Date.now();
for (var i = 0; i < numIter; ++i) {
  ft.read(filename, listIteration(i));
}