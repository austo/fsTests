var ft = require('./build/Release/ft'),
  path = require('path'),
  numIter = process.argv[2] || 1,
  filename = process.env['FILENAME'] ?
    path.resolve(__dirname, process.env['FILENAME']) :
    path.join(__dirname, 'bigfile.txt');


function listIteration(it) {
  return function (err, data) {
    if (err) {
      console.log(err);
    }
    else {
      console.log('ITERATION %d', it + 1);
      console.log(' -> length %d',  data.length);
    }
  };
}

for (var i = 0; i < numIter; ++i) {
  ft.readAsync(filename, listIteration(i));
}