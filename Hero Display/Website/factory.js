Snap.plugin( function( Snap, Element, Paper, global ) {
	Element.prototype.addTransform = function( t ) {
		return this.transform( this.transform().localMatrix.toTransformString() + t );
	};
});

var s = Snap('#drawing');
s.attr({ viewBox: "0 0 850 680" });

var title;
var factory, title, events;
var cloudBack, cloudBackBubble1, cloudBackBubble2;
var cloudFront, cloudFrontBubble1;

Snap.load('cloud_factory.svg', function (f) {
  cloudBack = f.select('#cloud_back');
  cloudBackBubble1 = f.select('#cloud_back_bubble1');
  cloudBackBubble2 = f.select('#cloud_back_bubble2');
  cloudFront = f.select('#cloud_front');
  cloudFrontBubble1 = f.select('#cloud_front_bubble1');

  factory = f.select('g').addTransform('t100,100');
  s.add(factory);

  var bb = s.getBBox();
  title = s.text(bb.cx, 50, 'Welcome to the Particle Cloud Event Factory!').attr({
	'font-size': '30px',
	'text-anchor': 'middle'
  });
  events = s.text(bb.cx, 610, '999 events published').attr({
	'font-size': '30px',
	'text-anchor': 'middle'
  });

  cloudBack.transform('t-20,0');
  cloudBackBubble1.transform('t-10,0');
  cloudBackBubble2.transform('t-2,0');
  function animateClouds() {
    var t = 3000;
    var easing = mina.easeinout;
    cloudBack.animate({
	  transform: 't10,0'
    }, t, easing, function () {
	    cloudBack.animate({
		  transform: 't-20,0'
	    }, t, easing, animateClouds);
    });
    cloudBackBubble1.animate({
	  transform: 't10,0'
    }, t, easing, function () {
	    cloudBackBubble1.animate({
		  transform: 't-10,0'
	    }, t, easing);
    });
    cloudBackBubble2.animate({
	  transform: 't5,0'
    }, t, easing, function () {
	    cloudBackBubble2.animate({
		  transform: 't-2,0'
	    }, t, easing);
    });
    cloudFront.animate({
	  transform: 't20,0'
    }, t, easing, function () {
	    cloudFront.animate({
		  transform: 't0,0'
	    }, t, easing);
    });
    cloudFrontBubble1.animate({
	  transform: 't5,0'
    }, t, easing, function () {
	    cloudFrontBubble1.animate({
		  transform: 't0,0'
	    }, t, easing);
    });
  }

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

  animateClouds();
  loadData();
});
