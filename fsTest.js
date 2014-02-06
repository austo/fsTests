var fs = require('fs'),
  path = require('path'),
  numIter = process.argv[2] || 1,
  filename = process.env['FILENAME'] ?
    path.resolve(__dirname, process.env['FILENAME']) :
    path.join(__dirname, 'bigfile.txt');

var start;

function readFile(fpath, callback) {
  fs.open(fpath, 'r', function (err, fd) {
    fs.fstat(fd, handleStats(fd, callback));
  });
}

function handleStats(fd, callback) {
  var cb = function () {
    var args = Array.prototype.slice.call(arguments);
    callback.apply(null, args.concat(fd));
  };

  return function (err, stats) {
    var buf = new Buffer(stats.size);
    fs.read(fd, buf, 0, stats.size, 0, cb);
  };
}

function listIteration(it) {
  return function (err, bytesRead, buffer) {
    if (err) {
      console.log(err);
    }
    else {
      console.log('ITERATION %d', it + 1);
      console.log(' -> length %d', buffer.length);
    }
    if (typeof arguments[3] === 'number') {
      fs.close(arguments[3], null);
      if (it === numIter - 1) {
        var end = Date.now();
        console.log('elapsed time: %dms', end - start);
      }
    }
  };
}

start = Date.now();
for (var i = 0; i < numIter; ++i) {
  readFile(filename, listIteration(i));
}