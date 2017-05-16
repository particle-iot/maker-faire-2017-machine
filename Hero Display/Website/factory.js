Snap.plugin( function( Snap, Element, Paper, global ) {
  Element.prototype.addTransform = function( t ) {
    return this.transform( this.transform().localMatrix.toTransformString() + t );
  };
});

var snap = Snap('#drawing');
snap.attr({ viewBox: "0 0 1920 1080" });

Snap.load('maker-faire-control-panel.svg', function (f) {
  function show(el) {
    el.attr('visibility', 'visible');
  }

  function hide(el) {
    el.attr('visibility', 'hidden');
  }

  function s(id) {
    return f.select(id);
  }

  function sevenSegment(top, topLeft, topRight, middle, bottomLeft, bottomRight, bottom) {
    return [s(top), s(topLeft), s(topRight), s(middle), s(bottomLeft), s(bottomRight), s(bottom)];
  }

  var sevenSegementPatterns = [
    // 0
    [     true,
     true,      true,
          false,
     true,      true,
          true
    ],
    // 1
    [     false,
     false,      true,
          false,
     false,      true,
          false
    ],
    // 2
    [     true,
     false,      true,
          true,
     true,      false,
          true
    ],
    // 3
    [     true,
     false,      true,
          true,
     false,      true,
          true
    ],
    // 4
    [     false,
     true,      true,
          true,
     false,     true,
          false
    ],
    // 5
    [     true,
     true,      false,
          true,
     false,     true,
          true
    ],
    // 6
    [     true,
     true,      false,
          true,
     true,      true,
          true
    ],
    // 7
    [     true,
     false,      true,
          false,
     false,      true,
          false
    ],
    // 8
    [     true,
     true,      true,
          true,
     true,      true,
          true
    ],
    // 9
    [     true,
     true,      true,
          true,
     false,      true,
          false
    ],
  ];
  function displayDigit(digit, segments) {
    var pattern = sevenSegementPatterns[digit];
    for (var i = 0; i < 7; i++) {
      if (pattern[i]) {
	show(segments[i]);
      } else {
	hide(segments[i]);
      }
    }
  }

  function displayNumber(number, segmentsArray) {
    displayDigit(Math.floor(number / 100) % 10, segmentsArray[0]);
    displayDigit(Math.floor(number / 10) % 10, segmentsArray[1]);
    displayDigit(number % 10, segmentsArray[2]);
  }

  var panel = s('#panel');
  var panel1 = {
    greenLight: s('#g3684'),
    greenNumbers: [
      sevenSegment('#g3348', '#g3336', '#g3352', '#g3360', '#g3340', '#g3356', '#g3344'),
      sevenSegment('#g3376', '#g3364', '#g3380', '#g3388', '#g3368', '#g3384', '#g3372'),
      sevenSegment('#g3404', '#g3392', '#g3408', '#g3416', '#g3396', '#g3412', '#g3400'),
    ]
  };
  snap.add(panel);

  hide(panel1.greenLight);

  var count = 0;
  setInterval(function () {
    count++;
    displayNumber(count, panel1.greenNumbers);
  }, 10);

  function loadData() {
    var headers = new Headers();
    headers.append('pragma', 'no-cache');
    headers.append('cache-control', 'no-cache');
    var request = new Request('can_signals.json', {
      headers: headers,
      cache: 'no-store'
    });

    return Promise.race([fetch(request), timeout(100)]).then(function (response) {
      return response.json();
    }).then(function (data) {
      updateData(data);
    }).then(reloadDataAfterInterval).catch(reloadDataAfterInterval);
  }

  var ballCount = -1;
  function updateData(data) {
    if (data.BallCount1 != ballCount) {
      ballCount = data.BallCount1;
      events.attr({ text: ballCount + ' events published' });
    }
  }

  function reloadDataAfterInterval() {
    return delay(250).then(function () {
      return loadData();
    });
  }

  function delay(milliseconds) {
    return new Promise(function (resolve) {
      setTimeout(resolve, milliseconds);
    });
  }

  function timeout(milliseconds) {
    return new Promise(function (resolve, reject) {
      setTimeout(reject, milliseconds);
    });
  }

  //loadData();
});
