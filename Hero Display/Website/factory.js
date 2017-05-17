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

  function display(el, visibility) {
    if (visibility) {
      show(el);
    } else {
      hide(el);
    }
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
    number = number || 0;
    displayDigit(Math.floor(number / 100) % 10, segmentsArray[0]);
    displayDigit(Math.floor(number / 10) % 10, segmentsArray[1]);
    displayDigit(number % 10, segmentsArray[2]);
  }

  function rotatePointer(angle, pointer) {
    angle = angle || 0;
    if (!pointer.originalBBox) {
      pointer.originalBBox = pointer.getBBox();
    }
    var bb = pointer.originalBBox;
    var matrix = new Snap.Matrix(1,0,0,1,0,0);
    matrix.rotate(angle, bb.x2-10, bb.y+10);
    pointer.attr({ transform: matrix });
  }

  var panel = s('#panel');
  var panel1 = {
    greenLight: s('#g3684'),
    greenNumbers: [
      sevenSegment('#g3348', '#g3336', '#g3352', '#g3360', '#g3340', '#g3356', '#g3344'),
      sevenSegment('#g3376', '#g3364', '#g3380', '#g3388', '#g3368', '#g3384', '#g3372'),
      sevenSegment('#g3404', '#g3392', '#g3408', '#g3416', '#g3396', '#g3412', '#g3400'),
    ],
    redLight: s('#g3696'),
    redNumbers: [
      sevenSegment('#g3432', '#g3420', '#g3436', '#g3444', '#g3424', '#g3440', '#g3428'),
      sevenSegment('#g3460', '#g3448', '#g3464', '#g3472', '#g3452', '#g3468', '#g3456'),
      sevenSegment('#g3488', '#g3476', '#g3492', '#g3500', '#g3480', '#g3496', '#g3484')
    ],
    blueLight: s('#g3708'),
    blueNumbers: [
      sevenSegment('#g3516', '#g3504', '#g3520', '#g3528', '#g3508', '#g3524', '#g3512'),
      sevenSegment('#g3544', '#g3532', '#g3548', '#g3556', '#g3536', '#g3552', '#g3540'),
      sevenSegment('#g3572', '#g3560', '#g3576', '#g3584', '#g3564', '#g3580', '#g3568')
    ],
  };

  var panel2 = {
    yellowLight: s('#g3672'),
    yellowNumbers: [
      sevenSegment('#g3600', '#g3588', '#g3604', '#g3612', '#g3592', '#g3608', '#g3596'),
      sevenSegment('#g3628', '#g3616', '#g3632', '#g3640', '#g3620', '#g3636', '#g3624'),
      sevenSegment('#g3656', '#g3644', '#g3660', '#g3668', '#g3648', '#g3664', '#g3652')
    ],
    positionCyan: s('#g3720'),
    positionOrange: s('#g3753'),
    positionPurple: s('#g3786')
  };

  var panel3 = {
    pointer: s('#path3819'),
    pointerMaxAngle: 285,
    numberIntegration1: [
      sevenSegment('#g3833', '#g3821', '#g3837', '#g3845', '#g3825', '#g3841', '#g3829'),
      sevenSegment('#g3861', '#g3849', '#g3865', '#g3873', '#g3853', '#g3869', '#g3857'),
      sevenSegment('#g3889', '#g3877', '#g3893', '#g3901', '#g3881', '#g3897', '#g3885')
    ],
    numberIntegration2: [
      sevenSegment('#g3917', '#g3905', '#g3921', '#g3929', '#g3909', '#g3925', '#g3913'),
      sevenSegment('#g3945', '#g3933', '#g3949', '#g3957', '#g3937', '#g3953', '#g3941'),
      sevenSegment('#g3973', '#g3961', '#g3977', '#g3985', '#g3965', '#g3981', '#g3969')
    ],
    numberIntegration3: [
      sevenSegment('#g4001', '#g3989', '#g4005', '#g4013', '#g3993', '#g4009', '#g3997'),
      sevenSegment('#g4029', '#g4017', '#g4033', '#g4041', '#g4021', '#g4037', '#g4025'),
      sevenSegment('#g4057', '#g4045', '#g4061', '#g4069', '#g4049', '#g4065', '#g4053')
    ]
  };

  var panel4 = {
    greenLight: s('#g4073'),
    yellowLight: s('#g4365'),
    purpleLight: s('#g4375'),
    orangeLight: s('#g4383'),
    numberResultYellow: [
      sevenSegment('#g4109', '#g4097', '#g4113', '#g4121', '#g4101', '#g4117', '#g4105'),
      sevenSegment('#g4139', '#g4127', '#g4143', '#g4151', '#g4131', '#g4147', '#g4135'),
      sevenSegment('#g4169', '#g4157', '#g4173', '#g4181', '#g4161', '#g4177', '#g4165')
    ],
    numberResultPurple: [
      sevenSegment('#g4199', '#g4187', '#g4203', '#g4211', '#g4191', '#g4207', '#g4195'),
      sevenSegment('#g4229', '#g4217', '#g4233', '#g4241', '#g4221', '#g4237', '#g4225'),
      sevenSegment('#g4259', '#g4247', '#g4263', '#g4271', '#g4251', '#g4267', '#g4255')
    ],
    numberResultOrange: [
      sevenSegment('#g4289', '#g4277', '#g4293', '#g4301', '#g4281', '#g4297', '#g4285'),
      sevenSegment('#g4319', '#g4307', '#g4323', '#g4331', '#g4311', '#g4327', '#g4315'),
      sevenSegment('#g4349', '#g4337', '#g4353', '#g4361', '#g4341', '#g4357', '#g4345')
    ]
  };
  var controls = {
    switchOn: s('#g4403'),
    switchOff: s('#g4438'),
  };

  snap.add(panel);

  hide(controls.switchOff);

  var angle = 0;
  var count = 0;
  function animate() {
    angle++;
    if (angle > maxAngle * 2) {
      angle = 0;
    }
    var maxAngle = panel3.pointerMaxAngle;
    rotatePointer(angle > maxAngle ? 2 * maxAngle - angle : angle, panel3.pointer);

    count = count + 111;
    displayNumber(count, panel1.greenNumbers);
    displayNumber(count, panel1.redNumbers);
    displayNumber(count, panel1.blueNumbers);
    display(panel1.greenLight, (count + 0) % 60 < 30);
    display(panel1.redLight, (count + 10) % 47 < 22);
    display(panel1.blueLight, (count + 40) % 50 < 25);

    displayNumber(count, panel2.yellowNumbers);
    var pos = (count % 60);
    if (pos < 20) {
      show(panel2.positionCyan);
      hide(panel2.positionOrange);
      hide(panel2.positionPurple);
    } else if(pos < 40) {
      hide(panel2.positionCyan);
      show(panel2.positionOrange);
      hide(panel2.positionPurple);
    } else {
      hide(panel2.positionCyan);
      hide(panel2.positionOrange);
      show(panel2.positionPurple);
    }
    display(panel2.yellowLight, (count + 20) % 40 < 25);

    displayNumber(count, panel3.numberIntegration1);
    displayNumber(count, panel3.numberIntegration2);
    displayNumber(count, panel3.numberIntegration3);

    displayNumber(count, panel4.numberResultYellow);
    displayNumber(count, panel4.numberResultPurple);
    displayNumber(count, panel4.numberResultOrange);
    display(panel4.greenLight, (count + 60) % 91 < 50);
    display(panel4.yellowLight, (count + 40) % 71 < 25);
    display(panel4.purpleLight, (count + 30) % 41 < 20);
    display(panel4.orangeLight, (count + 50) % 81 < 40);
  }
  //document.getElementsByTagName('body')[0].addEventListener('keyup', function (e) {
  //  animate();
  //});
  //setInterval(animmate, 10);

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

  var display = {
    lightTimeout: 1000,
    timeouts: {},
    updateData: function updateData(data) {
      // Panel 1
      this.showLightWithTimeout(data, 'BlueButtonPressed', panel1.blueLight);
      this.showLightWithTimeout(data, 'RedButtonPressed', panel1.redLight);
      this.showLightWithTimeout(data, 'GreenButtonPressed', panel1.greenLight);
      displayNumber(data.BlueBallCount, panel1.blueNumbers);
      displayNumber(data.RedBallCount, panel1.redNumbers);
      displayNumber(data.GreenBallCount, panel1.greenNumbers);

      // Panel 2
      this.showLightWithTimeout(data, 'Input2Active', panel2.yellowLight);
      displayNumber(data.BallCount2, panel2.yellowNumbers);
      if (data.InputColorHue < 30) {
        hide(panel2.positionCyan);
        show(panel2.positionOrange);
        hide(panel2.positionPurple);
      } else if(data.InputColorHue > 200) {
        hide(panel2.positionCyan);
        hide(panel2.positionOrange);
        show(panel2.positionPurple);
      } else {
        show(panel2.positionCyan);
        hide(panel2.positionOrange);
        hide(panel2.positionPurple);
      }

      // Panel 3
      var pointerRatio = 4.0;
      var pointerAngle = Math.min((data.InputCrankSpeed || 0) / pointerRatio, pointerRatio) * panel3.pointerMaxAngle;
      rotatePointer(pointerAngle, panel3.pointer);
      displayNumber(data.IntegrationCountA, panel3.numberIntegration1);
      displayNumber(data.IntegrationCountB, panel3.numberIntegration2);
      displayNumber(data.IntegrationCountC, panel3.numberIntegration3);

      // Panel 4
      this.showLightWithTimeout(data, 'Input4Active', panel4.greenLight);
      this.showLightWithTimeout(data, 'PrintingPrizeA', panel4.yellowLight);
      this.showLightWithTimeout(data, 'PrintingPrizeB', panel4.purpleLight);
      this.showLightWithTimeout(data, 'PrintingPrizeC', panel4.orangeLight);
      displayNumber(data.PrizeCountA, panel4.numberResultYellow);
      displayNumber(data.PrizeCountB, panel4.numberResultPurple);
      displayNumber(data.PrizeCountC, panel4.numberResultOrange);
    },
    showLightWithTimeout: function showLightWithTimeout(data, name, el) {
      if (data[name]) {
        clearTimeout(this.timeouts[name]);
        show(el);
        this.timeouts[name] = setTimeout(function () {
          hide(el);
        }, this.lightTimeout);
      }
    },
  };

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

  loadData();
});
